/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2003, 2004
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *      Sean Dague <http://dague.net/sean>
 */

#include <glib.h>
#include <time.h>

#include <snmp_bc_plugin.h>

oh_sel *bc_selcache = NULL;

/**
 * get_bc_sel_size:
 * @hnd: 
 * @id: 
 * 
 * Return value: Number of event log entries
 **/
static int get_bc_sel_size(void *hnd, SaHpiResourceIdT id)
{

        int i = 1;
        struct oh_handler_state *handle = hnd;
	
	/*
	 * Go synchronize cache and hardware copy of the SEL
	*/
        snmp_bc_check_selcache(hnd, id, SAHPI_NEWEST_ENTRY);
	
	/* Return the entry count    */
        i = g_list_length(handle->selcache->selentries);
        return i;

}

/**
 * get_bc_sel_size_from_hardware:
 * @ss: 
 * 
 * 
 * Return value:  Number of event log entries read from Blade Center 
 **/
static int get_bc_sel_size_from_hardware(struct snmp_session *ss)
{
        struct snmp_value run_value;
        char oid[50];
        int i = 1;

        /*
          Since Blade Center snmp agent does not provide this count,
          this is the only way to figure out how entries there are
	  in Blade Center SEL.
        */
        do {
                sprintf(oid, "%s.%d", BC_SEL_INDEX_OID, i);
                i++;
        } while(snmp_get(ss,oid,&run_value) == 0);
        
        /* think about it, and it makes sense */
        i -= 2;
        return i;
}

/**
 * snmp_bc_get_sel_entry:
 * @hnd: 
 * @id: 
 * @info: 
 * 
 * Return value: 0 on success, < 0 on error
 **/
int snmp_bc_get_sel_info(void *hnd, SaHpiResourceIdT id, SaHpiEventLogInfoT *info) 
{
        struct snmp_value first_value;
        struct oh_handler_state *handle = hnd;
        struct snmp_bc_hnd *custom_handle = (struct snmp_bc_hnd *)handle->data;
        bc_sel_entry sel_entry;
        struct tm curtime;
        char oid[50];
        int i = 1;
        
        SaHpiEventLogInfoT sel = {
                .Size = 512, /* this is clearly a guess but looks about right 
                              * from the 75% full errors I've seen */
                .Enabled = SAHPI_TRUE,
                .OverflowFlag = SAHPI_FALSE,
                .OverflowAction = SAHPI_SEL_OVERFLOW_DROP,
                .DeleteEntrySupported = SAHPI_FALSE
        };
        
        sprintf(oid, "%s.%d", BC_SEL_ENTRY_OID, i);
        /* we need first value to figure out what our update time is */
        snmp_get(custom_handle->ss,oid,&first_value);
        
        if(first_value.type == ASN_OCTET_STR) {
                if(snmp_bc_parse_sel_entry(handle,first_value.string, &sel_entry) < 0) {
                        dbg("Couldn't get first date");
                } else {
                        sel.UpdateTimestamp = 
                                (SaHpiTimeT) mktime(&sel_entry.time) * 1000000000;
                }
        }
        
        if(get_bc_sp_time(handle,&curtime) == 0) {
                sel.CurrentTime = 
                        (SaHpiTimeT) mktime(&curtime) * 1000000000;
        }
        
        sel.Entries = get_bc_sel_size(hnd, id);
        *info = sel;
        
        return 0;
}

/**
 * snmp_bc_get_sel_entry:
 * @hnd: 
 * @id: 
 * @current: 
 * @prev: 
 * @next: 
 * @entry: 
 * 
 * See saHpiEventLogEntryGet for params
 * 
 * Return value: 0 on success, < 0 on error
 **/
