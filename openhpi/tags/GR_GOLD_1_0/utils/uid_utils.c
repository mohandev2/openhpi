/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2003, 2004
 * Copyright (c) 2003 by Intel Corp.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *      David Judkovics <djudkovi@us.ibm.com>
 *      Renier Morales <renierm@users.sf.net>
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <SaHpi.h>
#include <config.h>
#include <oh_error.h>
#include <epath_utils.h>
#include <uid_utils.h>

static GHashTable *ep_hash_table;
static GHashTable *resource_id_hash_table;
static guint       resource_id;


/* use to build memory resident map table from file */
static int uid_map_from_file(void);
static int build_uid_map_data(int file);

/* used by oh_uid_remove() */
void write_ep_xref(gpointer key, gpointer value, gpointer file);

/* for hash table usage */
guint oh_entity_path_hash(gconstpointer key);
gboolean oh_entity_path_equal(gconstpointer a, gconstpointer b);

/*
 * oh_entity_path_hash: used by g_hash_table_new() 
 * in oh_uid_initialize(). See glib library for
 * further details.
 */
guint oh_entity_path_hash(gconstpointer key)
{
        const char *p = key;
        guint h = *p;

        int i;

        int entity_path_len;

        entity_path_len = sizeof(SaHpiEntityPathT);
        
        p += 1;
        
        for( i=0; i<entity_path_len - 1; i++ ){
/*              h = (h << 5) - h + *p; */
		h = (h * 131) + *p;
                p++;                          
        }

	/* don't change the 1009, its magic */
    	return( h % 1009 );

}

/*
 * oh_entity_path_equal: used by g_hash_table_new() 
 * in oh_uid_initialize(). See glib library for
 * further details.
 */
gboolean oh_entity_path_equal(gconstpointer a, gconstpointer b)
{
        if (!ep_cmp(a,b)) {
                return 1;
        }
        else {
                return 0;
        }
}

/**
 * oh_uid_initialize
 *
 * UID utils initialization routine
 * This functions must be called before any other uid_utils
 * are made. 
 * 
 * Returns: success 0, failure -1.
 **/
SaErrorT oh_uid_initialize(void) 
{
        static int initialized = FALSE;

        int rval;
        
        if(!initialized) {

                /* initialize hash tables */
                ep_hash_table = g_hash_table_new(oh_entity_path_hash, oh_entity_path_equal);
                resource_id_hash_table = g_hash_table_new(g_int_hash, g_int_equal);

                initialized = TRUE;

                resource_id = 1;

                /* initialize uid map */
                rval = uid_map_from_file();
        }
	else
		rval = SA_ERR_HPI_ERROR;

        return(rval);

}

/**
 * oh_uid_from_entity_path
 * @ep: value to be removed from used
 *
 * This function returns an unique value to be used as
 * an uid/resourceID base upon a unique entity path specified
 * by @ep.  If the entity path already exists, the already assigned 
 * resource id is returned.  Before returning, this call updates the
 * uid map file saved on disk.  
 * 
 * Returns: positive unsigned int, failure is 0.
 **/
guint oh_uid_from_entity_path(SaHpiEntityPathT *ep) 
{
        gpointer key;
	gpointer value;

        EP_XREF *ep_xref;

        char *uid_map_file;
        int file;
	
	SaHpiEntityPathT entitypath;

        if (!ep) return 0;
	
	ep_init(&entitypath);
	ep_concat(&entitypath,ep);
	key = &entitypath;

        /* check for presence of EP and */
        /* previously assigned uid      */
        ep_xref = (EP_XREF *)g_hash_table_lookup (ep_hash_table, key);
        if (ep_xref) {
                /*dbg("Entity Path already assigned uid. Use oh_uid_lookup().");*/
                return ep_xref->resource_id;
        }

        /* allocate storage for EP cross reference data structure*/
        ep_xref = (EP_XREF *)g_malloc0(sizeof(EP_XREF));
        if(!ep_xref) { 
                dbg("malloc failed");
                return 0;
        }

        memset(ep_xref, 0, sizeof(EP_XREF));
        memcpy(&ep_xref->entity_path, &entitypath, sizeof(SaHpiEntityPathT));

        ep_xref->resource_id = resource_id;
        resource_id++;

        value = (gpointer)ep_xref;

        /* entity path based key */   
        key = (gpointer)&ep_xref->entity_path; 
        g_hash_table_insert(ep_hash_table, key, value);

        /* resource id based key */
        key = (gpointer)&ep_xref->resource_id;
        g_hash_table_insert(resource_id_hash_table, key, value);

        /* save newly created ep xref (iud/resource_id) to map file */
        uid_map_file = (char *)getenv("OPENHPI_UID_MAP");
        if (uid_map_file == NULL) {
                uid_map_file = OH_DEFAULT_UID_MAP;
        }
        file = open(uid_map_file, O_WRONLY);
        if(file >= 0) {
                lseek(file, 0, SEEK_END);
                write(file,ep_xref, sizeof(EP_XREF));
                lseek(file, 0, SEEK_SET);
                write(file, &resource_id, sizeof(resource_id));
        }

        close(file);

        return ep_xref->resource_id;
}               

