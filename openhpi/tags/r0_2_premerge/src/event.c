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
 *     David Judkovics <djudkovi@us.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SaHpi.h>
#include <openhpi.h>
#include <epath_utils.h>

static void process_session_event(struct oh_hpi_event *e)
{
        struct oh_resource *res;
        GSList *i;

        data_access_lock();

        res = get_res_by_oid(e->parent);
        if (!res) {
                dbg("No the resource");
                data_access_unlock();
                return;
        }

        if (res->controlled == 0 
                && e->event.EventType == SAHPI_ET_HOTSWAP) {
                hotswap_push_event(e);
        }

        g_slist_for_each(i, res->domain_list) {
                SaHpiDomainIdT domain_id;
                struct oh_domain *d;
                GSList *j;

                domain_id = GPOINTER_TO_UINT(i->data);
                d = get_domain_by_id(domain_id);
                if (d) 
                        dsel_add2(d, e);
                else 
                        dbg("Invalid domain");

                g_slist_for_each(j, global_session_list) {
                        struct oh_session *s = j->data;
                        if (domain_id == s->domain_id
                            && s->event_state==OH_EVENT_SUBSCRIBE) {
                                session_push_event(s, e);
                        }
                }
        }

        data_access_unlock();
}

static void process_resource_event(struct oh_handler *h, struct oh_resource_event *e) 
{
        struct oh_resource *res;
        const char *entity_root;
        SaHpiEntityPathT root_ep;
        
        data_access_lock();

        res = insert_resource(h, e->id);
        memcpy(&res->entry, &e->entry, sizeof(res->entry));
        entity_root = g_hash_table_lookup(h->config,"entity_root");
        if (entity_root != NULL) {
                dbg("Append entity root %s", entity_root);
                string2entitypath(entity_root, &root_ep);
                ep_concat(&res->entry.ResourceEntity, &root_ep);
        }
        
        res->entry.ResourceId = global_rpt_counter;
        if (res->entry.ResourceCapabilities&SAHPI_CAPABILITY_DOMAIN) {
                dbg("New domain in resource!");
                if (!get_domain_by_oid(e->domain_id)) {
                        res->entry.DomainId = new_domain(e->domain_id);
                } else {
                        dbg("Reported domain?! Buggy plugin!");
                }
        }
                
        /*Assume all resources blongs to DEFAULT_DOMAIN*/
        res->domain_list = g_slist_append(res->domain_list, 
                        GUINT_TO_POINTER(SAHPI_DEFAULT_DOMAIN_ID));

        data_access_unlock();

}

static void process_domain_event(struct oh_handler *h, struct oh_domain_event *e)
{
        struct oh_resource *res;
        struct oh_domain *domain;

        data_access_lock();

        res = get_res_by_oid(e->res_id);
        domain = get_domain_by_oid(e->domain_id);
        
        if (!res) {
                dbg("Cannot find corresponding resource");
                data_access_unlock();
                return;
        }               
        if (!domain) {
                dbg("Cannot find corresponding domain");
                data_access_unlock();
                return;
        }               
        res->domain_list = g_slist_append(res->domain_list, GUINT_TO_POINTER(domain->domain_id));
        
        data_access_unlock();

}

static void process_rdr_event(struct oh_handler *h, struct oh_rdr_event *e)
{
        struct oh_resource *res;
        struct oh_rdr *rdr;
        const char *entity_root;
        SaHpiEntityPathT root_ep;

        res = get_res_by_oid(e->parent);
        if (!res) {
                dbg("Cannot find corresponding resource");
                return;
        }
        rdr = insert_rdr(res, e->id);
        memcpy(&rdr->rdr, &e->rdr, sizeof(rdr->rdr));
        entity_root = g_hash_table_lookup(h->config,"entity_root");
        if (entity_root != NULL) {
                dbg("Append entity root %s", entity_root);
                string2entitypath(entity_root, &root_ep);
                ep_concat(&rdr->rdr.Entity, &root_ep);
        }
        
        switch (rdr->rdr.RdrType) {
        case SAHPI_SENSOR_RDR: 
                rdr->rdr.RdrTypeUnion.SensorRec.Num = res->sensor_counter++;
                break;
        case SAHPI_CTRL_RDR:
                rdr->rdr.RdrTypeUnion.CtrlRec.Num = res->ctrl_counter++;
                break;
        case SAHPI_INVENTORY_RDR:
                rdr->rdr.RdrTypeUnion.InventoryRec.EirId = res->inventory_counter++;
                break;
        case SAHPI_WATCHDOG_RDR:
                rdr->rdr.RdrTypeUnion.WatchdogRec.WatchdogNum =  res->watchdog_counter++;
                break;
        default:
                dbg("FIXME: Cannot process such RDR type ");
                break;
        }
}

static void process_rsel_event(struct oh_handler *h, struct oh_rsel_event *e)
{
        struct oh_resource *res;
        
        dbg("RSEL event! rsel->oid=%p", e->rsel.oid.ptr);
        res = get_res_by_oid(e->rsel.parent);
        if (!res) {
                dbg("Cannot find corresponding resource");
                return;
        }
        
        rsel_add2(res, e);
}

static int get_event(void)
{
        int has_event;
        struct oh_event event;
        GSList *i;

        has_event = 0;

        data_access_lock();

        g_slist_for_each(i, global_handler_list) {
                struct timeval to = {0, 0};
                struct oh_handler *h = i->data; 
                int rv;
                
                rv = h->abi->get_event(h->hnd, &event, &to);
                if (rv>0) {
                        switch (event.type) {
                        case OH_ET_HPI:
                                /* add the event to session event list */
                                process_session_event(&event.u.hpi_event);
                                break;
                        case OH_ET_RESOURCE:
                                process_resource_event(h, &event.u.res_event);
                                break;
                        case OH_ET_DOMAIN:
                                process_domain_event(h, &event.u.domain_event);
                                break;
                        case OH_ET_RDR:
                                process_rdr_event(h, &event.u.rdr_event);
                                break;
                        case OH_ET_RSEL:
                                process_rsel_event(h, &event.u.rsel_event);
                                break;
                        default:
                                dbg("Error! Should not reach here!");
                        }

                        has_event = 1;
                } else if (rv <0) {
                        has_event = -1;
                        break;
                }
        }

        data_access_unlock();

        return has_event;
}

int get_events(void)
{
        int has_event;

        data_access_lock();

        do {
                has_event = get_event();
        } while (has_event>0);

        process_hotswap_policy();

        data_access_unlock();

        return 0;
}
