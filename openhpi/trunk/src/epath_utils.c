/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2003
 * Copyright (c) 2003 by Intel Corp.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Authors:
 *      Steve Sherman <stevees@us.ibm.com>
 *      Renier Morales <renierm@users.sf.net>
 */

/******************************************************************************
 * DESCRIPTION:
 * Module contains functions to convert between HPI's SaHpiEntityPathT
 * structure and an OpenHPI canonical string. The canonical string is formed
 * by removing the "SAHPI_ENT_" prefix from the HPI types, and creating 
 * tuples for the entity types. Order of significance is inverted to make 
 * entity paths look more like Unix directory structure. It is also assumed 
 * that {ROOT,0} exists implicitly before all of these entries. For example:
 *
 * {SYSTEM_CHASSIS,2}{PROCESSOR_BOARD,0}
 *
 * FUNCTIONS:
 * string2entitypath - Coverts canonical entity path string to HPI entity path
 * entitypath2string - Coverts HPI entity path to canonical entity path string
 * ep_concat         - Concatenates two SaHpiEntityPathT together.
 *
 * NOTES:
 *   - Don't have a good way to identify last real value in SaHpiEntityPath
 *     array, since UNSPECIFIED = 0 and Instance = 0 is a valid path
 *   - Duplicate names in SaHPIEntityTypeT enumeration aren't handled
 *     Names below won't be preserved across conversion calls:
 *       - IPMI_GROUP              - IPMI_GROUP + 0x90
 *       - IPMI_GROUP + 0xB0       - IPMI_GROUP + 0xD0
 *       - ROOT_VALUE              - SAFHPI_GROUP
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SaHpi.h>
#include <openhpi.h>
#include <epath_utils.h>

static unsigned int index2entitytype(unsigned int i);
static unsigned int entitytype2index(unsigned int i);

static gchar *eshort_names[] = {
	"UNSPECIFIED",
	"OTHER",
	"UNKNOWN",
	"PROCESSOR",
	"DISK_BAY",
	"PERIPHERAL_BAY",
	"SYS_MGMNT_MODULE",
	"SYSTEM_BOARD",
	"MEMORY_MODULE",
	"PROCESSOR_MODULE",
	"POWER_SUPPLY",
	"ADD_IN_CARD",
	"FRONT_PANEL_BOARD",
	"BACK_PANEL_BOARD",
	"POWER_SYSTEM_BOARD",
	"DRIVE_BACKPLANE",
	"SYS_EXPANSION_BOARD",
	"OTHER_SYSTEM_BOARD",
	"PROCESSOR_BOARD",
	"POWER_UNIT",
	"POWER_MODULE",
	"POWER_MGMNT",
	"CHASSIS_BACK_PANEL_BOARD",
	"SYSTEM_CHASSIS",
	"SUB_CHASSIS",
	"OTHER_CHASSIS_BOARD",
	"DISK_DRIVE_BAY",
	"PERIPHERAL_BAY_2",
	"DEVICE_BAY",
	"COOLING_DEVICE",
	"COOLING_UNIT",
	"INTERCONNECT",
	"MEMORY_DEVICE",
	"SYS_MGMNT_SOFTWARE",
	"BIOS",
	"OPERATING_SYSTEM",
	"SYSTEM_BUS",
	"GROUP",
	"REMOTE",
	"EXTERNAL_ENVIRONMENT",
	"BATTERY",
	"CHASSIS_SPECIFIC",     /* Jumps to 144 */
	"BOARD_SET_SPECIFIC",   /* Jumps to 176 */
	"OEM_SYSINT_SPECIFIC",  /* Jumps to 208 */
	"ROOT",                 /* Jumps to 65535 and continues from there... */
	"RACK",
	"SUBRACK",
	"COMPACTPCI_CHASSIS",
	"ADVANCEDTCA_CHASSIS",
	"SYSTEM_SLOT",
	"SBC_BLADE",
	"IO_BLADE",
	"DISK_BLADE",
	"DISK_DRIVE",
	"FAN",
	"POWER_DISTRIBUTION_UNIT",
	"SPEC_PROC_BLADE",
	"IO_SUBBOARD",
	"SBC_SUBBOARD",
	"ALARM_MANAGER",
	"ALARM_MANAGER_BLADE",
	"SUBBOARD_CARRIER_BLADE",
};

