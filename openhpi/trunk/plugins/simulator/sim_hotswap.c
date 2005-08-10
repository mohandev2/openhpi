/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2005
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *      W. David Ashley <dashley@us.ibm.com>
 */

#include <sim_init.h>


/*----------------------------------------------------------------------
  Note: we use the full HS model for the simulator.
  ----------------------------------------------------------------------*/


SaErrorT sim_get_hotswap_state(void *hnd,
			       SaHpiResourceIdT rid,
			       SaHpiHsStateT    *hsstate)
{
        struct simResourceInfo *privinfo;

	if (!hnd || !hsstate) {
		dbg("Invalid parameter.");
		return SA_ERR_HPI_INVALID_PARAMS;
	}

        struct oh_handler_state *state = (struct oh_handler_state *)hnd;

	/* Check if resource exists and has managed hotswap capabilities */
	SaHpiRptEntryT *rpt = oh_get_resource_by_id(state->rptcache, rid);
        if (!rpt) {
		return SA_ERR_HPI_INVALID_RESOURCE;
	}
        if (!(rpt->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP)) {
		dbg("No hs capability");
		return SA_ERR_HPI_CAPABILITY;
	}

        /* get our private state data */
	privinfo = (struct simResourceInfo *)oh_get_resource_data(state->rptcache, rid);
 	if (privinfo == NULL) {
		dbg("No resource data. ResourceId=%d", rid);
		return(SA_ERR_HPI_INTERNAL_ERROR);
	}

	*hsstate = privinfo->cur_hsstate;
	return SA_OK;
}


SaErrorT sim_set_hotswap_state(void *hnd,
			       SaHpiResourceIdT rid,
			       SaHpiHsStateT    hsstate)
{
        struct simResourceInfo *privinfo;

	if (!hnd) {
		dbg("Invalid parameter.");
		return SA_ERR_HPI_INVALID_PARAMS;
	}

	if (NULL == oh_lookup_hsstate(hsstate)) {
		dbg("Invalid hotswap state.");
		return SA_ERR_HPI_INVALID_REQUEST;
	}

        struct oh_handler_state *state = (struct oh_handler_state *)hnd;

	/* Check if resource exists and has managed hotswap capabilities */
	SaHpiRptEntryT *rpt = oh_get_resource_by_id(state->rptcache, rid);
        if (!rpt) {
		return SA_ERR_HPI_INVALID_RESOURCE;
	}
        if (!(rpt->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP)) {
		return SA_ERR_HPI_CAPABILITY;
	}

        /* get our private state data */
	privinfo = (struct simResourceInfo *)oh_get_resource_data(state->rptcache, rid);
 	if (privinfo == NULL) {
		dbg("No resource data. ResourceId=%d", rid);
		return(SA_ERR_HPI_INTERNAL_ERROR);
	}

        /* check that the state transition is correct */
        switch (privinfo->cur_hsstate) {
        case SAHPI_HS_STATE_INACTIVE:
                if (hsstate == SAHPI_HS_STATE_NOT_PRESENT) {
                        privinfo->cur_hsstate = hsstate;
                        return SA_OK;
                }
                if (hsstate == SAHPI_HS_STATE_INSERTION_PENDING) {
                        privinfo->cur_hsstate = hsstate;
                        return SA_OK;
                }
                break;
        case SAHPI_HS_STATE_INSERTION_PENDING:
                if (hsstate == SAHPI_HS_STATE_NOT_PRESENT) {
                        privinfo->cur_hsstate = hsstate;
                        return SA_OK;
                }
                if (hsstate == SAHPI_HS_STATE_INACTIVE) {
                        privinfo->cur_hsstate = hsstate;
                        return SA_OK;
                }
                if (hsstate == SAHPI_HS_STATE_ACTIVE) {
                        privinfo->cur_hsstate = hsstate;
                        return SA_OK;
                }
                break;
        case SAHPI_HS_STATE_ACTIVE:
                if (hsstate == SAHPI_HS_STATE_NOT_PRESENT) {
                        privinfo->cur_hsstate = hsstate;
                        return SA_OK;
                }
                if (hsstate == SAHPI_HS_STATE_EXTRACTION_PENDING) {
                        privinfo->cur_hsstate = hsstate;
                        return SA_OK;
                }
                break;
        case SAHPI_HS_STATE_EXTRACTION_PENDING:
                if (hsstate == SAHPI_HS_STATE_NOT_PRESENT) {
                        privinfo->cur_hsstate = hsstate;
                        return SA_OK;
                }
                if (hsstate == SAHPI_HS_STATE_ACTIVE) {
                        privinfo->cur_hsstate = hsstate;
                        return SA_OK;
                }
                if (hsstate == SAHPI_HS_STATE_INACTIVE) {
                        privinfo->cur_hsstate = hsstate;
                        return SA_OK;
                }
                break;
        case SAHPI_HS_STATE_NOT_PRESENT:
                if (hsstate == SAHPI_HS_STATE_INSERTION_PENDING) {
                        privinfo->cur_hsstate = hsstate;
                        return SA_OK;
                }
                break;
        default:
                break;
        }

	return SA_ERR_HPI_INVALID_REQUEST;
}


