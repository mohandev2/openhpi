/*      -*- linux-c -*-
 *
 * Copyright (c) 2003 by Intel Corp.
 * (C) Copyright IBM Corp. 2003
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Authors:
 *     Louis Zhuang <louis.zhuang@linux.intel.com>
 *     Sean Dague <sean@dague.net>
 *     Rusty Lynch
 *     David Judkovics <djudkovi@us.ibm.com>  
 */

#include <stdlib.h>
#include <string.h>
#include <oh_config.h>
#include <openhpi.h>
#include <oh_plugin.h>
#include <SaHpi.h>
#include <uid_utils.h>

#include <pthread.h>



static enum {
        OH_STAT_UNINIT,
        OH_STAT_READY,
} oh_hpi_state = OH_STAT_UNINIT;
static const int entry_id_offset = 1000;

#define OH_STATE_READY_CHECK                                    \
        do {                                                    \
                    if (OH_STAT_READY!=oh_hpi_state) {          \
                        dbg("Uninitialized HPI");               \
                        return SA_ERR_HPI_UNINITIALIZED;        \
                }                                               \
        }while(0)                                               

static inline struct oh_rdr * get_rdr(
        struct oh_resource *res, 
        SaHpiRdrTypeT type, 
        SaHpiUint8T num)
{
        GSList *i;

        data_access_lock();
        
        g_slist_for_each(i, res->rdr_list) {
                struct oh_rdr *rdr;
                rdr = i->data;
                
                if (rdr->rdr.RdrType != type)
                        continue;
                
                switch (type) {
                case SAHPI_CTRL_RDR:
                        if (rdr->rdr.RdrTypeUnion.CtrlRec.Num == num) {
                                data_access_unlock();
                                return rdr;
                        }
                        break;
                case SAHPI_SENSOR_RDR:
                        if (rdr->rdr.RdrTypeUnion.SensorRec.Num == num) {
                                data_access_unlock();
                                return rdr;
                        }
                        break;
                case SAHPI_INVENTORY_RDR:
                        if (rdr->rdr.RdrTypeUnion.InventoryRec.EirId == num) {
                                data_access_unlock();
                                return rdr;
                        }
                        break;
                case SAHPI_WATCHDOG_RDR:
                        if (rdr->rdr.RdrTypeUnion.WatchdogRec.WatchdogNum == num) {
                                data_access_unlock();
                                return rdr;
                        }
                        break;
                case SAHPI_NO_RECORD:
                default:
                        break;                  
                }
        }

        data_access_unlock();
        
        return NULL;
}

SaErrorT SAHPI_API saHpiInitialize(SAHPI_OUT SaHpiVersionT *HpiImplVersion)
{
        struct oh_plugin_config *tmpp;
        GHashTable *tmph;
      
        unsigned int i;
        char *openhpi_conf;
        
        /* initialize mutex used for data locking */
        /* in the future may want to add seperate */
        /* mutexes, one for each hash list        */
        data_access_lock();
        
        /* setup our global rpt_table */
        default_rpt = g_malloc0(sizeof(RPTable));
        if(!default_rpt) {
                dbg("Couldn't allocate RPT for Default Domain");
                return SA_ERR_HPI_ERROR;
        }

	/* initialize uid_utils, and load uid map file if present */
	if( oh_uid_initialize() ) {
		dbg("uid_intialization failed");
		return(SA_ERR_HPI_ERROR);
	}

        if (OH_STAT_UNINIT != oh_hpi_state) {
                dbg("Cannot initialize twice");
                data_access_unlock();
                return SA_ERR_HPI_DUPLICATE;
        }
        
        openhpi_conf = getenv("OPENHPI_CONF");
        
        if (openhpi_conf == NULL) {
                openhpi_conf = OH_DEFAULT_CONF;
        }       
        
        if (oh_load_config(openhpi_conf) < 0) {
                dbg("Can not load config");
                data_access_unlock();
                return SA_ERR_HPI_NOT_PRESENT;
        }
        
        for(i = 0; i < g_slist_length(global_plugin_list); i++) {
                tmpp = (struct oh_plugin_config *) g_slist_nth_data(
                        global_plugin_list, i);
                if(load_plugin(tmpp) == 0) {
                        dbg("Loaded plugin %s", tmpp->name);
                        tmpp->refcount++;
                } else {
                        dbg("Couldn't load plugin %s", tmpp->name);
                }
        }
        
        for(i = 0; i < g_slist_length(global_handler_configs); i++) {
                tmph = (GHashTable *) g_slist_nth_data(
                        global_handler_configs, i);
                if(plugin_refcount((char *)g_hash_table_lookup(tmph, "plugin")) > 0) {
                        if(load_handler(tmph) == 0) {
                                dbg("Loaded handler for plugin %s",
                                        (char *)g_hash_table_lookup(tmph, "plugin"));
                        } else {
                                dbg("Couldn't load handler for plugin %s",
                                        (char *)g_hash_table_lookup(tmph, "plugin"));
                        }
                } else {
                        dbg("load handler for unknown plugin %s",
                                (char *)g_hash_table_lookup(tmph, "plugin"));
                }
        } 
        
        oh_hpi_state = OH_STAT_READY;

        data_access_unlock();

        return SA_OK;
}

SaErrorT SAHPI_API saHpiFinalize(void)
{
        OH_STATE_READY_CHECK;

        /* free mutex */
	/* TODO: this wasn't here in bracnk, need to resolve history */
        data_access_lock();

	/* TODO: realy should have a oh_uid_finalize() that */
	/* frees memory,				    */
	if(oh_uid_map_to_file())
		dbg("error writing uid entity path mapping to file");    
        /*
          we should be doing handler shutdown here.
        */
        oh_hpi_state = OH_STAT_UNINIT;

        /* free mutex */
        data_access_unlock();

        return SA_OK;
}

