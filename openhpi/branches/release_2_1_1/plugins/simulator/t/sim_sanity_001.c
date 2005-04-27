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
* Authors:
*     Sean Dague <http://dague.net/sean>
*/

#include <stdlib.h>
#include <SaHpi.h>
#include <oh_utils.h>

 /**
  *  * Run a series of sanity tests on the simulator
  *   * Pass on success, otherwise a failure.
  *    **/

 /**
  *  * Utility macro to make it easier to state what failed
  *   */

#define failed(err)                                \
	do {                                            \
		failcount++;                            \
		dbg("Failed Test %d: %s", testnum, err); 	\
	} while(0)

#define runtest() testnum++

int main(int argc, char **argv)
{
	SaHpiSessionIdT sid = 0;
//	SaHpiRptEntryT res;
//	SaHpiEntryIdT id = SAHPI_FIRST_ENTRY;
	int failcount = 0;
	int testnum = 0;
	SaErrorT rc = SA_OK;

        rc = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID, &sid, NULL);
	runtest();
	if(rc != SA_OK) {
		failed("Failed to open session");
	}
			
	
	rc = saHpiDiscover(sid);
	runtest();
	if(rc != SA_OK) {
		failed("Failed to run discover");
	}

        dbg("Ran %d tests", testnum);
        /* if there is any failures, the test fails */

        if(failcount) {
		return -1;
	}
	
	return(0);
}	