#define ESHORTNAMES_ARRAY_SIZE 61
#define ELEMENTS_IN_SaHpiEntityT 2
#define EPATHSTRING_START_DELIMITER "{"
#define EPATHSTRING_END_DELIMITER "}"
#define EPATHSTRING_VALUE_DELIMITER ","
#define ESHORTNAMES_BEFORE_JUMP 40
#define ESHORTNAMES_FIRST_JUMP 41
#define ESHORTNAMES_SECOND_JUMP 42
#define ESHORTNAMES_THIRD_JUMP 43
#define ESHORTNAMES_LAST_JUMP 44

/***********************************************************************
 * string2entitypath
 * 
 * Parameters:
 *   epathstr  IN   Pointer to canonical entity path string
 *   epathptr  OUT  Pointer to HPI's entity path structure
 *
 * Returns:
 *    0  Successful return
 *   -1  Error return
 ***********************************************************************/
int string2entitypath(const gchar *epathstr, SaHpiEntityPathT *epathptr)
{

	gchar  **epathdefs = NULL, **epathvalues = NULL;
	gchar  *gstr = NULL, *etype = NULL, *einstance = NULL, *endptr;
        gint rtncode = 0;
	guint   i, j, match, instance, num_valid_entities = 0;
        GSList *epath_list = NULL, *lst = NULL;

	SaHpiEntityT  *entityptr = NULL;

	if (epathstr == NULL) {
		dbg("Input entity path string is NULL"); return(-1);
	}

	/* Split out {xxx,yyy} definition pairs */
	gstr = g_strdup(epathstr);
	epathdefs = g_strsplit(g_strstrip(gstr), EPATHSTRING_END_DELIMITER, -1);
	if (*epathdefs == NULL) { 
		dbg("Stripped entity path string is NULL"); 
		rtncode = -1; goto CLEANUP;
        }

	/* Split out entity type and instance strings; 
	   Convert to HPI structure types */
	for (i = 0; epathdefs[i] != NULL; i++) {
		epathvalues = g_strsplit(g_strstrip(epathdefs[i]), EPATHSTRING_VALUE_DELIMITER, ELEMENTS_IN_SaHpiEntityT);
		epathvalues[0] = g_strdelimit(epathvalues[0], EPATHSTRING_START_DELIMITER, ' ');

		etype = g_strstrip(epathvalues[0]);
		einstance = g_strstrip(epathvalues[1]);

		instance = strtol(einstance, &endptr, 10);
		if (*endptr != '\0') { 
			dbg("Invalid instance character"); 
			rtncode = -1; goto CLEANUP; }

		match = 0;
		for (j=0; j < ESHORTNAMES_ARRAY_SIZE + 1; j++) {
			if (!strcmp(eshort_names[j], etype)) {
				match = 1;
				break;
			}
		}
		if (!match) { 
			dbg("Invalid entity type string"); 
			rtncode = -1; goto CLEANUP;
		}

		/* Save entity path definitions; reverse order */
		if (num_valid_entities < SAHPI_MAX_ENTITY_PATH) {
			entityptr = (SaHpiEntityT *)g_malloc0(sizeof(*entityptr));
			if (entityptr == NULL) { 
				dbg("Out of memory"); 
				rtncode = -1; goto CLEANUP;
			}

			entityptr->EntityType = index2entitytype(j);
			entityptr->EntityInstance = instance;
			epath_list = g_slist_prepend(epath_list, (gpointer)entityptr);
		}

		num_valid_entities++; 
	}  
  
	/* Clear and write HPI entity path structure */
	memset(epathptr, 0, SAHPI_MAX_ENTITY_PATH * sizeof(*entityptr));

	i = 0;
	g_slist_for_each(lst, epath_list) {
		epathptr->Entry[i].EntityType = 
			((SaHpiEntityT *)(lst->data))->EntityType;
		epathptr->Entry[i].EntityInstance = 
			((SaHpiEntityT *)(lst->data))->EntityInstance;

		g_free(lst->data); /* Is this necessary ??? */
		epath_list = g_slist_remove(epath_list, lst);
		i++;
	}

	if (num_valid_entities > SAHPI_MAX_ENTITY_PATH) { 
		dbg("Too many entity defs"); rtncode = -1;
	}
   
 CLEANUP:
	g_free(gstr);
	g_strfreev(epathdefs);
	g_strfreev(epathvalues);
	g_slist_free(epath_list);

	return(rtncode);
} /* End string2entitypath */

