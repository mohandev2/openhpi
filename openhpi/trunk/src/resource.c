/* BSD License
 * Copyright (C) by Intel Crop.
 * Author: Louis Zhuang <louis.zhuang@linux.intel.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SaHpi.h>
#include <openhpi.h>

GSList *global_rpt = NULL;
unsigned int global_rpt_counter = 0; /*XXX: I use the couter for two purposes. 
				   1) RptInfo counter 2) ResourceId allocation */
struct timeval global_rpt_timestamp = {0, 0};

static struct oh_resource *add_res(struct oh_handler *h, struct oh_resource_id oid)
{
	struct oh_resource *res;
	
	res = malloc(sizeof(*res));
	if (!res) {
		dbg("Cannot get memory!");
		return NULL;
	}
	memset(res, 0, sizeof(*res));
	
	global_rpt_counter++;
	gettimeofday(&global_rpt_timestamp, NULL);
	
	memcpy(&res->oid, &oid, sizeof(oid));
	// set resource id in process_internal_event
	// res->entry.ResourceId = global_rpt_counter++;
	res->handler = h;
	h->resource_list = g_slist_append(h->resource_list, res);

	global_rpt = g_slist_append(global_rpt, res);
	return res;
}

struct oh_resource *get_res_by_oid(struct oh_resource_id oid)
{
	GSList *i;
	
	g_slist_for_each(i, global_rpt) {
		struct oh_resource *res = i->data;
		if (memcmp(&res->oid, &oid, sizeof(oid))==0)
				return res;
	}
	return NULL;
}

struct oh_resource *insert_resource(struct oh_handler *h, struct oh_resource_id oid)
{
	struct oh_resource *res;

	res = get_res_by_oid(oid);
	if (!res) {
		dbg("New entity, add it");
		res = add_res(h, oid);
	}
	if (!res) {
		dbg("Cannot add new entity");
	}
	return res;
}

struct oh_resource *get_resource(SaHpiResourceIdT rid)
{
	GSList *i;
	
	g_slist_for_each(i, global_rpt) {
		struct oh_resource *res = i->data;
		if (res->entry.ResourceId == rid)
			return res;
	}
	return NULL;
}

int resource_is_in_domain(struct oh_resource *res, SaHpiDomainIdT did)
{
	GSList *i;
	
	g_slist_for_each(i, res->domain_list) {
		SaHpiDomainIdT id = GPOINTER_TO_UINT(i->data);
		if (id == did)
			return 1;
	}
	return 0;
}

static struct oh_rdr *add_rdr(struct oh_resource *res, struct oh_rdr_id oid)
{
	struct oh_rdr *rdr;
	
	rdr = malloc(sizeof(*rdr));
	if (!rdr) {
		dbg("Cannot get memory!");
		return NULL;
	}
	memset(rdr, 0, sizeof(*rdr));
	
	memcpy(&rdr->oid, &oid, sizeof(oid));
	
	res->rdr_list = g_slist_append(res->rdr_list, rdr);
	return rdr;
}

struct oh_rdr *get_rdr_by_oid(struct oh_resource *res, struct oh_rdr_id oid)
{
	GSList *i;
	
	g_slist_for_each(i, res->rdr_list) {
		struct oh_rdr *rdr = i->data;
		if (memcmp(&rdr->oid, &oid, sizeof(oid))==0)
			return rdr;
	}
	return NULL;
}

struct oh_rdr *insert_rdr(struct oh_resource *res, struct oh_rdr_id oid)
{
	struct oh_rdr *rdr;
	
	rdr = get_rdr_by_oid(res, oid);
	if (!rdr) {
		dbg("New rdr, add it!");
		rdr = add_rdr(res, oid);
	}
	if (!rdr) {
		dbg("Cannot add new RDR");
	}
	return rdr;
}