int snmp_bc_get_sel_entry(void *hnd, SaHpiResourceIdT id, SaHpiEventLogEntryIdT current,
                          SaHpiEventLogEntryIdT *prev, SaHpiEventLogEntryIdT *next,
                          SaHpiEventLogEntryT *entry)
{
        SaHpiEventLogEntryT tmpentry, *tmpentryptr;
        struct oh_handler_state *handle = (struct oh_handler_state *)hnd;
	tmpentryptr = &tmpentry; 
	SaErrorT rc;

	rc = snmp_bc_check_selcache(hnd, id, current);
	if (handle->selcache != NULL) {
		rc = oh_sel_get(handle->selcache, current, prev, next, &tmpentryptr);
		if (rc != SA_OK) {
			dbg("Error fetching entry number %d from cache, errnum %d  >>>\n", current, rc);
		} else {
			memcpy(entry, tmpentryptr, sizeof(SaHpiEventLogEntryT));
		}
	} else {
		dbg("Missing handle->selcache");
	}
		
        return rc;
}

/**
 * snmp_bc_build_selcache
 * @hnd: 
 * @id: 
 * 
 * 
 * Return value: 0 on success, < 0 on error
 **/
SaErrorT snmp_bc_build_selcache(void *hnd, SaHpiResourceIdT id)
{
	int current;
	SaErrorT rv;
	struct oh_handler_state *handle = hnd;
	struct snmp_bc_hnd *custom_handle = handle->data;
	
	current = get_bc_sel_size_from_hardware(custom_handle->ss);
	if (current != 0) {
		do {
			rv = snmp_bc_sel_read_add (hnd, id, current); 
			current--;
		} while(current > 0);
	}
	return SA_OK;
}

/**
 * snmp_bc_check_selcache
 * @hnd: 
 * @id: 
 * @entryId: 
 * 
 * 
 * Return value: 0 on success, < 0 on error
 **/
int snmp_bc_check_selcache(void *hnd, SaHpiResourceIdT id, SaHpiEventLogEntryIdT entryId)
{
        struct oh_handler_state *handle = hnd;
	SaErrorT rv;

	if (g_list_length(handle->selcache->selentries) == 0) {
		rv = snmp_bc_build_selcache(hnd, id);
	} else {
		rv = snmp_bc_selcache_sync(hnd, id, entryId);
	}
	return rv;
}

/**
 * snmp_bc_selcache_sync
 * @hnd: 
 * @id: 
 * @entryId: 
 * 
 * 
 * Return value: 0 on success, < 0 on error
 **/
int snmp_bc_selcache_sync(void *hnd, SaHpiResourceIdT id, SaHpiEventLogEntryIdT entryId)
{
	SaHpiEventLogEntryIdT current;
	SaHpiEventLogEntryIdT prev;
	SaHpiEventLogEntryIdT next;
        struct snmp_value get_value;
        struct oh_handler_state *handle = hnd;
        struct snmp_bc_hnd *custom_handle = handle->data;
        bc_sel_entry sel_entry;
        SaHpiEventLogEntryT  *fetchentry;
        SaHpiTimeT new_timestamp;
	char oid[50];
	SaErrorT rv;
	int rc, cacheupdate = 0;

	rv = oh_sel_get(handle->selcache, SAHPI_NEWEST_ENTRY, &prev, &next, &fetchentry);
	if (rv != SA_OK)
		fetchentry = NULL;
		
	current = 1;
	sprintf(oid, "%s.%d", BC_SEL_ENTRY_OID, current);
       	rc = snmp_get(custom_handle->ss,oid,&get_value);
       	if (rc != 0) 
	{
		/* 
		 * snmp_get() returns -1 if snmp agent does not respond *or*
		 * snmp log entry does not exist. 
		 *
		 * For now, assuming the best i.e. snmp log is empty. 
		 */
		dbg("snmp log is empty.\n");
		rv = oh_sel_clear(handle->selcache);
	
	} else {
		if (fetchentry == NULL) {
			rv = snmp_bc_build_selcache(hnd, id);
			return SA_OK;
		}
		
		if(snmp_bc_parse_sel_entry(handle,get_value.string, &sel_entry) < 0) {
			dbg("Couldn't parse SEL Entry");
       			return -1;
		}

		new_timestamp = (SaHpiTimeT) mktime(&sel_entry.time) * 1000000000;
		if (fetchentry->Event.Timestamp != new_timestamp)
		{
			while (1) {
				current++;
               			sprintf(oid, "%s.%d", BC_SEL_ENTRY_OID, current);
               			rv = snmp_get(custom_handle->ss,oid,&get_value);
				if (rv == 0) {
               				if(snmp_bc_parse_sel_entry(handle,get_value.string, &sel_entry) < 0) {
               					dbg("Couldn't parse SEL Entry");
                       				return -1;
					}

					if ((fetchentry->Event.Timestamp == 
						 (SaHpiTimeT) mktime(&sel_entry.time) * 1000000000)) 
					{
						current--;
						cacheupdate = 1;	
						break;
					}
               			} else {
					dbg("Old entry not found and end of snmp log reached.\n");
					break;
				}
				
			}

			if (cacheupdate) {
	        		do {
					rv = snmp_bc_sel_read_add (hnd, id, current); 
                       			current--;
               			} while(current > 0);
			} else {
				rv = oh_sel_clear(handle->selcache);
				snmp_bc_build_selcache(hnd, id);				
			}
		} else {
			; /* dbg("There are no new entry indicated.\n"); */
		}
	}
	return SA_OK;  
}

