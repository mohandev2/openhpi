/* -*- linux-c -*-
 * 
 * (C) Copyright IBM Corp. 2004
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *     Peter D Phan <pdphan@users.sourceforge.net>
 */


#include <snmp_bc_plugin.h>
#include <sahpimacros.h>
#include <tsetup.h>

int main(int argc, char **argv) 
{

	/* ************************
	 * Local variables
	 * ***********************/	 
	int testfail = 0;
	SaErrorT          err;
	SaErrorT expected_err;
					
	SaHpiResourceIdT  id = 0;
        SaHpiSessionIdT sessionid;
	SaHpiResetActionT act = 0;
	SaHpiHsIndicatorStateT state = 0;
        /* *************************************                 
	 * Find a resource 
	 * * ************************************* */
	struct oh_handler l_handler;
	struct oh_handler *h= &l_handler;
	SaHpiRptEntryT rptentry;

	err = tsetup(&sessionid);
	if (err != SA_OK) {
		printf("Error! can not setup test environment\n");
		return -1;
	}

	err = tfind_resource(&sessionid, SAHPI_CAPABILITY_POWER, h, &rptentry);
	if (err != SA_OK) {
		printf("Error! can not setup test environment\n");
		err = tcleanup(&sessionid);
		return -1;
	}

	id = rptentry.ResourceId;
	/************************** 
	 * Test :
	 **************************/
	expected_err = SA_ERR_HPI_CAPABILITY;                   
	err = snmp_bc_get_indicator_state((void *)h->hnd, id, &state);
	checkstatus(&err, &expected_err, &testfail);

	/************************** 
	 * Test :
	 * expected_err = SA_ERR_HPI_CAPABILITY;
	 **************************/
	err = snmp_bc_set_indicator_state((void *)h->hnd, id, state);
	checkstatus(&err, &expected_err, &testfail);

	/************************** 
	 * Test :
	 * expected_err = SA_ERR_HPI_CAPABILITY;
	 **************************/
	err = snmp_bc_request_hotswap_action((void *)h->hnd, id, act);
	checkstatus(&err, &expected_err, &testfail);

	/************************** 
	 * Test :
	 * expected_err = SA_ERR_HPI_CAPABILITY;
	 **************************/
	err = snmp_bc_set_hotswap_state((void *)h->hnd, id, state);
	checkstatus(&err, &expected_err, &testfail);

	expected_err =  SA_ERR_HPI_INVALID_REQUEST;
	/************************** 
	 * Test :
	 * expected_err = SA_ERR_HPI_INVALID_REQUEST;
	 **************************/
	err = snmp_bc_request_hotswap_action((void *)h->hnd, id, -1);
	checkstatus(&err, &expected_err, &testfail);

	/************************** 
	 * Test :
	 * expected_err = SA_ERR_HPI_INVALID_REQUEST;
	 **************************/
	err = snmp_bc_set_indicator_state((void *)h->hnd, id, -1);
	checkstatus(&err, &expected_err, &testfail);

	/************************** 
	 * Test :
	 * expected_err = SA_ERR_HPI_INVALID_REQUEST;
	 **************************/
	err = snmp_bc_set_hotswap_state((void *)h->hnd, id, -1);
	checkstatus(&err, &expected_err, &testfail);

	/***************************
	 * Cleanup after all tests
	 ***************************/
	err = tcleanup(&sessionid);
	return testfail;

}

#include <tsetup.c>