SaErrorT SAHPI_API saHpiSessionOpen(
                SAHPI_IN SaHpiDomainIdT DomainId,
                SAHPI_OUT SaHpiSessionIdT *SessionId,
                SAHPI_IN void *SecurityParams)
{
        struct oh_session *s;
        int rv;
        
        OH_STATE_READY_CHECK;
        
        if(!is_in_domain_list(DomainId)) {
                dbg("domain does not exist!");
                return SA_ERR_HPI_INVALID_DOMAIN;
        }
        
        rv = session_add(DomainId, &s);
        if(rv < 0) {
                dbg("Out of space");
                return SA_ERR_HPI_OUT_OF_SPACE;
        }
        
        *SessionId = s->session_id;
        
        return SA_OK;
}


SaErrorT SAHPI_API saHpiSessionClose(SAHPI_IN SaHpiSessionIdT SessionId)
{
        struct oh_session *s;
        
        OH_STATE_READY_CHECK;
        
        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }
        
        session_del(s); 
        return SA_OK;
}


SaErrorT SAHPI_API saHpiResourcesDiscover(SAHPI_IN SaHpiSessionIdT SessionId)
{
        struct oh_session *s;
        GSList *i;
        int rv =0;

        data_access_lock();
        
        OH_STATE_READY_CHECK;

        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                data_access_unlock();
                return SA_ERR_HPI_INVALID_SESSION;
        }
        
        g_slist_for_each(i, global_handler_list) {
                struct oh_handler *h = i->data;
                rv |= h->abi->discover_resources(h->hnd);
        }
        if (rv) {
                dbg("Error attempting to discover resource");  
                data_access_unlock();
                return SA_ERR_HPI_UNKNOWN;
        }
        
        rv = get_events();
        if (rv<0) {
                dbg("Error attempting to process resources");
                data_access_unlock();
                return SA_ERR_HPI_UNKNOWN;
        }

        data_access_unlock();
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiRptInfoGet(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_OUT SaHpiRptInfoT *RptInfo)
{
        int rv =0;

        OH_STATE_READY_CHECK;

        rv = get_events();
        if (rv<0) {
                dbg("Error attempting to process events");
                return SA_ERR_HPI_UNKNOWN;
        }
        
        /* FIXME: we should really be getting event default_rpt from 
           a domain or session keyed hash */
        RptInfo->UpdateCount = default_rpt->rpt_info.UpdateCount;
        RptInfo->UpdateTimestamp= default_rpt->rpt_info.UpdateTimestamp;
        return SA_OK;
}

SaErrorT SAHPI_API saHpiRptEntryGet(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiEntryIdT EntryId,
                SAHPI_OUT SaHpiEntryIdT *NextEntryId,
                SAHPI_OUT SaHpiRptEntryT *RptEntry)
{
        struct oh_session *s;

        /* determine the right pointer later when we do multi domains */
        RPTable *rpt = default_rpt;
        SaHpiRptEntryT *req_entry;
        SaHpiRptEntryT *next_entry;
        
        OH_STATE_READY_CHECK;

        data_access_lock();

        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                data_access_unlock();
                return SA_ERR_HPI_INVALID_SESSION;
        }
        
        if (EntryId == SAHPI_FIRST_ENTRY) {
                req_entry = oh_get_resource_next(rpt, RPT_ENTRY_BEGIN);
        } else {
                req_entry = oh_get_resource_by_id(rpt, EntryId);
        }

        /* if the entry was NULL, clearly have an issue */
        if(req_entry == NULL) {
                dbg("Invalid EntryId");
                data_access_unlock();
                return SA_ERR_HPI_INVALID;
        }
        
        memcpy(RptEntry, req_entry, sizeof(RptEntry));

        next_entry = oh_get_resource_next(rpt, req_entry->EntryId);
        
        if(next_entry != NULL) {
                *NextEntryId = next_entry->EntryId;
        } else {
                *NextEntryId = SAHPI_LAST_ENTRY;
        }
        
        data_access_unlock();
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiRptEntryGetByResourceId(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_OUT SaHpiRptEntryT *RptEntry)
{
        RPTable *rpt = default_rpt;
        SaHpiRptEntryT *req_entry;

        OH_STATE_READY_CHECK;

        data_access_lock();

        req_entry = oh_get_resource_by_id(rpt, ResourceId);

        if(req_entry == NULL) {
                dbg("No such resource id");
                data_access_unlock();
                return SA_ERR_HPI_INVALID;
        }
        
        memcpy(RptEntry, req_entry, sizeof(*RptEntry));
        
        data_access_unlock();
        return SA_OK;
}

SaErrorT SAHPI_API saHpiResourceSeveritySet(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiSeverityT Severity)
{
        /* this requires a new abi call to push down to the plugin */
        return SA_ERR_HPI_UNKNOWN;
}

SaErrorT SAHPI_API saHpiResourceTagSet(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiTextBufferT *ResourceTag)
{
        /* this requires a new abi call to push down to the plugin */
        return SA_ERR_HPI_UNKNOWN;
}

SaErrorT SAHPI_API saHpiResourceIdGet(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_OUT SaHpiResourceIdT *ResourceId)
{
        /* this is going to be an interesting call to figure out */
        return SA_ERR_HPI_UNKNOWN;
}

SaErrorT SAHPI_API saHpiEntitySchemaGet(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_OUT SaHpiUint32T *SchemaId)
{
        /* we must return 0 for now, as we don't determine 
           schemas yet */
        *SchemaId = 0;
        return SA_OK;
}


