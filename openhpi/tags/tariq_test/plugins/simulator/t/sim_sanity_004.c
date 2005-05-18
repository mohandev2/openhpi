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
#include <oh_error.h>

/**
 * Run a series of sanity tests on the simulator
 * Pass on success, otherwise a failure.
**/

/*
 * Utility macro to make it easier to state what failed
*/

#define failed(err)                              \
	do {                                            \
		failcount++;                            \
		dbg("Failed Test %d: %s", testnum, err);    \
	} while(0)

#define runtest() testnum++
 
int main(int argc, char **argv)
{
	SaHpiSessionIdT sid = 0;
	SaHpiRptEntryT res;
	SaHpiRdrT rdr;
	SaHpiEntryIdT id = SAHPI_FIRST_ENTRY;
	SaHpiResourceIdT resid;
	int failcount = 0;
	int testnum = 0;
	SaErrorT rc = SA_OK;

        rc = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID, &sid, NULL);
	if(rc != SA_OK) {
		failed("Failed to open session");
	}

	rc = saHpiDiscover(sid);
	if(rc != SA_OK) {
		failed("Failed to run discover");
	}
						
	
        rc = saHpiRptEntryGet(sid, id, &id, &res);
        runtest();
	if (rc == SA_OK){
		resid = res.ResourceId;
	}
        if(rc != SA_OK) {
		dbg("Error %s",oh_lookup_error(rc));
		failed("Couldn't get the first rpt entry");
		/* we're toast, bail */
		goto end;
	}
	else{
		id = SAHPI_FIRST_ENTRY;
		int i, j = 0;
		rc = saHpiRdrGet(sid, resid, id, &id, &rdr);
		while(rc == SA_OK && id != SAHPI_LAST_ENTRY){
			int x = i;
			int i = rdr.RdrTypeUnion.SensorRec.Num;
		       	if(j == 1){
				if (i != x++){
					dbg("Error %s", oh_lookup_error(rc));
					failed("Num values do not increment correctly");
					goto end;
				}
			}
			rc = saHpiRdrGet(sid, resid, id, &id, &rdr);
		}
			j = 1;
		runtest();
		if(rc != SA_OK){
			dbg("Error %s", oh_lookup_error(rc));
			failed("Couldn't get the first rdr entry");
			goto end;
		}
	}
	
	dbg("Ran %d tests", testnum);
	/* if there is any failures, the test fails */
	end:
		if(failcount) {
			return -1;
		}

	return(0);
}