/**
 * oh_uid_remove 
 * @uid: value to be removed
 *
 * This functions removes the uid/entity path
 * pair from use and removes the use of the uid forever.
 * A new uid may be requested for this entity path
 * in the future. oh_uid_from_entity_path() writes
 * the entire uid/entity path pairings to file before 
 * returning. oh_uid_remove() deletes the pairing from file.
 * 
 * Returns: success 0, failure -1.
 **/
gint oh_uid_remove(guint uid)
{               
        EP_XREF *ep_xref;
        gpointer key;
        int rval;

        /* check netry exist in resource_id_hash_table */ 
        key = (gpointer)&uid;
        ep_xref = (EP_XREF *)g_hash_table_lookup (resource_id_hash_table, key);
        if(!ep_xref) {
                dbg("error freeing resource_id_hash_table");
                return -1;
        }

        /* check netry exist in resource_id_hash_table */ 
        key = (gpointer)&ep_xref->entity_path;
        ep_xref = (EP_XREF *)g_hash_table_lookup (ep_hash_table, key);
        if(!ep_xref) {
                dbg("error freeing resource_id_hash_table");
                return -1;
        }

        g_hash_table_remove(resource_id_hash_table, &ep_xref->resource_id);
        g_hash_table_remove(ep_hash_table, &ep_xref->entity_path);      
        
        free(ep_xref);

        rval = oh_uid_map_to_file();

        return rval;
}

/**
 * oh_uid_lookup
 * @ep: pointer to entity path used to identify resourceID/uid
 *
 * Fetches resourceID/uid based on entity path in @ep.
 *  
 * Returns: success returns resourceID/uid, failure is 0.
 **/
guint oh_uid_lookup(SaHpiEntityPathT *ep)
{
        EP_XREF *ep_xref;
	SaHpiEntityPathT entitypath;
        gpointer key;

        if (!ep) return 0;
        
	ep_init(&entitypath);
	ep_concat(&entitypath, ep);
	key = &entitypath;
        
        /* check hash table for entry in ep_hash_table */ 
        ep_xref = (EP_XREF *)g_hash_table_lookup (ep_hash_table, key);
        if(!ep_xref) {
                dbg("error looking up EP to get uid");
                return 0;
        }

        return(ep_xref->resource_id);
}

/**
 * oh_entity_path_lookup
 * @id: pointer to resource_id/uid identifying entity path
 * @ep: pointer to memory to fill in with entity path
 * 
 * Fetches entity path based upon resource id, @id.
 *
 * Returns: success 0, failed -1.
 **/
gint oh_entity_path_lookup(guint *id, SaHpiEntityPathT *ep)
{

        EP_XREF *ep_xref;
        gpointer key = id;
        
        if (!id || !ep) return -1;

        /* check hash table for entry in ep_hash_table */ 
        ep_xref = (EP_XREF *)g_hash_table_lookup (resource_id_hash_table, key);
        if(!ep_xref) {
                dbg("error looking up EP to get uid");
                return -1 ;
        }

        memcpy(ep, &ep_xref->entity_path, sizeof(SaHpiEntityPathT));

        return 0;

} 

/*
 * oh_uid_map_to_file: saves current uid and entity path mappings
 * to file, first element in file is 4 bytes for resource id,
 * then repeat EP_XREF structures holding uid and entity path pairings
 *
 * Return value: success 0, failed -1.
 */