SaErrorT SAHPI_API saHpiEventLogInfoGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_OUT SaHpiSelInfoT *Info)
{
        RPTable *rpt = default_rpt;
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        struct oh_session *s;
        
        OH_STATE_READY_CHECK;
        
        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }       
        
        /*
          Waiting for the right way to do DSELS 
          
        if (ResourceId==SAHPI_DOMAIN_CONTROLLER_ID) {
                if(dsel_get_info(s->domain_id, Info)<0)
                        return SA_ERR_HPI_UNKNOWN;
                return SA_OK;
        }
        */
        
        h = oh_get_resource_data(rpt, ResourceId);
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_RESOURCE;
        }
        
        res = oh_get_resource_by_id(rpt, ResourceId);
        if (!res) {
                dbg("Invalid resource");
                return SA_ERR_HPI_INVALID_RESOURCE;
        }                                               
        
        if(!(res->ResourceCapabilities & SAHPI_CAPABILITY_SEL)) {
                dbg("Can't use SEL functions on this resource");
                return SA_ERR_HPI_INVALID_REQUEST;
        }

        if (h->abi->get_sel_info(h->hnd, ResourceId, Info)<0)
                return SA_ERR_HPI_UNKNOWN;
        return SA_OK;
}

#if 0

SaErrorT SAHPI_API saHpiEventLogEntryGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiSelEntryIdT EntryId,
                SAHPI_OUT SaHpiSelEntryIdT *PrevEntryId,
                SAHPI_OUT SaHpiSelEntryIdT *NextEntryId,
                SAHPI_OUT SaHpiSelEntryT *EventLogEntry,
                SAHPI_INOUT SaHpiRdrT *Rdr,
                SAHPI_INOUT SaHpiRptEntryT *RptEntry)
{
        struct oh_session *s;
        GSList *sel_list;

        OH_STATE_READY_CHECK;
        
        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }       
        
        if (ResourceId==SAHPI_DOMAIN_CONTROLLER_ID) {
                struct oh_domain *d;
                
                d = get_domain_by_id(s->domain_id);
                if (!d) {
                        dbg("Internal error?!");
                        sel_list = NULL;
                } else 
                        sel_list = d->sel_list;
                
                return process_sel_entry(sel_list, EntryId, 
                                PrevEntryId, NextEntryId, 
                                EventLogEntry, Rdr, RptEntry, &dsel_ops);       
        } else {
                struct oh_resource *res;

                res = get_resource(ResourceId);
                if (!res) {
                        dbg("Internal error?!");
                        sel_list = NULL;
                } else
                        sel_list = res->sel_list;
                return process_sel_entry(sel_list, EntryId, 
                                PrevEntryId, NextEntryId, 
                                EventLogEntry, Rdr, RptEntry, &rsel_ops);       
        }
}

SaErrorT SAHPI_API saHpiEventLogEntryAdd (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiSelEntryT *EvtEntry)
{
        struct oh_session *s;
        
        OH_STATE_READY_CHECK;
        
        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }       
        
        if (ResourceId==SAHPI_DOMAIN_CONTROLLER_ID) {
                if (dsel_add(s->domain_id, EvtEntry)<0)
                        return SA_ERR_HPI_UNKNOWN;
                else 
                        return SA_OK;
        }
        
        /* rsel_add will make plug-in send a RSEL event 
         * for the entry */
        if (rsel_add(ResourceId, EvtEntry)<0)
                return SA_ERR_HPI_UNKNOWN;
        
        /* to get RSEL evtry into infrastructure */
        get_events();
        return SA_OK;
}

SaErrorT SAHPI_API saHpiEventLogEntryDelete (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiSelEntryIdT EntryId)
{
        struct oh_session *s;
        
        OH_STATE_READY_CHECK;
        
        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }       
        
        if (ResourceId==SAHPI_DOMAIN_CONTROLLER_ID) {
                if (dsel_del(s->domain_id, EntryId)<0)
                        return SA_ERR_HPI_UNKNOWN;
                else
                        return SA_OK;
        }

        if (rsel_del(ResourceId, EntryId)<0)
                return SA_ERR_HPI_UNKNOWN;
        else
                return SA_OK;
}

SaErrorT SAHPI_API saHpiEventLogClear (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId)
{
        struct oh_session *s;
        
        OH_STATE_READY_CHECK;
        
        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }       
        
        if (ResourceId==SAHPI_DOMAIN_CONTROLLER_ID) {
                if (dsel_clr(s->domain_id)<0)
                        return SA_ERR_HPI_UNKNOWN;
                else
                        return SA_OK;
        }

        if (rsel_clr(ResourceId)<0)
                return SA_ERR_HPI_UNKNOWN;
        return SA_OK;
}

SaErrorT SAHPI_API saHpiEventLogTimeGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_OUT SaHpiTimeT *Time)
{
        struct oh_session *s;
        struct oh_resource *res;
        SaHpiSelInfoT info;
        
        OH_STATE_READY_CHECK;
        
        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }       
        
        if (ResourceId==SAHPI_DOMAIN_CONTROLLER_ID) {
                *Time = dsel_get_time(s->domain_id);
                return SA_OK;
        }
        
        res = get_resource(ResourceId);
        if (!res) {
                dbg("Invalid resource");
                return SA_ERR_HPI_INVALID_RESOURCE;
        }                                               
        
        if (res->handler->abi->get_sel_info(res->handler->hnd, res->oid, &info)<0)
                return SA_ERR_HPI_UNKNOWN;
        *Time = info.CurrentTime;
        return SA_OK;
}

SaErrorT SAHPI_API saHpiEventLogTimeSet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiTimeT Time)
{
        struct oh_session *s;
        struct oh_resource *res;
        struct timeval time;
        
        OH_STATE_READY_CHECK;
        
        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }       
        
        if (ResourceId==SAHPI_DOMAIN_CONTROLLER_ID) {
                dsel_set_time(s->domain_id, Time);
                return SA_OK;
        }

        res = get_resource(ResourceId);
        if (!res) {
                dbg("Invalid resource");
                return SA_ERR_HPI_INVALID_RESOURCE;
        }                                               
        
        time.tv_sec  = Time / 1000000000;
        time.tv_usec = Time % 1000000000 * 1000;
        if (res->handler->abi->set_sel_time(res->handler->hnd, res->oid, &time)<0)
                return SA_ERR_HPI_UNKNOWN;
        return SA_OK;
}