/**
 * snmp_bc_set_sel_time:
 * @hnd: 
 * @id: 
 * @time: 
 * 
 * 
 * 
 * Return value: 
 **/
int snmp_bc_set_sel_time(void *hnd, SaHpiResourceIdT id, SaHpiTimeT time)
{
        struct oh_handler_state *handle = hnd;
        struct snmp_bc_hnd *custom_handle = (struct snmp_bc_hnd *)handle->data;
        struct tm tv;
        time_t tt;
        SaErrorT returncode;

        tt = time / 1000000000;
        
        localtime_r(&tt, &tv);

        if (set_bc_sp_time(custom_handle->ss,&tv) == 0)
                returncode = SA_OK;
        else
                returncode = SA_ERR_HPI_ERROR;
                 
        return returncode;
}

/**
 * snmp_bc_add_sel_entry:
 * @hnd: 
 * @id: 
 * @Event: 
 * 
 * Add is not supported with tihs hardware, so -1 is always returned
 * 
 * Return value: -1
 **/
int snmp_bc_add_sel_entry(void *hnd, SaHpiResourceIdT id, const SaHpiEventT *Event)
{
        return SA_ERR_HPI_INVALID_CMD;
}

/**
 * snmp_bc_sel_read_add:
 * @hnd: 
 * @id: 
 * @current: 
 * 
 * 
 * Return value: -1 if fails. 0 SA_OK
 **/
int snmp_bc_sel_read_add (void *hnd, SaHpiResourceIdT id, SaHpiEventLogEntryIdT current)
{
        struct snmp_value get_value;
        struct oh_handler_state *handle = hnd;
        struct snmp_bc_hnd *custom_handle = handle->data;
        SaHpiEventT tmpevent;
        char oid[50];
        SaErrorT rv;
	int isdst = 0;

	sprintf(oid, "%s.%d", BC_SEL_ENTRY_OID, current);
	snmp_get(custom_handle->ss,oid,&get_value);

	if(get_value.type == ASN_OCTET_STR) {
		int event_enabled;

		/*snmp_bc_parse_sel_entry(handle,get_value.string, &sel_entry);*/
		/* isdst = sel_entry.time.tm_isdst;*/
                log2event(hnd, get_value.string, &tmpevent, isdst, &event_enabled);
                handle->selcache->nextId = current;                
                rv = oh_sel_add(handle->selcache, &tmpevent);
		
		if (event_enabled) {
			rv = snmp_bc_add_to_eventq(hnd, &tmpentry.Event);
		}
	} else {
		dbg("Couldn't fetch SEL Entry from BladeCenter snmp");
		return -1;
	}

        return SA_OK;
}

