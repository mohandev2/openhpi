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
 * Contributors:
 *     David Judkovics <djudkovi@us.ibm.com> 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SaHpi.h>
#include <openhpi.h>

/* multi-threading support, use Posix mutex for data access */
/* initialize mutex used for data locking */
extern pthread_mutex_t data_access_mutex; 

void process_hotswap_policy(void)
{
        SaHpiTimeT cur, est;
        struct oh_hpi_event e;
        struct oh_resource *res;
        int (*set_hotswap_state)(void *hnd, struct oh_resource_id id,
                                                SaHpiHsStateT state);   
        
        while(hotswap_pop_event(&e)>0) {
        
                if (e.event.EventType != SAHPI_ET_HOTSWAP) {
                        dbg("Non-hotswap event!");
                        return;
                }
        
                res = get_res_by_oid(e.parent);
                if (!(res->entry.ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP)) {
                        dbg("Non-hotswapable resource?!");
                        return;
                }
                if (res->controlled) {
                        dbg("Controlled resource?!");
                        return;
                }

                set_hotswap_state = res->handler->abi->set_hotswap_state;
                if (!set_hotswap_state) {
                        dbg("Unsupport hotswap maintainance?!");
                        return;
                }

                gettimeofday1(&cur);
        
                if (e.event.EventDataUnion.HotSwapEvent.HotSwapState 
                                == SAHPI_HS_STATE_INSERTION_PENDING) {
                        est = e.event.Timestamp + get_hotswap_auto_insert_timeout();
                        if (cur>=est) {
                                set_hotswap_state(res->handler->hnd, res->oid, SAHPI_HS_STATE_ACTIVE_HEALTHY);
                        }
                } else if (e.event.EventDataUnion.HotSwapEvent.HotSwapState
                                == SAHPI_HS_STATE_EXTRACTION_PENDING) {
                        est = e.event.Timestamp + res->auto_extract_timeout;
                        if (cur>=est) {
                                set_hotswap_state(res->handler->hnd, res->oid, SAHPI_HS_STATE_INACTIVE);
                        }
                } else {
                        dbg();
                }
        }
}

static GSList *hs_eq=NULL;
/*
 * session_push_event pushs and event into a session.
 * We store a copy of event so that caller of the function
 * needn't care about ref counter of the event.
*/

int hotswap_push_event(struct oh_hpi_event *e)
{
        struct oh_event *e1;

        pthread_mutex_lock(&data_access_mutex);

        e1 = malloc(sizeof(*e1));
        if (!e1) {
                dbg("Out of memory!");
                pthread_mutex_unlock(&data_access_mutex);
                return -1;
        }
        memcpy(e1, e, sizeof(*e));

        hs_eq = g_slist_append(hs_eq, (gpointer *) e1);
        
        pthread_mutex_unlock(&data_access_mutex);

        return 0;
}

/*
 * session_pop_event - pops events off the session.  
 *
 * return codes are left as was, but it seems that return 1 for success
 * here doesn't jive with the rest of the exit codes
 */

int hotswap_pop_event(struct oh_hpi_event *e) 
{
        GSList *head;
        
        pthread_mutex_lock(&data_access_mutex);

        if (g_slist_length(hs_eq) == 0) {
                pthread_mutex_unlock(&data_access_mutex);
                return 0;
        }
       
        head = hs_eq;
        hs_eq = g_slist_remove_link(hs_eq, head);
        
        memcpy(e, head->data, sizeof(*e));
        
        free(head->data);
        g_slist_free_1(head);
        
        pthread_mutex_unlock(&data_access_mutex);

        return 1;
}

/*
 * session_has_event - query if the session has events
 */
int hotswap_has_event()
{
        return (hs_eq == NULL) ? 0 : 1;
}

static SaHpiTimeoutT hotswap_auto_insert_timeout;

SaHpiTimeoutT get_hotswap_auto_insert_timeout(void)
{
        return hotswap_auto_insert_timeout;
}

void set_hotswap_auto_insert_timeout(SaHpiTimeoutT to)
{
        hotswap_auto_insert_timeout = to;
}