SaErrorT SAHPI_API saHpiEventLogStateGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_OUT SaHpiBoolT *Enable)
{
        struct oh_session *s;
        struct oh_resource *res;
        SaHpiSelInfoT info;

        OH_STATE_READY_CHECK;
        
        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }       
        
        if (ResourceId==SAHPI_DOMAIN_CONTROLLER_ID) {
                *Enable = dsel_get_state(s->domain_id);
                return SA_OK;
        }

        res = get_resource(ResourceId);
        if (!res) {
                dbg("Invalid resource");
                return SA_ERR_HPI_INVALID_RESOURCE;
        }                                               
        
        if (res->handler->abi->get_sel_info(res->handler->hnd, res->oid, &info)<0)
                return SA_ERR_HPI_UNKNOWN;
        *Enable = info.Enabled;
        return SA_OK;
}

SaErrorT SAHPI_API saHpiEventLogStateSet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiBoolT Enable)
{
        struct oh_session *s;
        struct oh_resource *res;
        
        OH_STATE_READY_CHECK;
        
        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }       
        
        if (ResourceId==SAHPI_DOMAIN_CONTROLLER_ID) {
                if (dsel_set_state(s->domain_id, Enable)<0)
                        return SA_ERR_HPI_UNKNOWN;
                else
                        return SA_OK;
        }

        res = get_resource(ResourceId);
        if (!res) {
                dbg("Invalid resource");
                return SA_ERR_HPI_INVALID_RESOURCE;
        }                                               
        
        if (res->handler->abi->set_sel_state(res->handler->hnd, res->oid, Enable)<0)
                return SA_ERR_HPI_UNKNOWN;
        return SA_OK;
}

#endif
SaErrorT SAHPI_API saHpiSubscribe (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiBoolT ProvideActiveAlarms)
{
        struct oh_session *s;
        
        OH_STATE_READY_CHECK;
        
        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }
        
        if (s->event_state == OH_EVENT_SUBSCRIBE) {
                dbg("Duplicate subscribe");
                return SA_ERR_HPI_DUPLICATE;
        }
        
        if (ProvideActiveAlarms) {
                /*FIXME: nothing to do here?! */
        }

        s->event_state = OH_EVENT_SUBSCRIBE;
        return SA_OK;
}


SaErrorT SAHPI_API saHpiUnsubscribe (
                SAHPI_IN SaHpiSessionIdT SessionId)
{
        struct oh_session *s;
        
        OH_STATE_READY_CHECK;
        
        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }

        if (s->event_state == OH_EVENT_UNSUBSCRIBE) {
                dbg("Duplicate subscribe");
                return SA_ERR_HPI_INVALID_REQUEST;
        }
        
        s->event_state = OH_EVENT_UNSUBSCRIBE;
        return SA_OK;
}

SaErrorT SAHPI_API saHpiEventGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiTimeoutT Timeout,
                SAHPI_OUT SaHpiEventT *Event,
                SAHPI_INOUT SaHpiRdrT *Rdr,
                SAHPI_INOUT SaHpiRptEntryT *RptEntry)
{
        RPTable *rpt = default_rpt;
        struct oh_session *s;
        SaHpiTimeT now, end;
        OH_STATE_READY_CHECK;
        int value;

        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }
        
        gettimeofday1(&now);
        if (Timeout== SAHPI_TIMEOUT_BLOCK) {
                end = now + (SaHpiTimeT)10000*1000000000; /*set a long time*/
        } else {
                end = now + Timeout;
        }
        
        while (1) {
                int rv;
                
                rv = get_events();
                if (rv<0) {
                        value = SA_ERR_HPI_UNKNOWN;
                        break;
                }
                
                if (session_has_event(s)) {
                        value = SA_OK;
                        break;
                }
                
                gettimeofday1(&now);    
                if (now>=end) {
                        value = SA_ERR_HPI_TIMEOUT;
                        break;
                }
        }
        
        //dbg("now=%lld, end=%lld", now, end);
        
        if (value==SA_OK) {
                struct oh_hpi_event e;
                SaHpiRptEntryT *res;
                SaHpiRdrT *rdr;
                
                if (session_pop_event(s, &e)<0) {
                        dbg("Empty event queue?!");
                        return SA_ERR_HPI_UNKNOWN;
                }

                memcpy(Event, &e.event, sizeof(*Event));
                
                if (Rdr)
                        Rdr->RdrType = SAHPI_NO_RECORD;

                if (RptEntry)
                        RptEntry->ResourceCapabilities = 0;

                res = oh_get_resource_by_id(rpt,e.parent);

                if (res) {
                        if (Rdr) {
                                rdr = oh_get_rdr_by_id(rpt,res->ResourceId,e.id);
                                if (rdr) {
                                        memcpy(Rdr, rdr, sizeof(*Rdr));
                                } else {
                                        dbg("Event without resource");
                                }
                        }

                        if (RptEntry) {
                                memcpy(RptEntry, res, sizeof(*RptEntry));
                        }
                } else {
                        dbg("Event without resource");  
                }
        }
        
        return value;

}


SaErrorT SAHPI_API saHpiRdrGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiEntryIdT EntryId,
                SAHPI_OUT SaHpiEntryIdT *NextEntryId,
                SAHPI_OUT SaHpiRdrT *Rdr)
{
        RPTable *rpt = default_rpt;
        SaHpiRdrT *rdr_cur;
        SaHpiRdrT *rdr_next;

        data_access_lock();
        
        if(EntryId == SAHPI_FIRST_ENTRY) {
                rdr_cur = oh_get_rdr_next(rpt, ResourceId, RDR_BEGIN);
        } else {
                rdr_cur = oh_get_rdr_by_id(rpt, ResourceId, EntryId);
        }

        if(rdr_cur == NULL) {
                dbg("No RDR available");
                data_access_unlock();
                return SA_ERR_HPI_INVALID;
        }
        
        memcpy(Rdr, rdr_cur, sizeof(*Rdr));

        rdr_next = oh_get_rdr_next(rpt, ResourceId, rdr_cur->RecordId);
        if(rdr_next == NULL) {
                *NextEntryId = SAHPI_LAST_ENTRY;
        } else {
                *NextEntryId = rdr_next->RecordId;
        }
        
        data_access_unlock();
        
        return SA_OK;
}

