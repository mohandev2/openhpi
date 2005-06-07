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
 *	  Christina Hernandez <hernanc@us.ibm.com>
 *        W. David Ashley <dashley@us.ibm.com>
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


void *sim_open(GHashTable *handler_config)
{
        struct oh_handler_state *state = NULL;
        char *tok = NULL;
        if (!handler_config) {
                dbg("GHashTable *handler_config is NULL!");
                return NULL;
        }
        tok = g_hash_table_lookup(handler_config, "entity_root");

        if (!tok) {
                dbg("entity_root is needed and not present");
                return NULL;
        }

//      trace("%s, %s, %s",
//            (char *)g_hash_table_lookup(handler_config, "plugin"),
//            (char *)g_hash_table_lookup(handler_config, "name"),
//            tok);

        state = g_malloc0(sizeof(struct oh_handler_state));
        if (!state) {
                dbg("out of memory");
                return NULL;
        }

        /* save the handler config hash table it holds  */
        /* the openhpi.conf file config info            */
        state->config = handler_config;

        /* initialize hashtable pointer */
        state->rptcache = (RPTable *)g_malloc0(sizeof(RPTable));
        oh_init_rpt(state->rptcache);

        if (!(state->eventq_async = g_async_queue_new())) {
                dbg("g_async_queue_new failed\n");
                g_free(state->rptcache);
                g_free(state);
                return NULL;
        }

        return (void *)state;
}


SaErrorT sim_discover(void *hnd)
{
	struct oh_handler_state *inst = (struct oh_handler_state *) hnd;
	struct oh_event event;
	SaHpiRptEntryT *rpt_entry;
	SaHpiEntityPathT root_ep;
	char *entity_root;
	SaHpiRdrT *rdr_entry;

	struct oh_handler_state *oh_hnd = hnd;
	entity_root = (char *)g_hash_table_lookup(oh_hnd->config,"entity_root");
	oh_encode_entitypath (entity_root, &root_ep);

        build_rptcache(inst->rptcache, &root_ep);
	sim_discover_sensors(inst->rptcache);
	sim_discover_controls(inst->rptcache);

	rpt_entry = oh_get_resource_next(inst->rptcache, SAHPI_FIRST_ENTRY);

	while (rpt_entry) {
		dbg("here resource event id %d", rpt_entry->ResourceId);
		memset(&event, 0, sizeof(event));
		event.type = OH_ET_RESOURCE;
		event.did = oh_get_default_domain_id();
		memcpy(&event.u.res_event.entry, rpt_entry, sizeof(SaHpiRptEntryT));
		g_async_queue_push(inst->eventq_async, eventdup(&event));

		rdr_entry = oh_get_rdr_next(inst->rptcache, rpt_entry->ResourceId, SAHPI_FIRST_ENTRY);
		if(!rdr_entry){printf("I don't work\n");}
		while (rdr_entry) {
			// dbg("here rdr event id %d", rdr_entry->RecordId);
			memset(&event, 0, sizeof(event));
			event.type = OH_ET_RDR;
			event.u.rdr_event.parent = rpt_entry->ResourceId;
			memcpy(&event.u.rdr_event.rdr, rdr_entry, sizeof(SaHpiRdrT));
			g_async_queue_push(inst->eventq_async, eventdup(&event));

			rdr_entry = oh_get_rdr_next(inst->rptcache, rpt_entry->ResourceId, rdr_entry->RecordId);
		}
		rpt_entry = oh_get_resource_next(inst->rptcache, rpt_entry->ResourceId);
        }

	return 0;
}


int sim_get_event(void *hnd, struct oh_event *event)
{
	struct oh_handler_state *state = hnd;
	struct oh_event *e = NULL;

	if ((e = g_async_queue_try_pop(state->eventq_async))) {
		trace("retrieving sim event from async q");
		memcpy(event, e, sizeof(*event));
		event->did = oh_get_default_domain_id();
		g_free(e);
		return 1;
	} else {
		trace("no more events for sim instance");
        }
        return 0;
}


void sim_close(void *hnd)
{
	struct oh_handler_state *state = hnd;

        /* TODO: we may need to do more here than just this! */
        g_free(state->rptcache);
        g_free(state);
        return;
}


