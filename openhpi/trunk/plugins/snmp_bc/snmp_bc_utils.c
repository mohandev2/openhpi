/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2003, 2005
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *      Chris Chia <cchia@users.sf.net>
 */

#include <uuid/uuid.h>
#include <snmp_bc_plugin.h>
#include <snmp_bc_utils.h>

SaErrorT snmp_bc_get_guid(struct snmp_bc_hnd *custom_handle,
			  struct oh_event *e,
			  struct ResourceInfo *res_info_ptr)
{
        SaErrorT status;
	SaHpiGuidT guid;
        struct  snmp_value get_value;
        gchar  *UUID = NULL, *BC_UUID = NULL;
        gchar **tmpstr = NULL;
        const   gchar  *UUID_delimiter1 = " ";
        const   gchar  *UUID_delimiter2 = "-";
        const   gchar  *UUID_delimiter    = "-";
        const   gchar  *NA   = "NOT AVAILABLE";    /* not case sensitive */
        guint   UUID_cnt = 0, i = 0;
        uuid_t  UUID_val;

        if ( (custom_handle == NULL) || (e == NULL) || (res_info_ptr == NULL)) {
                dbg("Invalid parameter.");
                status = SA_ERR_HPI_INVALID_PARAMS;
                goto CLEANUP2;
        }
	
	memset(&guid, 0, sizeof(SaHpiGuidT));  /* default to zero */
	if (res_info_ptr->mib.OidUuid == NULL) {
		trace("NULL UUID OID");
		status = SA_OK;
		goto CLEANUP;
	}
        status = snmp_bc_oid_snmp_get(custom_handle, 
					&(e->u.res_event.entry.ResourceEntity),
					res_info_ptr->mib.OidUuid,            
					&get_value, SAHPI_TRUE);
        if(( status != SA_OK) || (get_value.type != ASN_OCTET_STR)) {
                dbg("Cannot get OID rc=%d; oid=%s type=%d.", 
                        status, res_info_ptr->mib.OidUuid, get_value.type);
                if ( status != SA_ERR_HPI_BUSY)  status = SA_ERR_HPI_NO_RESPONSE;
                goto CLEANUP;
        }

        trace("UUID=%s.", get_value.string);
        /* rid lead+trail blanks */
        BC_UUID = g_strstrip(g_strdup(get_value.string));
        if (BC_UUID == NULL || BC_UUID[0] == '\0') {
                dbg("UUID is NULL.");
                status = SA_ERR_HPI_ERROR;
                goto CLEANUP;
        }
        if (g_ascii_strcasecmp( BC_UUID, NA ) == 0) {
                trace("UUID is N/A %s, set GUID to zeros.", BC_UUID);
                for ( i=0; i<16; i++ ) UUID_val[i] = 0;
                memmove ( guid, &UUID_val, sizeof(uuid_t));
                status = SA_OK;
                goto CLEANUP;
        }
        /* separate substrings */
        tmpstr = g_strsplit(BC_UUID, UUID_delimiter1, -1);
        for ( UUID_cnt=0; tmpstr[UUID_cnt] != NULL; UUID_cnt++ );
        /* trace("number of UUID substrings = %d, strings =", UUID_cnt); */
        /* for (i=0; i<UUID_cnt; i++) trace(" %s", tmpstr[i]); trace("\n"); */
        if ( UUID_cnt == 0 ) {
                dbg("Zero length UUID string.");
                status = SA_ERR_HPI_ERROR;
                goto CLEANUP;
        }
        if ( UUID_cnt == 1 ) { /* check with second possible substring delimiter */
                tmpstr = g_strsplit(BC_UUID, UUID_delimiter2, -1);
                for ( UUID_cnt=0; ; UUID_cnt++ ) {
                        if ( tmpstr[UUID_cnt] == NULL ) break;
                }
                /* trace("Number of UUID substrings = %d, strings =", UUID_cnt); */
                /* for (i=0; i<UUID_cnt; i++) trace(" %s", tmpstr[i]); trace("\n"); */
                if ( UUID_cnt == 0 ) {
                        dbg("Zero length UUID string.");
                        status = SA_ERR_HPI_ERROR;
                        goto CLEANUP;
                }
        }
        if ( UUID_cnt == UUID_SUBSTRINGS_CNT1 ) {
                /* UUID has 8 four character strings 4-4-4-4-4-4-4-4
                 * convert to industry standard UUID 8-4-4-4-12 string */
                UUID = g_strconcat( tmpstr[0], tmpstr[1], UUID_delimiter,   
                                    tmpstr[2], UUID_delimiter,             
                                    tmpstr[3], UUID_delimiter,            
                                    tmpstr[4], UUID_delimiter,           
                                    tmpstr[5], tmpstr[6], tmpstr[7], NULL );
                if (UUID == NULL) {
                        dbg("Bad UUID string.");
                        status = SA_ERR_HPI_ERROR;
                        goto CLEANUP;
                }
                trace("UUID string %s", UUID);
                /* convert UUID string to numeric UUID value */
                if ( (status = uuid_parse(UUID, UUID_val)) ) {
                        dbg("Cannot parse UUID string err=%d.", status);
                        status = SA_ERR_HPI_ERROR;
                        goto CLEANUP;
                }       
                /* trace("GUID value  "); */
                /* for (i=0; i<16; i++) { trace("%02x", UUID_val[i]);} trace("\n"); */
                memmove ( guid, &UUID_val, sizeof(uuid_t));
                status = SA_OK;
        }
        else if ( UUID_cnt == UUID_SUBSTRINGS_CNT2 ) {
                /* Got a 5 substring case, just put in the delimiter */
                UUID = g_strconcat( tmpstr[0], UUID_delimiter,   
                                    tmpstr[1], UUID_delimiter,             
                                    tmpstr[2], UUID_delimiter,            
                                    tmpstr[3], UUID_delimiter,           
                                    tmpstr[4], NULL );
                if (UUID == NULL) {
                        dbg("Bad UUID string.");
                        status = SA_ERR_HPI_ERROR;
                        goto CLEANUP;
                }
                trace("UUID=%s", UUID);
                /* Convert UUID string to numeric UUID value */
                if ( (status = uuid_parse(UUID, UUID_val)) ) {
                        dbg("Cannot parse UUID string. err=%d.", status);
                        status = SA_ERR_HPI_ERROR;
                        goto CLEANUP;
                }       
                /* trace("GUID value  "); */
                /* for (i=0; i<16; i++) { trace("%02x", UUID_val[i]);} trace("\n"); */
                memmove ( guid, &UUID_val, sizeof(uuid_t));
                status = SA_OK;
        }
        else {  /* Non standard case unsupported */
                dbg("Non standard UUID string.");
                status = SA_ERR_HPI_ERROR;
        }

  CLEANUP:
  	memmove(e->u.res_event.entry.ResourceInfo.Guid, guid, sizeof(SaHpiGuidT));
  CLEANUP2:
        g_free(UUID);
        g_free(BC_UUID);
        g_strfreev(tmpstr);
                                                                                             
        /* trace("get_guid exit status %d.", status); */
        return(status);
}