/* Sensor data functions */

SaErrorT SAHPI_API saHpiSensorReadingGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiSensorNumT SensorNum,
                SAHPI_OUT SaHpiSensorReadingT *Reading)
{
        int (*get_func) (void *, SaHpiResourceIdT, SaHpiSensorNumT, SaHpiSensorReadingT *);
        
        RPTable *rpt = default_rpt;
        struct oh_handler *h;

        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        get_func = h->abi->get_sensor_data;

        if (!get_func)
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (get_func(h->hnd, ResourceId, SensorNum, Reading))
                return SA_ERR_HPI_UNKNOWN;

        return SA_OK;
}

SaErrorT SAHPI_API saHpiSensorReadingConvert (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiSensorNumT SensorNum,
                SAHPI_IN SaHpiSensorReadingT *ReadingInput,
                SAHPI_OUT SaHpiSensorReadingT *ConvertedReading)
{
        RPTable *rpt = default_rpt;
        SaHpiRdrT *rdr;
        SaHpiSensorRecT *sensor;
        SaHpiSensorReadingFormatsT format;

        rdr = oh_get_rdr_by_type(rpt, ResourceId, SAHPI_SENSOR_RDR, SensorNum);

        if (!rdr)
                return SA_ERR_HPI_NOT_PRESENT;

        sensor = &(rdr->RdrTypeUnion.SensorRec);

        /* if ReadingInput neither contains a raw nor a intepreted value or
           if it contains both, return an error */
        format = ReadingInput->ValuesPresent
                & (SAHPI_SRF_RAW|SAHPI_SRF_INTERPRETED);
        
        if (format == 0 || format == (SAHPI_SRF_RAW|SAHPI_SRF_INTERPRETED))
                return SA_ERR_HPI_INVALID_PARAMS;

        /* if the sensor does not supports raw and interpreted values,
           return an error */
        format = sensor->DataFormat.ReadingFormats
                 & (SAHPI_SRF_RAW|SAHPI_SRF_INTERPRETED);

        if (format != (SAHPI_SRF_RAW|SAHPI_SRF_INTERPRETED))
                return SA_ERR_HPI_INVALID_DATA;

        /* cannot convert non numeric values */
        if (!sensor->DataFormat.IsNumeric)
                return SA_ERR_HPI_INVALID_DATA;

        /* if the conversion factors vary over sensor
           range, return an error */ 
        if (!sensor->DataFormat.FactorsStatic)
                return  SA_ERR_HPI_INVALID_DATA;

        if (ReadingInput->ValuesPresent == SAHPI_SRF_RAW) {
                ConvertedReading->ValuesPresent = SAHPI_SRF_INTERPRETED;
                ConvertedReading->Interpreted.Type = 
                        SAHPI_SENSOR_INTERPRETED_TYPE_FLOAT32;

                return sensor_convert_from_raw(sensor,
                                               ReadingInput->Raw,
                                               &ConvertedReading->Interpreted
                                                .Value.SensorFloat32);
        }

        /* only float is supported */
        if (   ReadingInput->Interpreted.Type
            != SAHPI_SENSOR_INTERPRETED_TYPE_FLOAT32)
                return SA_ERR_HPI_INVALID_PARAMS;

        ConvertedReading->ValuesPresent = SAHPI_SRF_RAW;

        return sensor_convert_to_raw(sensor,
                                     ReadingInput->Interpreted.Value
                                      .SensorFloat32,
                                     &ConvertedReading->Raw);
}

SaErrorT SAHPI_API saHpiSensorThresholdsSet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiSensorNumT SensorNum,
                SAHPI_OUT SaHpiSensorThresholdsT *SensorThresholds)
{
        int (*set_func) (void *, SaHpiResourceIdT, SaHpiSensorNumT, const SaHpiSensorThresholdsT *);
        
        RPTable *rpt = default_rpt;
        struct oh_handler *h;

        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        set_func = h->abi->set_sensor_thresholds;

        if (!set_func)
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (set_func(h->hnd, ResourceId, SensorNum, SensorThresholds))
                return SA_ERR_HPI_UNKNOWN;

        return SA_OK;
}

SaErrorT SAHPI_API saHpiSensorThresholdsGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiSensorNumT SensorNum,
                SAHPI_IN SaHpiSensorThresholdsT *SensorThresholds)
{
        int (*get_func) (void *, SaHpiResourceIdT, SaHpiSensorNumT, SaHpiSensorThresholdsT *);

        RPTable *rpt = default_rpt;
        struct oh_handler *h;

        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        get_func = h->abi->get_sensor_thresholds;

        if (!get_func)
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (get_func(h->hnd, ResourceId, SensorNum, SensorThresholds))
                return SA_ERR_HPI_UNKNOWN;

        return SA_OK;
}