SaErrorT build_rptcache(RPTable *rptcache, SaHpiEntityPathT *root_ep)
{
	int i;
	SaHpiRptEntryT res;
	int x = 0;
	//struct oh_event *e;
	//struct SensorMoreInfo *info;

        //e = (struct oh_event *)g_malloc0(sizeof(struct oh_event));
	// info = (struct SensorMoreInfo *)g_malloc0(sizeof(struct SensorMoreInfo));

	while (sim_rpt_array[x].rpt.ResourceInfo.ManufacturerId != 0){
		x++;
	}

	for(i = 0; i < x; i++){
        	memcpy(&res, &sim_rpt_array[i].rpt, sizeof(SaHpiRptEntryT));
		oh_concat_ep(&res.ResourceEntity, root_ep);
		res.ResourceId = oh_uid_from_entity_path(&res.ResourceEntity);
		printf("I am res.ResourceId %d\n", res.ResourceId);
		dbg("Adding resource number %d",i);

	        sim_create_resourcetag(&res.ResourceTag, sim_rpt_array[i].comment, root_ep->Entry[i].EntityLocation);
                oh_add_resource(rptcache, &res, NULL, FREE_RPT_DATA);

	}
        return i;
}


/*
 * Simulator function table for plugin interface
 *
 */

static struct oh_abi_v2 oh_sim_plugin = {
        .open                   	= sim_open,
        .close                  	= sim_close,
        .get_event              	= sim_get_event,
        .discover_resources     	= sim_discover,
        .discover_domain_resources     	= NULL,
        .set_resource_tag              	= NULL,
        .set_resource_severity         	= NULL,
        .get_el_info                   	= NULL,
        .set_el_time                   	= NULL,
        .add_el_entry                  	= NULL,
        .get_el_entry                  	= NULL,
        .clear_el                      	= NULL,
        .set_el_state                  	= NULL,
        .reset_el_overflow             	= NULL,
	.get_sensor_reading     	= sim_get_sensor_reading,
        .get_sensor_thresholds  	= sim_get_sensor_thresholds,
	.set_sensor_thresholds		= sim_set_sensor_thresholds,
	.get_sensor_enable		= sim_get_sensor_enable,
	.set_sensor_enable              = sim_set_sensor_enable,
	.get_sensor_event_enables       = sim_get_sensor_event_enable,
	.set_sensor_event_enables       = sim_set_sensor_event_enable,
	.get_sensor_event_masks         = sim_get_sensor_event_masks,
	.set_sensor_event_masks         = sim_set_sensor_event_masks,
	.get_control_state		= sim_get_control_state,
	.set_control_state		= sim_set_control_state,
	.get_idr_info     		= NULL,
	.get_idr_area_header            = NULL,
	.add_idr_area                   = NULL,
	.del_idr_area                   = NULL,
	.get_idr_field                  = NULL,
	.add_idr_field                  = NULL,
	.set_idr_field                  = NULL,
	.del_idr_field                  = NULL,
	.get_watchdog_info              = NULL,
	.set_watchdog_info              = NULL,
	.reset_watchdog                 = NULL,
	.get_next_announce              = NULL,
	.get_announce                   = NULL,
	.ack_announce                   = NULL,
	.add_announce                   = NULL,
	.del_announce                   = NULL,
	.get_annunc_mode                = NULL,
	.set_annunc_mode                = NULL,
	.get_hotswap_state              = NULL,
	.set_hotswap_state              = NULL,
	.request_hotswap_action         = NULL,
	.get_power_state                = NULL,
	.set_power_state                = NULL,
	.get_indicator_state            = NULL,
	.set_indicator_state            = NULL,
	.control_parm                   = NULL,
	.get_reset_state                = NULL,
	.set_reset_state                = NULL
};


int sim_get_interface(void **pp, const uuid_t uuid)
{
        if (uuid_compare(uuid, UUID_OH_ABI_V2) == 0) {
                *(struct oh_abi_v2 **)pp = &oh_sim_plugin;
                return 0;
        }

        *pp = NULL;
        return -1;
}


int get_interface(void **pp, const uuid_t uuid)
                  __attribute__ ((weak, alias("sim_get_interface")));

