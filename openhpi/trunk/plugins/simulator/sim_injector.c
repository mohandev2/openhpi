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
 *
 */


#include <sim_init.h>


static struct oh_event *eventdup(const struct oh_event *event)
{
        struct oh_event *e;
        e = g_malloc0(sizeof(*e));
        if (!e) {
                dbg("Out of memory!");
                return NULL;
        }
        memcpy(e, event, sizeof(*e));
        return e;
}



static SaErrorT sim_create_resourcetag(SaHpiTextBufferT *buffer, const char *str, SaHpiEntityLocationT loc)
{
	char *locstr;
	SaErrorT err = SA_OK;
	SaHpiTextBufferT working;

	if (!buffer || loc < SIM_HPI_LOCATION_BASE ||
	    loc > (pow(10, OH_MAX_LOCATION_DIGITS) - 1)) {
		return(SA_ERR_HPI_INVALID_PARAMS);
	}

	err = oh_init_textbuffer(&working);
	if (err) { return(err); }

	locstr = (gchar *)g_malloc0(OH_MAX_LOCATION_DIGITS + 1);
	if (locstr == NULL) {
		dbg("Out of memory.");
		return(SA_ERR_HPI_OUT_OF_SPACE);
	}
	snprintf(locstr, OH_MAX_LOCATION_DIGITS + 1, " %d", loc);

	if (str) { oh_append_textbuffer(&working, str); }
	err = oh_append_textbuffer(&working, locstr);
	if (!err) {
		err = oh_copy_textbuffer(buffer, &working);
	}
	g_free(locstr);
	return(err);
}


/* return a handler state pointer by looking for its handler_name */
struct oh_handler_state *sim_get_handler_by_name(char *name)
{
        struct oh_handler_state *state = NULL;
        int i = 0;
        char *handler_name;
        state = (struct oh_handler_state *)g_slist_nth_data(sim_handler_states, i);
        while (state != NULL) {
                handler_name = (char *)g_hash_table_lookup(state->config,
                                                           "handler_name");
                if (strcmp(handler_name, name) == 0) {
                        return state;
                }
                state = (struct oh_handler_state *)g_slist_nth_data(sim_handler_states, i);
        }

        return NULL;
}







/* inject a resource */
// assuptions about the input SaHpiRptEntryT *data entry
// - all fields are assumed to have valid values except
//    o EntryId (filled in by oh_add_resource function)
//    o ResourceId
//    o ResourceEntity (assumed to have only partial data)
SaErrorT sim_inject_resource(struct oh_handler_state *state,
                             SaHpiRptEntryT *data, void *privdata,
                             const char * comment) {
	SaHpiEntityPathT root_ep;
	SaHpiRptEntryT *res;
	char *entity_root;
	struct oh_event event;
        struct simResourceInfo *privinfo;
        SaErrorT rc;

        /* check arguments */
        if (state == NULL || data == NULL) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        /* get the entity root */
	entity_root = (char *)g_hash_table_lookup(state->config,"entity_root");
	oh_encode_entitypath (entity_root, &root_ep);

        /* set up the rpt entry */
        res = g_malloc(sizeof(SaHpiRptEntryT));
        if (res == NULL) {
                dbg("Out of memory in build_rptcache\n");
                return SA_ERR_HPI_OUT_OF_MEMORY;
        }
        memcpy(res, data, sizeof(SaHpiRptEntryT));
        oh_concat_ep(&res->ResourceEntity, &root_ep);
        res->ResourceId = oh_uid_from_entity_path(&res->ResourceEntity);
        sim_create_resourcetag(&res->ResourceTag, comment,
                               root_ep.Entry[0].EntityLocation);

        /* set up our private data store for resource state info */
        if (!privdata) {
                privinfo = (struct simResourceInfo *)g_malloc0(sizeof(struct simResourceInfo));
                if (privinfo == NULL) {
                        dbg("Out of memory in build_rptcache\n");
                        return SA_ERR_HPI_OUT_OF_MEMORY;
                }
                if (res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP) {
                        privinfo->cur_hsstate = SAHPI_HS_STATE_ACTIVE;
                }
                privdata = (void *)privinfo;
        }

        /* perform the injection */
        dbg("Injecting ResourceId %d", res->ResourceId);
        rc = oh_add_resource(state->rptcache, res, privdata, FREE_RPT_DATA);
        if (rc) {
                dbg("Error %d injecting ResourceId %d", rc, res->ResourceId);
                return rc;
        }

        /* make sure the caller knows what resiurce ID was actually assigned */
        data->ResourceId = res->ResourceId;

        /* now add an event for the resource add */
        memset(&event, 0, sizeof(event));
        event.type = OH_ET_RESOURCE;
        event.did = oh_get_default_domain_id();
        memcpy(&event.u.res_event.entry, res, sizeof(SaHpiRptEntryT));
        sim_inject_event(state, eventdup(&event));

        return SA_OK;
}


/* inject an rdr */
// assuptions about the input SaHpiRdrT *data entry
// - all fields are assumed to have valid values
// - no checking of the data is performed
// assuptions about the input *privdata entry
// - no checking of the data is performed
SaErrorT sim_inject_rdr(struct oh_handler_state *state, SaHpiResourceIdT resid,
                        SaHpiRdrT *rdr, void * privinfo) {
	struct oh_event event;
        SaErrorT rc;

        /* check arguments */
        if (state == NULL || resid == 0 || rdr == NULL) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        /* perform the injection */
        dbg("Injecting rdr for ResourceId %d", resid);
	rc = oh_add_rdr(state->rptcache, resid, rdr, privinfo, 0);
        if (rc) {
                dbg("Error %d injecting rdr for ResourceId %d", rc, resid);
                return rc;
        }

        /* now inject an event for the rdr */
        memset(&event, 0, sizeof(event));
        event.type = OH_ET_RDR;
        event.u.rdr_event.parent = resid;
        memcpy(&event.u.rdr_event.rdr, rdr, sizeof(SaHpiRdrT));
        sim_inject_event(state, eventdup(&event));

        return SA_OK;
}


/* inject an event */
// assuptions about the input oh_event *data entry
// - all fields are assumed to have valid values
// - no checking of the data is performed
SaErrorT sim_inject_event(struct oh_handler_state *state, struct oh_event *data) {

        /* check arguments */
        if (state== NULL || data == NULL) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        /* perform the injection */
        dbg("Injecting event");
	g_async_queue_push(state->eventq_async, data);
        return SA_OK;
}

