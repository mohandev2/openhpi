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


SaErrorT sim_el_get_info(void *hnd, SaHpiResourceIdT id,
                         SaHpiEventLogInfoT *info)
{
	SaErrorT err;
        struct oh_handler_state *state;

        if (!hnd || !info) {
                dbg("Invalid parameter.");
                return SA_ERR_HPI_INVALID_PARAMS;
	}
        state = (struct oh_handler_state *)hnd;

        err = oh_el_info(state->elcache, info);
        return err;
}


SaErrorT sim_el_set_time(void *hnd, SaHpiResourceIdT id, SaHpiTimeT time)
{
        struct oh_handler_state *state;
        SaErrorT err;

	if (!hnd) {
		dbg("Invalid parameter.");
		return SA_ERR_HPI_INVALID_PARAMS;
	}
        state = (struct oh_handler_state *)hnd;

	err = oh_el_timeset(state->elcache, 0);
        if (err) {
		dbg("Cannot set time. Error=%s.", oh_lookup_error(err));
		return SA_ERR_HPI_INTERNAL_ERROR;
	}

        return SA_OK;
}


SaErrorT sim_el_add_entry(void *hnd, SaHpiResourceIdT id,
                          const SaHpiEventT *Event)
{
        struct oh_handler_state *state;

	if (!hnd) {
		dbg("Invalid parameter.");
		return SA_ERR_HPI_INVALID_PARAMS;
	}
        state = (struct oh_handler_state *)hnd;

        return  oh_el_append(state->elcache, Event, NULL, NULL);
}


SaErrorT sim_el_get_entry(void *hnd, SaHpiResourceIdT id,
		          SaHpiEventLogEntryIdT current,
		          SaHpiEventLogEntryIdT *prev,
		          SaHpiEventLogEntryIdT *next,
		          SaHpiEventLogEntryT *entry, SaHpiRdrT  *rdr,
                          SaHpiRptEntryT *rptentry)
{

	SaErrorT err = SA_OK;
	oh_el_entry tmpentry, *tmpentryptr;
	tmpentryptr = &tmpentry;
        struct oh_handler_state *state;

        if (!hnd || !prev || !next || !entry) {
                dbg("Invalid parameter.");
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        state = (struct oh_handler_state *)hnd;

        err = oh_el_get(state->elcache, current, prev, next, &tmpentryptr);
        if (err) {
                dbg("Getting Event Log entry=%d from cache failed. Error=%s.",
                    current, oh_lookup_error(err));
                return err;
        } else {
                memcpy(entry, &(tmpentryptr->event),
                       sizeof(SaHpiEventLogEntryT));
                if (rdr)
                        memcpy(rdr, &tmpentryptr->rdr, sizeof(SaHpiRdrT));
                if (rptentry)
                        memcpy(rptentry, &(tmpentryptr->res),
                               sizeof(SaHpiRptEntryT));
        }

        return SA_OK;
}


SaErrorT sim_el_clear(void *hnd, SaHpiResourceIdT id)
{
        struct oh_handler_state *state;
	SaErrorT err;

	if (!hnd) {
		dbg("Invalid parameter.");
		return SA_ERR_HPI_INVALID_PARAMS;
	}
        state = (struct oh_handler_state *)hnd;

	err = oh_el_clear(state->elcache);
	if (err) {
		dbg("Cannot clear system Event Log. Error=%s.",
                    oh_lookup_error(err));
		return err;
	}

	return SA_OK;
}


SaErrorT sim_el_overflow(void *hnd, SaHpiResourceIdT id)
{
        struct oh_handler_state *state;

	if (!hnd) {
		dbg("Invalid parameter.");
		return SA_ERR_HPI_INVALID_PARAMS;
	}
        state = (struct oh_handler_state *)hnd;

        return oh_el_overflowreset(state->elcache);
}


void * oh_get_el_info (void *, SaHpiResourceIdT, SaHpiEventLogInfoT *)
                __attribute__ ((weak, alias("sim_el_get_info")));

void * oh_set_el_time (void *, SaHpiResourceIdT, const SaHpiEventT *)
                __attribute__ ((weak, alias("sim_el_set_time")));

void * oh_add_el_entry (void *, SaHpiResourceIdT, const SaHpiEventT *)
                __attribute__ ((weak, alias("sim_el_add_entry")));

void * oh_get_el_entry (void *, SaHpiResourceIdT, SaHpiEventLogEntryIdT,
                       SaHpiEventLogEntryIdT *, SaHpiEventLogEntryIdT *,
                       SaHpiEventLogEntryT *, SaHpiRdrT *, SaHpiRptEntryT  *)
                __attribute__ ((weak, alias("sim_el_get_entry")));

void * oh_clear_el (void *, SaHpiResourceIdT)
                __attribute__ ((weak, alias("sim_el_clear")));

void * oh_reset_el_overflow (void *, SaHpiResourceIdT)
                __attribute__ ((weak, alias("sim_el_overflow")));


