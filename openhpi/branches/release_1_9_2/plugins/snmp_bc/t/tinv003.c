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
	SaHpiIdrIdT       idrId = 0;
	SaHpiEntryIdT     areaId = 0;
	/* SaHpiIdrAreaTypeT areatype; */
	SaHpiEntryIdT     nextAreaId;
	SaHpiIdrAreaHeaderT header;
        /* *************************************                 
	 * Find a resource with Sensor type rdr
	 * * ************************************* */
	struct oh_handler l_handler;
	struct oh_handler *h= &l_handler;
	SaHpiRptEntryT rptentry;
	SaHpiRdrT	*rdrptr;

	err = tsetup(&sessionid);
	if (err != SA_OK) {
		printf("Error! can not setup test environment\n");
		return -1;
	}

	err = tfind_resource(&sessionid, SAHPI_CAPABILITY_INVENTORY_DATA, h, &rptentry);
	if (err != SA_OK) {
		printf("Error! can not setup test environment\n");
		err = tcleanup(&sessionid);
		return -1;
	}

	struct oh_handler_state *handle = (struct oh_handler_state *)h->hnd;
	id = rptentry.ResourceId;
        idrId  = 0;
	do {
		idrId++;
		rdrptr = oh_get_rdr_by_type(handle->rptcache, id, SAHPI_INVENTORY_RDR, idrId);
		if (rdrptr != NULL) break;
	} while ((rdrptr == NULL) && (idrId < 128));

        if (rdrptr == NULL) testfail = -1;
		
	/************************** 
	 * Test :
	 **************************/
	expected_err = SA_ERR_HPI_NOT_PRESENT;                   
	err = snmp_bc_get_idr_area_header(handle , id, 5000,
		       			SAHPI_IDR_AREATYPE_UNSPECIFIED,
				       	areaId, &nextAreaId, &header);
	checkstatus(&err, &expected_err, &testfail);
		
	/************************** 
	 * Test :
	 **************************/
	expected_err = SA_OK;                   
	err = snmp_bc_get_idr_area_header(handle , id, idrId,
		       			SAHPI_IDR_AREATYPE_UNSPECIFIED,
				       	areaId, &nextAreaId, &header);
	checkstatus(&err, &expected_err, &testfail);
		
	/************************** 
	 * Test :
	 **************************/
	expected_err = SA_OK;                   
	err = snmp_bc_get_idr_area_header(handle , id, idrId,
		       			SAHPI_IDR_AREATYPE_UNSPECIFIED,
				       	SAHPI_FIRST_ENTRY, &nextAreaId, &header);
	checkstatus(&err, &expected_err, &testfail);

	/**************************&*
	 * Cleanup after all tests
	 ***************************/
	err = tcleanup(&sessionid);
	return testfail;

}

#include <tsetup.c>