/* Function: SaHpiSensorTypeGet */
/* Core SAF_HPI function */
/* Not mapped to plugin */
/* Data in RDR */
SaErrorT SAHPI_API saHpiSensorTypeGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiSensorNumT SensorNum,
                SAHPI_OUT SaHpiSensorTypeT *Type,
                SAHPI_OUT SaHpiEventCategoryT *Category)
{
        RPTable *rpt = default_rpt;
        SaHpiRdrT *rdr;

        rdr = oh_get_rdr_by_type(rpt, ResourceId, SAHPI_SENSOR_RDR, SensorNum);

        
        if (!rdr)
                return SA_ERR_HPI_INVALID_PARAMS;
        
        if (!memcpy(Type, &(rdr->RdrTypeUnion.SensorRec.Type),
                    sizeof(SaHpiSensorTypeT)))
                return SA_ERR_HPI_ERROR;

        if (!memcpy(Category, &(rdr->RdrTypeUnion.SensorRec.Category),
                    sizeof(SaHpiEventCategoryT)))
                return SA_ERR_HPI_ERROR;
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiSensorEventEnablesGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiSensorNumT SensorNum,
                SAHPI_OUT SaHpiSensorEvtEnablesT *Enables)
{
        int (*get_sensor_event_enables)(void *hnd, SaHpiResourceIdT, 
                                        SaHpiSensorNumT,
                                        SaHpiSensorEvtEnablesT *enables);
        
        RPTable *rpt = default_rpt;
        struct oh_handler *h;

        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        get_sensor_event_enables = h->abi->get_sensor_event_enables;
        
        if (!get_sensor_event_enables)
                return SA_ERR_HPI_UNSUPPORTED_API;
        if (get_sensor_event_enables(h->hnd, ResourceId, SensorNum, Enables))
                return SA_ERR_HPI_UNKNOWN;
        return SA_OK;
}

SaErrorT SAHPI_API saHpiSensorEventEnablesSet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiSensorNumT SensorNum,
                SAHPI_IN SaHpiSensorEvtEnablesT *Enables)
{
        int (*set_sensor_event_enables)(void *hnd, SaHpiResourceIdT,
                                        SaHpiSensorNumT,
                                        const SaHpiSensorEvtEnablesT *enables);
        
        RPTable *rpt = default_rpt;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        set_sensor_event_enables = h->abi->set_sensor_event_enables;
        
        if (!set_sensor_event_enables)
                return SA_ERR_HPI_UNSUPPORTED_API;
        if (set_sensor_event_enables(h->hnd, ResourceId, SensorNum, Enables))
                return SA_ERR_HPI_UNKNOWN;
        return SA_OK;
}

/* End Sensor functions */

/* Control data functions */

SaErrorT SAHPI_API saHpiControlTypeGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiCtrlNumT CtrlNum,
                SAHPI_OUT SaHpiCtrlTypeT *Type)
{
        RPTable *rpt = default_rpt;
        SaHpiRdrT *rdr;
        
        rdr = oh_get_rdr_by_type(rpt, ResourceId, SAHPI_CTRL_RDR, CtrlNum);
        if (!rdr)
                return SA_ERR_HPI_INVALID_PARAMS;

        if (!memcpy(Type, &(rdr->RdrTypeUnion.CtrlRec.Type), 
                    sizeof(SaHpiCtrlTypeT)))
                return SA_ERR_HPI_ERROR;

        return SA_OK;
}

SaErrorT SAHPI_API saHpiControlStateGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiCtrlNumT CtrlNum,
                SAHPI_INOUT SaHpiCtrlStateT *CtrlState)
{
        int (*get_func)(void *, SaHpiResourceIdT, SaHpiCtrlNumT, SaHpiCtrlStateT *);
        RPTable *rpt = default_rpt;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        get_func = h->abi->get_control_state;
        if (!get_func)          
                return SA_ERR_HPI_UNSUPPORTED_API;
        
        if (get_func(h->hnd, ResourceId, CtrlNum, CtrlState))
                return SA_ERR_HPI_UNKNOWN;
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiControlStateSet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiCtrlNumT CtrlNum,
                SAHPI_IN SaHpiCtrlStateT *CtrlState)
{
        int (*set_func)(void *, SaHpiResourceIdT, SaHpiCtrlNumT, SaHpiCtrlStateT *);
        RPTable *rpt = default_rpt;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        set_func = h->abi->set_control_state;
        if (!set_func)          
                return SA_ERR_HPI_UNSUPPORTED_API;
        
        if (set_func(h->hnd, ResourceId, CtrlNum, CtrlState))
                return SA_ERR_HPI_UNKNOWN;

        return SA_OK;
}

/* current sahpi.h missed SA_ERR_INVENT_DATA_TRUNCATED */
#ifndef SA_ERR_INVENT_DATA_TRUNCATED
//#warning "No 'SA_ERR_INVENT_DATA_TRUNCATED 'definition in sahpi.h!"
#define SA_ERR_INVENT_DATA_TRUNCATED    (SaErrorT)(SA_HPI_ERR_BASE - 1000)
#endif

SaErrorT SAHPI_API saHpiEntityInventoryDataRead (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiEirIdT EirId,
                SAHPI_IN SaHpiUint32T BufferSize,
                SAHPI_OUT SaHpiInventoryDataT *InventData,
                SAHPI_OUT SaHpiUint32T *ActualSize)
{
        int (*get_size)(void *, SaHpiResourceIdT, SaHpiEirIdT, size_t *);
        int (*get_func)(void *, SaHpiResourceIdT, SaHpiEirIdT, SaHpiInventoryDataT *);
        
        RPTable *rpt = default_rpt;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        get_size = h->abi->get_inventory_size;
        get_func = h->abi->get_inventory_info;
        if (!get_func || !get_size)             
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (get_size(h->hnd, ResourceId, EirId, ActualSize))
                return SA_ERR_HPI_UNKNOWN;
        
        if (*ActualSize>BufferSize)
                return SA_ERR_INVENT_DATA_TRUNCATED;
        
        if (get_func(h->hnd, ResourceId, EirId, InventData))
                return SA_ERR_HPI_UNKNOWN;

        return SA_OK;
}

SaErrorT SAHPI_API saHpiEntityInventoryDataWrite (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiEirIdT EirId,
                SAHPI_IN SaHpiInventoryDataT *InventData)
{
        int (*set_func)(void *, SaHpiResourceIdT, SaHpiEirIdT, const SaHpiInventoryDataT *);

        RPTable *rpt = default_rpt;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        set_func = h->abi->set_inventory_info;
        if (!set_func)          
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (set_func(h->hnd, ResourceId, EirId, InventData))
                return SA_ERR_HPI_UNKNOWN;

        return SA_OK;
}