SaErrorT sim_request_hotswap_action(void *hnd,
				    SaHpiResourceIdT rid,
				    SaHpiHsActionT act)
{
        struct simResourceInfo *privinfo;

	if (!hnd) {
		dbg("Invalid parameter.");
		return SA_ERR_HPI_INVALID_PARAMS;
	}

	if (NULL == oh_lookup_hsaction(act)) {
		dbg("Invalid hotswap action.");
		return SA_ERR_HPI_INVALID_REQUEST;
	}

        struct oh_handler_state *state = (struct oh_handler_state *)hnd;

	/* Check if resource exists and has managed hotswap capabilities */
	SaHpiRptEntryT *rpt = oh_get_resource_by_id(state->rptcache, rid);
        if (!rpt) {
		return SA_ERR_HPI_INVALID_RESOURCE;
	}

        /* if not simplified HS then return an error */
        if (!(rpt->ResourceCapabilities & SAHPI_CAPABILITY_FRU)) {
		return SA_ERR_HPI_CAPABILITY;
	}

        /* get our private state data */
	privinfo = (struct simResourceInfo *)oh_get_resource_data(state->rptcache, rid);
 	if (privinfo == NULL) {
		dbg("No resource data. ResourceId=%d", rid);
		return(SA_ERR_HPI_INTERNAL_ERROR);
	}

        /* check that the action corresponds to a valid state */
        if (act == SAHPI_HS_ACTION_INSERTION &&
            privinfo ->cur_hsstate == SAHPI_HS_STATE_INACTIVE) {
                privinfo->cur_hsstate = SAHPI_HS_STATE_INSERTION_PENDING;
                return SA_OK;
        }
        if (act == SAHPI_HS_ACTION_EXTRACTION &&
            privinfo ->cur_hsstate == SAHPI_HS_STATE_ACTIVE) {
                privinfo->cur_hsstate = SAHPI_HS_STATE_EXTRACTION_PENDING;
                return SA_OK;
        }

	return SA_ERR_HPI_INVALID_REQUEST;
}


void * oh_get_hotswap_state (void *, SaHpiResourceIdT, SaHpiHsStateT *)
                __attribute__ ((weak, alias("sim_get_hotswap_state")));

void * oh_set_hotswap_state (void *, SaHpiResourceIdT, SaHpiHsStateT)
                __attribute__ ((weak, alias("sim_set_hotswap_state")));

void * oh_request_hotswap_action (void *, SaHpiResourceIdT, SaHpiHsActionT)
                __attribute__ ((weak, alias("sim_request_hotswap_action")));