/**
 * snmp_bc_parse_sel_entry:
 * @text: text as returned by snmpget call for an event log entry
 * @sel: blade center system event log
 * 
 * This call is used to create a blade center sel entry from the returned
 * snmp string.  Another transform will have to happen to turn this into 
 * an SAHPI sel entry. 
 * 
 * Return value: 0 for success, -1 for format error, -2 for premature data termination
 **/
int snmp_bc_parse_sel_entry(struct oh_handler_state *handle, char * text, bc_sel_entry * sel) 
{
        bc_sel_entry ent;
        char level[8];
        char * findit;
	
        /* Severity first */
	findit = strstr(text, "Severity:");
	if (findit != NULL) {
        	if(sscanf(findit,"Severity:%7s",level)) {
                	if(strcmp(level,"INFO") == 0) {
                        	ent.sev = SAHPI_INFORMATIONAL;
                	} else if(strcmp(level,"WARN") == 0) {
                        	ent.sev = SAHPI_MINOR;
                	} else if(strcmp(level,"ERR") == 0) {
                        	ent.sev = SAHPI_CRITICAL;
                	} else {
                        	ent.sev = SAHPI_DEBUG;
                	}
        	} else {
                	dbg("Couldn't parse Severity from Blade Center Log Entry");
                	return -1;
        	}
	}
                

	findit = strstr(text, "Source:");
	if (findit != NULL) {
        	if(!sscanf(findit,"Source:%19s",ent.source)) {
                	dbg("Couldn't parse Source from Blade Center Log Entry");
                	return -1;
        	}
	} else 
		return -2;


	findit = strstr(text, "Name:");
	if (findit != NULL) {
        	if(!sscanf(findit,"Name:%19s",ent.sname)) {
                	dbg("Couldn't parse Name from Blade Center Log Entry");
                	return -1;
        	}
	} else 
		return -2;
        
        
	findit = strstr(text, "Date:");
	if (findit != NULL) {
        	if(sscanf(findit,"Date:%2d/%2d/%2d  Time:%2d:%2d:%2d",
                	  &ent.time.tm_mon, &ent.time.tm_mday, &ent.time.tm_year, 
                  	&ent.time.tm_hour, &ent.time.tm_min, &ent.time.tm_sec)) {
			set_bc_dst(handle, &ent.time);
                	ent.time.tm_mon--;
                	ent.time.tm_year += 100;
        	} else {
                	dbg("Couldn't parse Date/Time from Blade Center Log Entry");
                	return -1;
        	}
	} else 
		return -2;
        
        
	findit = strstr(text, "Text:");
	if (findit != NULL) {
        	/* advance to data */
        	findit += 5;
        	strncpy(ent.text,findit,BC_SEL_ENTRY_STRING - 1);
        	ent.text[BC_SEL_ENTRY_STRING - 1] = '\0';
	} else 
		return -2;
        
        *sel = ent;
        return 0;
}

/**
 * snmp_bc_clear_sel:
 * @hnd: 
 * @id: 
 * @info: 
 * 
 * Return value: 0 on success, < 0 on error
 **/
SaErrorT snmp_bc_clear_sel(void *hnd, SaHpiResourceIdT id) 
{
        char oid[50];
	int retcode;
	struct snmp_value set_value;
        struct oh_handler_state *handle = hnd;
        struct snmp_bc_hnd *custom_handle = handle->data;
	SaErrorT rv;
		
	rv = oh_sel_clear(handle->selcache);

	sprintf(oid, "%s", BC_CLEAR_SEL_OID);
	set_value.type = ASN_INTEGER;
	set_value.str_len = 1;
	set_value.integer = (long) clearEventLogExecute;
	retcode = snmp_set(custom_handle->ss, oid, set_value);
	
	if (retcode != 0)
		rv = SA_ERR_HPI_ERROR;
	else 
		rv = SA_OK;
		
	return rv;

}
/* end of snmp_bc_sel.c */