SaErrorT SAHPI_API saHpiWatchdogTimerGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiWatchdogNumT WatchdogNum,
                SAHPI_OUT SaHpiWatchdogT *Watchdog)
{
        int (*get_func)(void *, SaHpiResourceIdT, SaHpiWatchdogNumT, SaHpiWatchdogT *);
        
        RPTable *rpt = default_rpt;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        get_func = h->abi->get_watchdog_info;
        if (!get_func)          
                return SA_ERR_HPI_UNSUPPORTED_API;
        
        if (get_func(h->hnd, ResourceId, WatchdogNum, Watchdog))
                return SA_ERR_HPI_UNKNOWN;
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiWatchdogTimerSet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiWatchdogNumT WatchdogNum,
                SAHPI_IN SaHpiWatchdogT *Watchdog)
{
        int (*set_func)(void *, SaHpiResourceIdT, SaHpiWatchdogNumT, SaHpiWatchdogT *);

        RPTable *rpt = default_rpt;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        set_func = h->abi->set_watchdog_info;
        if (!set_func)          
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (set_func(h->hnd, ResourceId, WatchdogNum, Watchdog))
                return SA_ERR_HPI_UNKNOWN;

        return SA_OK;
}

SaErrorT SAHPI_API saHpiWatchdogTimerReset (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiWatchdogNumT WatchdogNum)
{
        int (*reset_func)(void *, SaHpiResourceIdT, SaHpiWatchdogNumT);

        RPTable *rpt = default_rpt;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        reset_func = h->abi->reset_watchdog;
        if (!reset_func)                
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (reset_func(h->hnd, ResourceId, WatchdogNum))
                return SA_ERR_HPI_UNKNOWN;

        return SA_OK;
}

SaErrorT SAHPI_API saHpiHotSwapControlRequest (
        SAHPI_IN SaHpiSessionIdT SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId)
{
        RPTable *rpt = default_rpt;
        SaHpiRptEntryT *res;
        
        res = oh_get_resource_by_id(rpt, ResourceId);
        if(res == NULL) {
                dbg("No such resouce");
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP))
                return SA_ERR_HPI_INVALID;
        
        /* TODO: make a better symbolic constant */
        oh_set_resource_managed(ResourceId, 1);

        return SA_OK;
}

SaErrorT SAHPI_API saHpiResourceActiveSet (
        SAHPI_IN SaHpiSessionIdT SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId)
{
        int (*set_hotswap_state)(void *hnd, SaHpiResourceIdT,
                        SaHpiHsStateT state);
        
        RPTable *rpt = default_rpt;
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        res = oh_get_resource_by_id(rpt, ResourceId);
        if(res == NULL) {
                dbg("No such resouce");
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP))
                return SA_ERR_HPI_INVALID;
        if (!oh_is_resource_managed(ResourceId)) 
                return SA_ERR_HPI_INVALID_CMD;
        
        set_hotswap_state = h->abi->set_hotswap_state;
        if (!set_hotswap_state) 
                return SA_ERR_HPI_UNSUPPORTED_API;
        
        /* this was done in the old code, so we do it here */
        oh_set_resource_managed(ResourceId, 0);

        if (set_hotswap_state(h->hnd, ResourceId, SAHPI_HS_STATE_ACTIVE_HEALTHY)<0) 
                return SA_ERR_HPI_UNKNOWN;
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiResourceInactiveSet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId)
{
        int (*set_hotswap_state)(void *hnd, SaHpiResourceIdT rid,
                                 SaHpiHsStateT state);
        
        RPTable *rpt = default_rpt;
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        res = oh_get_resource_by_id(rpt, ResourceId);
        if(res == NULL) {
                dbg("No such resouce");
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP))
                return SA_ERR_HPI_INVALID;
        if (!oh_is_resource_managed(ResourceId))
                return SA_ERR_HPI_INVALID_CMD;
        
        set_hotswap_state = h->abi->set_hotswap_state;
        if (!set_hotswap_state) 
                return SA_ERR_HPI_UNSUPPORTED_API;

        oh_set_resource_managed(ResourceId, 0);
        if (set_hotswap_state(h->hnd, ResourceId, SAHPI_HS_STATE_INACTIVE)<0) 
                return SA_ERR_HPI_UNKNOWN;
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiAutoInsertTimeoutGet(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_OUT SaHpiTimeoutT *Timeout)
{
        struct oh_session *s;
        
        OH_STATE_READY_CHECK;

        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }
        
        *Timeout = get_hotswap_auto_insert_timeout();
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiAutoInsertTimeoutSet(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiTimeoutT Timeout)
{
        struct oh_session *s;
        
        OH_STATE_READY_CHECK;

        s = session_get(SessionId);
        if (!s) {
                dbg("Invalid session");
                return SA_ERR_HPI_INVALID_SESSION;
        }
        
        set_hotswap_auto_insert_timeout(Timeout);
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiAutoExtractTimeoutGet(
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_OUT SaHpiTimeoutT *Timeout)
{
        RPTable *rpt = default_rpt;
        SaHpiRptEntryT *res;
        
        res = oh_get_resource_by_id(rpt, ResourceId);
        if(res == NULL) {
                dbg("No such resouce");
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP))
                return SA_ERR_HPI_INVALID;
        
        /*
          I think we need to push this down to the plugin
          *Timeout = res->auto_extract_timeout;
          */
        return SA_OK;
}

SaErrorT SAHPI_API saHpiAutoExtractTimeoutSet(
        SAHPI_IN SaHpiSessionIdT SessionId,
        SAHPI_IN SaHpiResourceIdT ResourceId,
        SAHPI_IN SaHpiTimeoutT Timeout)
{
        RPTable *rpt = default_rpt;
        SaHpiRptEntryT *res;
        
        res = oh_get_resource_by_id(rpt, ResourceId);
        if(res == NULL) {
                dbg("No such resouce");
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP))
                return SA_ERR_HPI_INVALID;
        
        /* 
           I think we need to push this down to the plugin
           res->auto_extract_timeout = Timeout;
        */
        return SA_OK;
}

