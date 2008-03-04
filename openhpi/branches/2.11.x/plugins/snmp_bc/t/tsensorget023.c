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
	SaHpiBoolT disabled = SAHPI_FALSE;
	SaHpiResourceIdT  id;
        SaHpiSessionIdT sessionid;
	 
	SaHpiSensorNumT sid = 0;
	SaHpiEventStateT state;
	SaHpiSensorReadingT reading;
	struct SensorInfo *sinfo;
													    
	/* *************************************	 	 
	 * Find a resource with Sensor type rdr
	 * ************************************* */
        struct oh_handler l_handler;
	struct oh_handler *h= &l_handler;
        SaHpiRptEntryT rptentry;
	SaHpiRdrT *rdrptr;
	
	err = tsetup(&sessionid);
	if (err != SA_OK) {
		printf("Error! bc_sensor, can not setup test environment\n");
		return -1;

	}
	err = tfind_resource(&sessionid, (SaHpiCapabilitiesT) SAHPI_CAPABILITY_SENSOR, h, &rptentry);
	if (err != SA_OK) {
		printf("Error! bc_sensor, can not setup test environment\n");
		err = tcleanup(&sessionid);
		return -1;

	}
	
	struct oh_handler_state *handle = (struct oh_handler_state *)h->hnd;
	id = rptentry.ResourceId;
	/************************** 
	 * Test 23: 
	 **************************/
	sid = 0;
	do {
		sid++,
		rdrptr = oh_get_rdr_by_type(handle->rptcache, id, SAHPI_SENSOR_RDR, sid);
		if (rdrptr != NULL) {
			if (rdrptr->RdrTypeUnion.SensorRec.DataFormat.IsSupported == SAHPI_TRUE) {
				break;
			} else 
				rdrptr = NULL;
		}
	} while ((rdrptr == NULL) && (sid < 128 ));

	if (rdrptr == NULL) testfail = -1;		
	sinfo = (struct SensorInfo *)oh_get_rdr_data(handle->rptcache, id, rdrptr->RecordId);	
	/**************************
	 * Test 24: 
	 **************************/
	if (sinfo->sensor_enabled == SAHPI_TRUE) {
		sinfo->sensor_enabled = SAHPI_FALSE;
		disabled = SAHPI_TRUE;
	}
	expected_err = SA_ERR_HPI_INVALID_REQUEST;
	err = snmp_bc_get_sensor_eventstate((void *)h->hnd, id, sid, &reading, &state);
	checkstatus(&err, &expected_err, &testfail);
	/***************************
	 * Cleanup after all tests
	 ***************************/
	 err = tcleanup(&sessionid);
	 return testfail;

}

#include <tsetup.c>