gint oh_uid_map_to_file(void)
{
        char *uid_map_file;
        int file;

        uid_map_file = (char *)getenv("OPENHPI_UID_MAP");
        
        if (uid_map_file == NULL) {
                uid_map_file = OH_DEFAULT_UID_MAP;
        }

        file = open(uid_map_file, O_WRONLY|O_CREAT|O_TRUNC);
        if(file < 0) {
                dbg("Configuration file '%s' could not be opened", uid_map_file);
                return -1;
        }

        /* write resource id */
        write(file, (void *)&resource_id, sizeof(resource_id));

        /* write all EP_XREF data records */
        g_hash_table_foreach(resource_id_hash_table, write_ep_xref, &file);

        if(close(file) != 0) {
                dbg("Couldn't close file '%s'.", uid_map_file);
                return -1;
        }
        
        return 0;
}


/*
 * write_ep_xref: called by g_hash_table_foreach(), for each 
 * hash table entry see glib manual for further details 
 * 
 * Return value: None (void).
 */
void write_ep_xref(gpointer key, gpointer value, gpointer file)
{
        write(*(int *)file, value, sizeof(EP_XREF));
}


/*
 * uid_map_from_file: called from oh_uid_initialize() during intialization
 * This function, if a uid map file exists, reads the current value for
 * uid and intializes the memory resident uid map file from file.
 * 
 * Return value: success 0, error -1.
 */
static gint uid_map_from_file(void)
{
        char *uid_map_file;
        int file;
        int rval;

         /* initialize uid map file */
         uid_map_file = (char *)getenv("OPENHPI_UID_MAP");
         if (uid_map_file == NULL) {
                 uid_map_file = OH_DEFAULT_UID_MAP;
         }
         file = open(uid_map_file, O_RDONLY);
         if(file < 0) {
                 /* create map file with resource id initial value */
                 dbg("Configuration file '%s' does not exist, initializing", uid_map_file);
                 file = open(uid_map_file,
			     O_RDWR | O_CREAT | O_TRUNC, 
			     S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );
                 if(file < 0) {
                         dbg("Could not initialize uid map file, %s", uid_map_file );
                                         return -1;
                 }
                 /* write initial uid value */
                 if( write(file,(void *)&resource_id, sizeof(resource_id)) < 0 ) {
                         dbg("failed to write uid, on uid map file initialization");
                         close(file);
                         return -1;
                 }
                 if(close(file) != 0) {
                         dbg("Couldn't close file '%s'.during uid map file initialization", uid_map_file);
                         return -1;
                 }
                 /* return from successful initialization, from newly created uid map file */
                 return 0;
         }

         /* read uid/resouce_id highest count from uid map file */
         if (read(file,&resource_id, sizeof(resource_id)) != sizeof(resource_id)) {
                 dbg("error setting uid from existing uid map file");
                 return -1;
         }

         rval = build_uid_map_data(file);
         close(file);

         if (rval < 0) 
                return -1;
        
         /* return from successful initialization from existing uid map file */
         return 0;
}

/*
 * build_uid_map_data: used by uid_map_from_file(),  recursively 
 * reads map file and builds two hash tables and EP_XREF data
 * structures
 *
 * @file: key into a GHashTable
 *
 * Return value: success 0, error -1.
 */
static gint build_uid_map_data(int file)
{
        int rval;
        EP_XREF *ep_xref;
        EP_XREF ep_xref1;
        gpointer value;
        gpointer key;

        rval = read(file, &ep_xref1, sizeof(EP_XREF));

        while ( (rval != EOF) && (rval == sizeof(EP_XREF)) ) {  

                /* copy read record from ep_xref1 to malloc'd ep_xref */
                ep_xref = (EP_XREF *)g_malloc0(sizeof(EP_XREF));
                if (!ep_xref) 
                        return -1;
                memcpy(ep_xref, &ep_xref1, sizeof(EP_XREF));
 
                value = (gpointer)ep_xref;

                /* entity path based key */   
                key = (gpointer)&ep_xref->entity_path; 
                g_hash_table_insert(ep_hash_table, key, value);

                /* resource id based key */
                key = (gpointer)&ep_xref->resource_id;
                g_hash_table_insert(resource_id_hash_table, key, value);
                
                rval = read(file, &ep_xref1, sizeof(EP_XREF));
        }

        /* TODO thought EOF would return -1, its not so check other way */
        /* if (rval != EOF), rval of 0 seems to be EOF */
        if ( (rval > 0)  && (rval < sizeof(EP_XREF)) ) {
                dbg("error building ep xref from map file");
                return -1;
        }
        return 0;
}