/***********************************************************************
 * entitypath2string
 *  
 * Parameters:
 *   epathptr IN   Pointer to HPI's entity path structure
 *   epathstr OUT  Pointer to canonical entity path string
 *   strsize  IN   Canonical string length
 *
 * Returns:
 *   >0  Number of characters written to canonical entity path string
 *   -1  Error return
 ***********************************************************************/
int entitypath2string(const SaHpiEntityPathT *epathptr, gchar *epathstr, const gint strsize)
{
     
	gchar  *instance_str, *catstr, *tmpstr;
	gint   err, i, strcount = 0, rtncode = 0;

	if (epathstr == NULL || strsize <= 0) { 
		dbg("Null string or invalid string size"); return(-1);
	}

        if (epathptr == NULL) {
                epathstr = "";
                return 0;
        }

	instance_str = (gchar *)g_malloc0(MAX_INSTANCE_DIGITS + 1);
	tmpstr = (gchar *)g_malloc0(strsize);
	if (instance_str == NULL) { 
		dbg("Out of memory"); 
		rtncode = -1; goto CLEANUP;
	}
	if (tmpstr == NULL) { 
		dbg("Out of memory"); 
		rtncode = -1; goto CLEANUP;
	}
	
	for (i = (SAHPI_MAX_ENTITY_PATH - 1); i >= 0; i--) {
                guint num_digits, work_instance_num;

		/* Find last element of structure; Current choice not good,
		   since type=instance=0 is valid */
		if (epathptr->Entry[i].EntityType == 0){ continue; }
  
		/* Validate and convert data */
                work_instance_num = epathptr->Entry[i].EntityInstance;
                for (num_digits = 1; (work_instance_num = work_instance_num/10) > 0; num_digits++);
		
		if (num_digits > MAX_INSTANCE_DIGITS) { 
                        g_free(instance_str); 
			dbg("Instance value too big");
                        rtncode = -1; goto CLEANUP;
		}

                err = snprintf(instance_str, MAX_INSTANCE_DIGITS + 1,
                               "%d", epathptr->Entry[i].EntityInstance);

		strcount = strcount + 
			strlen(EPATHSTRING_START_DELIMITER) + 
			strlen(eshort_names[entitytype2index(epathptr->Entry[i].EntityType)]) + 
			strlen(EPATHSTRING_VALUE_DELIMITER) + 
			strlen(instance_str) + 
			strlen(EPATHSTRING_END_DELIMITER);

		if (strcount >= strsize - 1) {
			dbg("Not enough space allocated for string"); 
			rtncode = -1; goto CLEANUP;
		}

		catstr = g_strconcat(EPATHSTRING_START_DELIMITER,
				     eshort_names[entitytype2index(epathptr->Entry[i].EntityType)],
				     EPATHSTRING_VALUE_DELIMITER,
				     instance_str, 
				     EPATHSTRING_END_DELIMITER, 
				     NULL);

		strcat(tmpstr, catstr);
		g_free(catstr); 
	}
        rtncode = strcount;
	/* Write string */
	memset(epathstr, 0 , strsize);
	strcpy(epathstr, tmpstr);

 CLEANUP:
	g_free(instance_str);
	g_free(tmpstr);

	return(rtncode);
} /* End entitypath2string */


