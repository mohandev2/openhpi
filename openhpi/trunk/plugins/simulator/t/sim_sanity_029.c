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

        rc = saHpiResourcePowerStateSet(sid, 1, SAHPI_POWER_ON);
        if (rc != SA_OK) {
		dbg("Couldn't set power state");
		dbg("Error %s",oh_lookup_error(rc));
                return -1;
	}

        rc = saHpiResourcePowerStateSet(sid, 1, SAHPI_POWER_OFF);
        if (rc == SA_OK) {
		dbg("Able to set invalid power state");
		dbg("Error %s",oh_lookup_error(rc));
                return -1;
	}

	return 0;
}

