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
        SaHpiIdrAreaHeaderT header;
        SaHpiEntryIdT next;
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

        rc = saHpiIdrAreaHeaderGet(sid, 1, 1, SAHPI_IDR_AREATYPE_CHASSIS_INFO,
                                   SAHPI_FIRST_ENTRY, &next, &header);
        if (rc != SA_OK) {
		dbg("Couldn't get first area header");
		dbg("Error %s",oh_lookup_error(rc));
                return -1;
	}

        rc = saHpiIdrAreaHeaderGet(sid, 1, 1, SAHPI_IDR_AREATYPE_CHASSIS_INFO,
                                   next, &next, &header);
        if (rc == SA_OK) {
		dbg("Invalid area header returned");
                return -1;
	}

	return 0;
}