/**
 * ep_concat: Concatenate two entity path structures (SaHpiEntityPathT).
 * @dest: IN,OUT Left-hand entity path. Gets appended with 'append'.
 * @append: IN Right-hand entity path. Pointer entity path to be appended.
 *
 * 'dest' is assumed to be the least significant entity path in the operation.
 *
 * Returns value: 0 on Success, Negative number on Failure.
 **/
int ep_concat(SaHpiEntityPathT *dest, const SaHpiEntityPathT *append)
{
        unsigned int i, j;

        if(!dest) return -1;
        if(!append) return 0;

        for(i = 0; i < SAHPI_MAX_ENTITY_PATH; i++) {
                if(dest->Entry[i].EntityType == 0) {
                        break;
                }
        }

        for (j = 0; i < SAHPI_MAX_ENTITY_PATH; i++) {
                if(append->Entry[j].EntityType == 0) break;
                dest->Entry[i].EntityInstance = append->Entry[j].EntityInstance;
                dest->Entry[i].EntityType = append->Entry[j].EntityType;
                j++;
        }

        return 0;
}

int set_epath_instance(struct oh_event *e, SaHpiEntityTypeT et, SaHpiEntityInstanceT ei)
{
        int i;
        int retval = -1;

        for(i = 0; i < SAHPI_MAX_ENTITY_PATH; i++) {
                if(e->u.res_event.entry.ResourceEntity.Entry[i].EntityType == et) {
                        e->u.res_event.entry.ResourceEntity.Entry[i].EntityInstance = ei;
                        retval = 0;
                        break;
                }
        }
        return retval;
}

/**
 * append_root: Append the SAHPI_ENT_ROOT element to an entity path structure.
 * @ep: IN,OUT Pointer to entity path. SAHPI_ENT_ROOT will be appended to it.
 *
 * If an entity path already has a root element, the function returns without
 * making any changes and reporting success.
 *
 * Returns value: 0 on Success, Negative number on Failure.
 **/
int append_root(SaHpiEntityPathT *ep)
{
        unsigned int i;
        int rc = -1;

        for(i = 0; i < SAHPI_MAX_ENTITY_PATH; i++) {
                if (ep->Entry[i].EntityType == 0 &&
                    ep->Entry[i-1].EntityType != SAHPI_ENT_ROOT) {
                        ep->Entry[i].EntityType = SAHPI_ENT_ROOT;
                        ep->Entry[i].EntityInstance = 0;
                        rc = 0;
                        break;
                } else if (ep->Entry[i].EntityType == 0 &&
                           ep->Entry[i-1].EntityType == SAHPI_ENT_ROOT) {
                        rc = 0;
                        break;
                }
        }

        return rc;
}

static unsigned int index2entitytype(unsigned int i)
{
        if(i <= ESHORTNAMES_BEFORE_JUMP) {
                return i;
        } else if(i == ESHORTNAMES_FIRST_JUMP) {
                return (unsigned int)SAHPI_ENT_CHASSIS_SPECIFIC;
        } else if(i == ESHORTNAMES_SECOND_JUMP) {
                return (unsigned int)SAHPI_ENT_BOARD_SET_SPECIFIC;
        } else if(i == ESHORTNAMES_THIRD_JUMP) {
                return (unsigned int)SAHPI_ENT_OEM_SYSINT_SPECIFIC;
        } else {
                return (i - ESHORTNAMES_LAST_JUMP + (unsigned int)SAHPI_ENT_ROOT);
        }
}

static unsigned int entitytype2index(unsigned int i)
{
        if(i <= ESHORTNAMES_BEFORE_JUMP) {
                return i;
        } else if (i == (unsigned int)SAHPI_ENT_CHASSIS_SPECIFIC) {
                return ESHORTNAMES_FIRST_JUMP;
        } else if (i == (unsigned int)SAHPI_ENT_BOARD_SET_SPECIFIC) {
                return ESHORTNAMES_SECOND_JUMP;
        } else if (i == (unsigned int)SAHPI_ENT_OEM_SYSINT_SPECIFIC) {
                return ESHORTNAMES_THIRD_JUMP;
        } else {
                return (i - (unsigned int)SAHPI_ENT_ROOT + ESHORTNAMES_LAST_JUMP);
        }
}
