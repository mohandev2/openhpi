/*
 * (C) Copyright 2016-2017 Hewlett Packard Enterprise Development LP
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the Hewlett Packard Enterprise, nor the names
 * of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author(s)
 *      Hemantha Beecherla <hemantha.beecherla@hpe.com>
 *      Mohan Devarajulu <mohan.devarajulu@hpe.com>
 */

/* Include libraries */
#include "ov_rest_event.h"
#include "ov_rest_composer_event.h"
#include "ov_rest_discover.h"
#include "sahpi_wrappers.h"
#include "ov_rest_parser_calls.h"
#include "ov_rest.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

SaErrorT ov_rest_proc_composer_status(struct oh_handler_state *oh_handler,
 					 struct eventInfo *ov_event,
					 enum healthStatus composer_health)
{
	SaErrorT rv = SA_OK;
	struct ov_rest_handler *ov_handler = NULL;
	struct composer_status *composer = {0};
	struct applianceInfo appliance = {{0}};
	SaHpiRptEntryT *rpt = NULL;
	SaHpiSeverityT severity = 0;
	struct oh_event event;
	struct enclosureStatusResponse enclosure_response = {0};
	char *enclosure_doc = NULL;
	int bayNumber = 0;
	json_object * jvalue = NULL, *appliance_array = NULL;
	struct ov_rest_sensor_info *sensor_info = NULL;
	SaHpiInt32T sensor_status = 0;
	SaHpiInt32T sensor_val = 0;
	SaHpiRdrT rdr = {0};
	SaHpiResourceIdT resource_id = 0;

	if (oh_handler == NULL)
	{
		err ("Invalid parameters");
		return SA_ERR_HPI_INVALID_PARAMS;
	}
	ov_handler = (struct ov_rest_handler *) oh_handler->data;
	composer = (struct composer_status*)&ov_handler->
					ov_rest_resources.composer;
	
	/* Check whether this alerts is coming from Active Appliance/Composer */
	bayNumber = ov_rest_get_baynumber(ov_event->resourceID);
	asprintf (&ov_handler->connection->url,"https://%s%s" ,
			ov_handler->connection->hostname,ov_event->resourceUri);
	rv = ov_rest_getenclosureStatus(oh_handler, &enclosure_response,
			ov_handler->connection, enclosure_doc);
	if(rv != SA_OK){
		return rv;
	}
	if(!enclosure_response.enclosure){
		err("No Response for enclosure status call");
		return SA_ERR_HPI_INVALID_DATA;
	}
	appliance_array = ov_rest_wrap_json_object_object_get(
					enclosure_response.enclosure,
					"applianceBays");
	if(!appliance_array ||
		(json_object_get_type(appliance_array) != json_type_array)){
		err("Invalid Response for appliance bay %d",bayNumber);
		ov_rest_wrap_json_object_put(enclosure_response.root_jobj);
		return SA_ERR_HPI_INVALID_DATA;
	}
	jvalue = json_object_array_get_idx(appliance_array, bayNumber-1);
	if(!jvalue){
		err("Invalid Response for appliance bay %d",bayNumber);
		ov_rest_wrap_json_object_put(enclosure_response.root_jobj);
		return SA_ERR_HPI_INVALID_DATA;
	}
	ov_rest_json_parse_applianceInfo(jvalue, &appliance);
	ov_rest_wrap_json_object_put(enclosure_response.root_jobj);
	if(!appliance.serialNumber){
		err("No serial number at appliance bay %d",bayNumber);
		return SA_ERR_HPI_INVALID_DATA;
	}
	if(strcmp(appliance.serialNumber, composer->serial_number)){
		warn("This alert is not from Active Appliance, ignore");
		wrap_g_free(enclosure_doc);
		return SA_OK;	
	}	

	/* Get the rpt entry of the resource */
	rpt = oh_get_resource_by_id(oh_handler->rptcache,
			composer->resource_id);
	if (rpt == NULL) {
		err("resource RPT is NULL for composure rid %d",
				composer->resource_id);
		wrap_g_free(enclosure_doc);
		return SA_ERR_HPI_INTERNAL_ERROR;
	}

	memset(&event, 0, sizeof(struct oh_event));

	switch (composer_health) {
		case OK:
			severity = SAHPI_OK;
			/* Resource restored */
			rpt->ResourceFailed = SAHPI_FALSE;
			event.event.EventDataUnion.ResourceEvent.
				ResourceEventType = SAHPI_RESE_RESOURCE_RESTORED;
			break;
		case Critical:
		case Disabled:
			severity = SAHPI_CRITICAL;
			/* Resource failed */
			rpt->ResourceFailed = SAHPI_TRUE;
			event.event.EventDataUnion.ResourceEvent.
				ResourceEventType = SAHPI_RESE_RESOURCE_FAILURE;
			break;
		case Warning:
			severity = SAHPI_MAJOR;
			/* Resource failed */
			rpt->ResourceFailed = SAHPI_TRUE;
			event.event.EventDataUnion.ResourceEvent.
				ResourceEventType = SAHPI_RESE_RESOURCE_FAILURE;
			break;
		default:
			err("Unknown Composer status %d ", composer_health);
			wrap_g_free(enclosure_doc);
			return SA_ERR_HPI_INTERNAL_ERROR;
	}

	if (rpt->ResourceSeverity == severity) {
		dbg("Ignore the event. There is no change in composer status");
		wrap_g_free(enclosure_doc);
		return rv;
	}

	rpt->ResourceSeverity = severity;
	rv = oh_add_resource(oh_handler->rptcache, rpt, NULL, 0);
	if (rv != SA_OK) {
		err("Failed to update Composer rpt");
		wrap_g_free(enclosure_doc);
		return rv;
	}

	/* Update the event structure */
	event.hid = oh_handler->hid;
	oh_gettimeofday(&(event.event.Timestamp));
	event.event.Source =
		composer->resource_id;
	event.event.Severity = severity;
	event.event.EventType = SAHPI_ET_RESOURCE;
	memcpy(&(event.resource), rpt, sizeof(SaHpiRptEntryT));

	/* Raise the HPI sensor event */
	oh_evt_queue_push(oh_handler->eventq, copy_ov_rest_event(&event));

	/* Build operational status sensor rdr */
	switch (composer_health) {
		case OK:
			sensor_val = OP_STATUS_OK;
			break;
		case Critical:
			sensor_val = OP_STATUS_CRITICAL;
			break;
		case Warning:
			sensor_val = OP_STATUS_WARNING;
			break;
		case Disabled:
			sensor_val = OP_STATUS_DISABLED;
			break;
		default :
			sensor_val = OP_STATUS_UNKNOWN;
	}

	OV_REST_BUILD_ENABLE_SENSOR_RDR(OV_REST_SEN_OPER_STATUS, sensor_val);
		
	wrap_g_free(enclosure_doc);
	return SA_OK;
}

