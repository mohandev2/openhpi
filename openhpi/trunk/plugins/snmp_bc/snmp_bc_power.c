/*      -*- linux-c -*-
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
 *      Steve Sherman <stevees@us.ibm.com>
 */

#include <snmp_bc_plugin.h>

/**
 * snmp_bc_get_hotswap_state:
 * @hnd: Handler data pointer.
 * @rid: Resource ID.
 * @state: Location to store resource's hotswap state.
 *
 * Retrieves a sensor's value and/or state. Both @data and @state
 * may be NULL, in which case this function can be used to test for
 * sensor presence.
 *
 * Return values:
 * SA_OK - Normal case.
 * SA_ERR_HPI_CAPABILITY - Resource doesn't have SAHPI_CAPABILITY_SENSOR.
 * SA_ERR_HPI_INVALID_RESOURCE - Resource doesn't exist.
 * SA_ERR_HPI_INVALID_REQUEST - Sensor is disabled.
 * SA_ERR_HPI_INVALID_PARAMS - Pointer parameter(s) are NULL.
 **/
SaErrorT snmp_bc_get_power_state(void *hnd,
				 SaHpiResourceIdT rid,
				 SaHpiPowerStateT *state)
{
	if (!hnd || !state){
		dbg("Missing handle\n");
		return SA_ERR_HPI_INVALID_PARAMS;
	}

	gchar *oid;
	int rtn_code = SA_OK;
        struct snmp_value get_value;
	SaErrorT status;
        struct oh_handler_state *handle = (struct oh_handler_state *)hnd;
        struct snmp_bc_hnd *custom_handle = (struct snmp_bc_hnd *)handle->data;

        SaHpiRptEntryT *res = oh_get_resource_by_id(handle->rptcache, rid);
	if(res == NULL) {
		return SA_ERR_HPI_INVALID_RESOURCE;
	}
         struct ResourceInfo *s =
                (struct ResourceInfo *)oh_get_resource_data(handle->rptcache, rid);
	if(s == NULL) {
		return SA_ERR_HPI_INVALID_CMD;
	}
	if(s->mib.OidPowerState == NULL) { 
		return SA_ERR_HPI_INVALID_CMD;
	}

	oid = oh_derive_string(&(res->ResourceEntity), s->mib.OidPowerState);
	if(oid == NULL) {
		dbg("NULL SNMP OID returned for %s\n",s->mib.OidPowerState);
		return SA_ERR_HPI_INTERNAL_ERROR;
	}


	status = snmp_bc_snmp_get(custom_handle, oid, &get_value);
	if(( status == SA_OK) && (get_value.type == ASN_INTEGER)) {
		switch (get_value.integer) {
		case 0:
			*state = SAHPI_POWER_OFF;
			break;
		case 1:
			*state = SAHPI_POWER_ON;
			break;
		default:
			dbg("Invalid power state read for oid=%s\n",oid);
			rtn_code = SA_ERR_HPI_INTERNAL_ERROR;
		}
        } else {
		dbg("Couldn't fetch SNMP %s vector; Type=%d\n",oid,get_value.type);
		rtn_code = status;
	}

	g_free(oid);
        return rtn_code;
}

SaErrorT snmp_bc_set_power_state(void *hnd,
				 SaHpiResourceIdT rid,
				 SaHpiPowerStateT state)
{
	if (!hnd){
		dbg("Missing handle\n");
		return SA_ERR_HPI_INVALID_PARAMS;
	}

	gchar *oid;
	int rtn_code = SA_OK;
	SaErrorT status;

        struct snmp_value set_value;
        struct oh_handler_state *handle = (struct oh_handler_state *)hnd;
        struct snmp_bc_hnd *custom_handle = (struct snmp_bc_hnd *)handle->data;

        SaHpiRptEntryT *res = oh_get_resource_by_id(handle->rptcache, rid);
	if(res == NULL) {
		return SA_ERR_HPI_INVALID_RESOURCE;
	}
        struct ResourceInfo *s =
                (struct ResourceInfo *)oh_get_resource_data(handle->rptcache, rid);
	if(s == NULL) {
		return SA_ERR_HPI_INVALID_CMD;
	}
	if(s->mib.OidPowerOnOff == NULL) { 
		return SA_ERR_HPI_INVALID_CMD; 
	}

	oid = oh_derive_string(&(res->ResourceEntity), s->mib.OidPowerOnOff);
	if(oid == NULL) {
		dbg("NULL SNMP OID returned for %s\n",s->mib.OidPowerOnOff);
		return SA_ERR_HPI_INTERNAL_ERROR;
	}

	set_value.type = ASN_INTEGER;
	set_value.str_len = 1;
	switch (state) {
	case SAHPI_POWER_OFF:
		set_value.integer = 0;
		status = snmp_bc_snmp_set(custom_handle, oid, set_value);
		if (status != SA_OK) {
			dbg("SNMP could not set %s; Type=%d.\n",s->mib.OidPowerOnOff,set_value.type);
			if (status == SA_ERR_HPI_BUSY) return status;
			else return SA_ERR_HPI_NO_RESPONSE;
		}
		break;
		
	case SAHPI_POWER_ON:
		set_value.integer = 1;
		status = snmp_bc_snmp_set(custom_handle, oid, set_value);
		if (status != SA_OK) {
			dbg("SNMP could not set %s; Type=%d.\n",s->mib.OidPowerOnOff,set_value.type);
			if (status == SA_ERR_HPI_BUSY) return status;
			else return SA_ERR_HPI_NO_RESPONSE;
		}
		break;
		
	case SAHPI_POWER_CYCLE:
	        {
			SaHpiResetActionT act = SAHPI_COLD_RESET;
			rtn_code=snmp_bc_set_reset_state(hnd, rid, act);
	        }
		break;
	default:
		dbg("Invalid Power Action Type - %d\n", state);
		rtn_code = SA_ERR_HPI_INVALID_PARAMS;
	}

	g_free(oid);
        return rtn_code;
}
