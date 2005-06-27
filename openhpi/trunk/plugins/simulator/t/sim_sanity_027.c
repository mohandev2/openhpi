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
        SaHpiAnnouncementT announ;
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

        announ.EntryId = SAHPI_FIRST_ENTRY;
        rc = saHpiAnnunciatorGetNext(sid, 1, 1, SAHPI_ALL_SEVERITIES, FALSE,
                                     &announ);
        if (rc != SA_OK) {
		dbg("Couldn't get next annunciator");
		dbg("Error %s",oh_lookup_error(rc));
                return -1;
	}

        rc = saHpiAnnunciatorGetNext(sid, 1, 1, SAHPI_ALL_SEVERITIES, FALSE,
                                     &announ);
        if (rc != SA_OK) {
		dbg("Couldn't get next annunciator");
		dbg("Error %s",oh_lookup_error(rc));
                return -1;
	}

        rc = saHpiAnnunciatorGetNext(sid, 1, 1, SAHPI_ALL_SEVERITIES, FALSE,
                                     &announ);
        if (rc != SA_OK) {
		dbg("Couldn't get next annunciator");
		dbg("Error %s",oh_lookup_error(rc));
                return -1;
	}

        rc = saHpiAnnunciatorGetNext(sid, 1, 1, SAHPI_ALL_SEVERITIES, FALSE,
                                     &announ);
        if (rc == SA_OK) {
		dbg("Invalid number of announcements");
		dbg("Error %s",oh_lookup_error(rc));
                return -1;
	}

	return 0;
}

