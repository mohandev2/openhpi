/*      -*- linux-c -*-
 *
 *(C) Copyright IBM Corp. 2005
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 *	Authors:
 *     	Sean Dague <http://dague.net/sean>
*/

#include <stdlib.h>
#include <SaHpi.h>
#include <oh_utils.h>
#include <sahpi_struct_utils.h>
#include <oh_error.h>


/**
 * Run a series of sanity tests on the simulator
 * Return 0 on success, otherwise return -1
 **/

int main(int argc, char **argv)
{
	SaHpiSessionIdT sid = 0;
	SaHpiEventT entry;
	SaErrorT rc = SA_OK;

        rc = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID, &sid, NULL);
	if (rc != SA_OK) {
		dbg("Failed to open session");
                return -1;
	}

	rc = saHpiDiscover(sid);
	if (rc != SA_OK) {
		dbg("Failed to run discover");
                return -1;
	}

        /* initialize the new event log entry */
        entry.Source = SAHPI_UNSPECIFIED_RESOURCE_ID;
        entry.EventType = SAHPI_ET_USER;
        entry.Timestamp = 0;
        entry.Severity = SAHPI_INFORMATIONAL;
        oh_init_textbuffer(&entry.EventDataUnion.UserEvent.UserEventData);
        oh_append_textbuffer(&entry.EventDataUnion.UserEvent.UserEventData,
                             "My user data");

        /* add event log entry */
        rc = saHpiEventLogEntryAdd(sid, 1, &entry);
        if (rc != SA_OK) {
		dbg("Couldn't add event log entry");
		dbg("Error %s",oh_lookup_error(rc));
                return -1;
	}

	return 0;
}