SaErrorT SAHPI_API saHpiHotSwapStateGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_OUT SaHpiHsStateT *State)
{
        int (*get_hotswap_state)(void *hnd, SaHpiResourceIdT rid,
                                 SaHpiHsStateT *state);
        RPTable *rpt = default_rpt;
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        res = oh_get_resource_by_id(rpt, ResourceId);
        if(res == NULL) {
                dbg("No such resouce");
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP))
                return SA_ERR_HPI_INVALID;
        
        get_hotswap_state = h->abi->get_hotswap_state;
        if (!get_hotswap_state) 
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (get_hotswap_state(h->hnd, ResourceId, State)<0) 
                return SA_ERR_HPI_UNKNOWN;
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiHotSwapActionRequest (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiHsActionT Action)
{
        int (*request_hotswap_action)(void *hnd, SaHpiResourceIdT rid,
                        SaHpiHsActionT act);
        
        RPTable *rpt = default_rpt;
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        res = oh_get_resource_by_id(rpt, ResourceId);
        if(res == NULL) {
                dbg("No such resouce");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP))
                return SA_ERR_HPI_INVALID;
        
        request_hotswap_action = h->abi->request_hotswap_action;
        if (!request_hotswap_action) 
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (request_hotswap_action(h->hnd, ResourceId, Action)<0) 
                return SA_ERR_HPI_UNKNOWN;
        
        get_events();
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiResourcePowerStateGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_OUT SaHpiHsPowerStateT *State)
{
        int (*get_power_state)(void *hnd, SaHpiResourceIdT id,
                               SaHpiHsPowerStateT *state);
        RPTable *rpt = default_rpt;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        get_power_state = h->abi->get_power_state;
        if (!get_power_state) 
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (get_power_state(h->hnd, ResourceId, State)<0) 
                return SA_ERR_HPI_UNKNOWN;
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiResourcePowerStateSet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiHsPowerStateT State)
{
        int (*set_power_state)(void *hnd, SaHpiResourceIdT id,
                               SaHpiHsPowerStateT state);
        RPTable *rpt = default_rpt;
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        res = oh_get_resource_by_id(rpt, ResourceId);
        if(res == NULL) {
                dbg("No such resouce");
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        set_power_state = h->abi->set_power_state;
        if (!set_power_state) 
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (set_power_state(h->hnd, ResourceId, State)<0) 
                return SA_ERR_HPI_UNKNOWN;
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiHotSwapIndicatorStateGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_OUT SaHpiHsIndicatorStateT *State)
{
        int (*get_indicator_state)(void *hnd, SaHpiResourceIdT id,
                                   SaHpiHsIndicatorStateT *state);
        RPTable *rpt = default_rpt;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
                
        get_indicator_state = h->abi->get_indicator_state;
        if (!get_indicator_state) 
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (get_indicator_state(h->hnd, ResourceId, State)<0) 
                return SA_ERR_HPI_UNKNOWN;
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiHotSwapIndicatorStateSet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiHsIndicatorStateT State)
{
        int (*set_indicator_state)(void *hnd, SaHpiResourceIdT id,
                                   SaHpiHsIndicatorStateT state);
        RPTable *rpt = default_rpt;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        set_indicator_state = h->abi->set_indicator_state;
        if (!set_indicator_state) 
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (set_indicator_state(h->hnd, ResourceId, State)<0) 
                return SA_ERR_HPI_UNKNOWN;
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiParmControl (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiParmActionT Action)
{
        int (*control_parm)(void *, SaHpiResourceIdT, SaHpiParmActionT);
        
        RPTable *rpt = default_rpt;
        SaHpiRptEntryT *res;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        res = oh_get_resource_by_id(rpt, ResourceId);
        if(res == NULL) {
                dbg("No such resouce");
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        if (!(res->ResourceCapabilities & SAHPI_CAPABILITY_CONFIGURATION))
                return SA_ERR_HPI_INVALID;
        
        control_parm = h->abi->control_parm;
        if (!control_parm) 
                return SA_ERR_HPI_UNSUPPORTED_API;

        if (control_parm(h->hnd, ResourceId, Action)<0) 
                return SA_ERR_HPI_UNKNOWN;
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiResourceResetStateGet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_OUT SaHpiResetActionT *ResetAction)
{
        int (*get_func)(void *, SaHpiResourceIdT, SaHpiResetActionT *);

        RPTable *rpt = default_rpt;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        get_func = h->abi->get_reset_state;
        if (!get_func) 
                return SA_ERR_HPI_INVALID_CMD;

        if (get_func(h->hnd, ResourceId, ResetAction)<0) 
                return SA_ERR_HPI_UNKNOWN;
        
        return SA_OK;
}

SaErrorT SAHPI_API saHpiResourceResetStateSet (
                SAHPI_IN SaHpiSessionIdT SessionId,
                SAHPI_IN SaHpiResourceIdT ResourceId,
                SAHPI_IN SaHpiResetActionT ResetAction)
{
        int (*set_func)(void *, SaHpiResourceIdT, SaHpiResetActionT);
        RPTable *rpt = default_rpt;
        struct oh_handler *h;
        
        h = oh_get_resource_data(rpt, ResourceId);
        
        if(!h) {
                dbg("Can't find handler for ResourceId %d",ResourceId);
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        
        set_func = h->abi->set_reset_state;
        if (!set_func) 
                return SA_ERR_HPI_INVALID_CMD;

        if (set_func(h->hnd, ResourceId, ResetAction)<0) 
                return SA_ERR_HPI_UNKNOWN;
        
        return SA_OK;
}
