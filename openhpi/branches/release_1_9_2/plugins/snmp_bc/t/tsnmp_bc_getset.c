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
	gchar *BUSY_OID = ".1.3.6.1.4.1.2.3.51.2.4.4.1.7777";

	int testfail = 0;
	SaErrorT          err;
	SaErrorT expected_err;

        SaHpiSessionIdT sessionid;
	struct snmp_value value;
		
	/* ************************	 	 
	 * Find a resource with Control type rdr
	 * ***********************/
        struct oh_handler l_handler;
	struct oh_handler *h= &l_handler;
        SaHpiRptEntryT rptentry;
	
	err = tsetup(&sessionid);
	if (err != SA_OK) {
		printf("Error! bc_set_resource_tag, can not setup test environment\n");
		return -1;

	}
	err = tfind_resource(&sessionid, (SaHpiCapabilitiesT) SAHPI_CAPABILITY_CONTROL, h, &rptentry);
	if (err != SA_OK) {
		printf("Error! bc_set_resource_tag, can not setup test environment\n");
		err = tcleanup(&sessionid);
		return -1;

	}

        struct oh_handler_state *handle = (struct oh_handler_state *)h->hnd;
	struct snmp_bc_hnd *custom_handle = (struct snmp_bc_hnd *)handle->data;
		
	/************************** 
	 * Test 1: Test Busy Status
	 **************************/
	expected_err = SA_ERR_HPI_BUSY;
	err = snmp_bc_snmp_get(custom_handle, BUSY_OID, &value); 
	checkstatus(&err, &expected_err, &testfail);
	
	/************************** 
	 * Test 2: Test Timeout Status
	 **************************/
	custom_handle->handler_retries = SNMP_BC_MAX_SNMP_RETRY_ATTEMPTED;
	expected_err = SA_ERR_HPI_NO_RESPONSE;

	err = snmp_bc_snmp_get(custom_handle, BUSY_OID, &value); 
	checkstatus(&err, &expected_err, &testfail);
	
	/************************** 
	 * Test 3: Valid case
	 **************************/
	expected_err = SA_OK;

	err = snmp_bc_snmp_get(custom_handle, SNMP_BC_DATETIME_OID, &value); 
	checkstatus(&err, &expected_err, &testfail);
	
	/************************** 
	 * Test 4: validate field
	 **************************/
	if (custom_handle->handler_retries != 0) {
		printf("Error! handler_retries does not get cleared after a successful snmp_get\n");
		testfail = -1;
	}
		
	/************************** 
	 * Test 5: Test Busy Status, snmp_set
	 **************************/
	expected_err = SA_ERR_HPI_BUSY;
	err = snmp_bc_snmp_set(custom_handle, BUSY_OID, value); 
	checkstatus(&err, &expected_err, &testfail);
	
	/************************** 
	 * Test 6: Test Timeout Status, snmp_set
	 **************************/
	custom_handle->handler_retries = SNMP_BC_MAX_SNMP_RETRY_ATTEMPTED;
	expected_err = SA_ERR_HPI_NO_RESPONSE;

	err = snmp_bc_snmp_set(custom_handle, BUSY_OID, value); 
	checkstatus(&err, &expected_err, &testfail);
	
	/************************** 
	 * Test 7: Valid case
	 **************************/
	expected_err = SA_OK;

	err = snmp_bc_snmp_set(custom_handle, SNMP_BC_DATETIME_OID, value); 
	checkstatus(&err, &expected_err, &testfail);
	
	/************************** 
	 * Test 8: validate field
	 **************************/
	if (custom_handle->handler_retries != 0) {
		printf("Error! handler_retries does not get cleared after a successful snmp_set\n");
		testfail = -1;
	}

	/***************************
	 * Cleanup after all tests
	 ***************************/
	 err = tcleanup(&sessionid);
	 return testfail;

}

#include <tsetup.c>
