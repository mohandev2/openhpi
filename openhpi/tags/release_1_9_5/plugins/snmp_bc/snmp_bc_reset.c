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
 * snmp_bc_get_reset_state:
 * @hnd: Handler data pointer.
 * @rid: Resource ID.
 * @act: Location to store resource's reset action state.
 *
 * Retrieves a resource's reset action state.
 * Always return SAHPI_RESET_DEASSERT.
 *
 * Return values:
 * SA_OK - Normal case.
 * SA_ERR_HPI_CAPABILITY - Resource doesn't have SAHPI_CAPABILITY_RESET.
 * SA_ERR_HPI_INVALID_RESOURCE - Resource doesn't exist.
 * SA_ERR_HPI_INVALID_PARAMS - Pointer parameter(s) are NULL.
 **/
SaErrorT snmp_bc_get_reset_state(void *hnd,
				 SaHpiResourceIdT rid,
				 SaHpiResetActionT *act)
{
	if (!hnd || !act) {
		dbg("Invalid parameter.");
		return(SA_ERR_HPI_INVALID_PARAMS);
	}

        struct oh_handler_state *handle = (struct oh_handler_state *)hnd;
        struct snmp_bc_hnd *custom_handle = (struct snmp_bc_hnd *)handle->data;

	if (!custom_handle) {
		dbg("Invalid parameter.");
		return(SA_ERR_HPI_INVALID_PARAMS);
	}

	/* Check if resource exists and has reset capabilities */
	SaHpiRptEntryT *rpt = oh_get_resource_by_id(handle->rptcache, rid);
        if (!rpt) return(SA_ERR_HPI_INVALID_RESOURCE);
        if (!(rpt->ResourceCapabilities & SAHPI_CAPABILITY_RESET)) return(SA_ERR_HPI_CAPABILITY);

	*act = SAHPI_RESET_DEASSERT;

	return(SA_OK);
}

/**
 * snmp_bc_set_reset_state:
 * @hnd: Handler data pointer.
 * @rid: Resource ID.
 * @act: Reset action state to set.
 *
 * Sets a resource's reset action state.
 *
 * Return values:
 * SA_OK - Normal case.
 * SA_ERR_HPI_CAPABILITY - Resource doesn't have SAHPI_CAPABILITY_RESET.
 * SA_ERR_HPI_INVALID_CMD - Resource doesn't support SAHPI_RESET_ASSERT.
 * SA_ERR_HPI_INVALID_RESOURCE - Resource doesn't exist.
 * SA_ERR_HPI_INVALID_PARAMS - Pointer parameter(s) are NULL; @act invalid.
 **/
SaErrorT snmp_bc_set_reset_state(void *hnd,
				 SaHpiResourceIdT rid,
				 SaHpiResetActionT act)
{
	SaErrorT err;
	struct ResourceInfo *resinfo;
        struct snmp_value set_value;

	if (!hnd || NULL == oh_lookup_resetaction(act)){
		dbg("Invalid parameter.");
		return(SA_ERR_HPI_INVALID_PARAMS);
	}
	if (act == SAHPI_RESET_ASSERT || act == SAHPI_RESET_DEASSERT) return(SA_ERR_HPI_INVALID_CMD);
	
        struct oh_handler_state *handle = (struct oh_handler_state *)hnd;
        struct snmp_bc_hnd *custom_handle = (struct snmp_bc_hnd *)handle->data;

	if (!custom_handle) {
		dbg("Invalid parameter.");
		return(SA_ERR_HPI_INVALID_PARAMS);
	}

	/* Check if resource exists and has reset capabilities */
	SaHpiRptEntryT *rpt = oh_get_resource_by_id(handle->rptcache, rid);
        if (!rpt) return(SA_ERR_HPI_INVALID_RESOURCE);
        if (!(rpt->ResourceCapabilities & SAHPI_CAPABILITY_RESET)) return(SA_ERR_HPI_CAPABILITY);

	resinfo = (struct ResourceInfo *)oh_get_resource_data(handle->rptcache, rid);
 	if (resinfo == NULL) {
		dbg("No resource data. Resource=%s", rpt->ResourceTag.Data);
		return(SA_ERR_HPI_INTERNAL_ERROR);
	}       

	if (resinfo->mib.OidReset == NULL) {
		dbg("No Reset OID.");
		return(SA_ERR_HPI_INTERNAL_ERROR);
	}

	switch (act) {
	case SAHPI_COLD_RESET: /* COLD = WARM Reset Action */
	case SAHPI_WARM_RESET:
		
		set_value.type = ASN_INTEGER;
		set_value.str_len = 1;
		set_value.integer = 1;
		
		err = snmp_bc_oid_snmp_set(custom_handle, &(rpt->ResourceEntity),
						 resinfo->mib.OidReset, set_value);
		if (err) {
			dbg("Cannot set SNMP OID=%s; Type=%d.", resinfo->mib.OidReset, set_value.type);
			return(err);
		}
		break;
	case SAHPI_RESET_ASSERT: /* RESET_ASSERT = RESET_DEASSERT Action */
	case SAHPI_RESET_DEASSERT:
		return(SA_ERR_HPI_INVALID_CMD);
	default:
		dbg("Invalid Reset Action Type=%d.", act);
		return(SA_ERR_HPI_INTERNAL_ERROR);
	}

        return(SA_OK);
}
