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
 *     Sean Dague <http://dague.net/sean>
 *     David Judkovics <djudkovi@us.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SaHpi.h>
#include <openhpi.h>
#include <sel_utils.h>


/* declare Rptable object */
RPTable *default_rpt = NULL; 


/*
 *  Global list of all available domain id's (SaHpiDomainIdT).
 *  The intent is that this list is maintained as new RPT entries
 *  are added and removed from the global RPT table, and used by
 *  saHpiSessionOpen() to determine if the requested domain exist
 *  without doing a full search of the RPT. 
 */
static GSList *global_domain_list = NULL;

int is_in_domain_list(SaHpiDomainIdT did) 
{
        GSList *i;
        
        data_access_lock();

        g_slist_for_each(i, global_domain_list) {
                struct oh_domain *d = i->data;
                if(d->domain_id == did) {
                        data_access_unlock();
                        return 1;
                }
        }

        data_access_unlock();
        
        return 0;
}

struct oh_domain *get_domain_by_id(SaHpiDomainIdT did)
{
        GSList *i;
        
        data_access_lock();

        g_slist_for_each(i, global_domain_list) {
                struct oh_domain *d = i->data;
                if(d->domain_id == did) {
                        data_access_unlock();
                        return d;
                }
        }

        data_access_unlock();

        return NULL;
}

int add_domain(SaHpiDomainIdT did)
{
        struct oh_domain *d;
        
        if(is_in_domain_list(did) > 0) {
                dbg("Domain %d exists already, something is fishy", did);
                return -1;
        }
        
        data_access_lock();
        
        d = malloc(sizeof(*d));
        if (!d) {
                dbg("Out of memory");
                data_access_unlock();
                return -1;
        }
        
        d->domain_id = did;
        d->sel = oh_sel_create(OH_SEL_MAX_SIZE);
 
        global_domain_list = g_slist_append(global_domain_list, d);

        data_access_unlock();
        
        return 0;
}


void oh_cleanup_domain(void)
{
        data_access_lock();

        while(global_domain_list) {
                struct oh_domain *d = (struct oh_domain *)global_domain_list->data;
                global_domain_list = g_slist_remove(global_domain_list, d);

                if (d->sel)
                        oh_sel_close(d->sel);

                free(d);
        }

        data_access_unlock();
}

