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
 *      Renier Morales <renierm@users.sf.net>
 *
 */

#ifndef __OH_DOMAIN_H
#define __OH_DOMAIN_H

#include <SaHpi.h>
#include <glib.h>
#include <oh_utils.h>

/* Number of pre-alloced session slots for a domain. */
#define OH_SESSION_PREALLOC 5

/*
 *  Global table of all active domains (oh_domain).
 *  Encapsulated in a struct to store a lock alongside of it.
 */
struct oh_domain_table {
        GHashTable *table;
        GStaticRecMutex lock;
};

struct oh_dat { /* Domain Alarm Table */
        SaHpiAlarmIdT next_id;
        GSList *list;
        SaHpiUint32T update_count;
        SaHpiTimeT update_timestamp;
        SaHpiBoolT overflow;
};

struct oh_drt { /* Domain Reference Table */
        SaHpiEntryIdT next_id;
        GSList *list;
        SaHpiUint32T update_count;
        SaHpiTimeT update_timestamp;
        SaHpiBoolT overflow;
};

extern struct oh_domain_table oh_domains;

/*
 * Representation of an domain
 */
struct oh_domain {
        /* This id is used by app
         * to identify the domain
         */
        SaHpiDomainIdT id;

	/* id of parent domain. 0 for unspecified domain */
        SaHpiDomainIdT p_id;

        /* Domain's Resource Presence Table */
        RPTable rpt;

        /* Domain Alarm Table */
        struct oh_dat dat;

       /* Domain Reference Table */
        struct oh_drt drt;

        /* Domain Information */
        SaHpiDomainCapabilitiesT capabilities;
        SaHpiBoolT        is_peer;
        SaHpiTextBufferT  tag;
        SaHpiGuidT        guid;

        /* Domain Event Log */
        oh_el *del;

        /* List of session ids */
        GArray *sessions;

        /* Synchronization - used internally by domain interfaces below. */
        GStaticRecMutex lock;
        int refcount;
        GStaticRecMutex refcount_lock;
};

SaHpiDomainIdT oh_create_domain(SaHpiDomainCapabilitiesT capabilities,
                                SaHpiTextBufferT *tag);
SaHpiDomainIdT oh_get_default_domain_id(void);
SaHpiDomainIdT oh_request_domain_id(unsigned int handler_id,
                                    SaHpiTextBufferT *tag,
				    SaHpiDomainCapabilitiesT capabilities,
				    SaHpiDomainIdT parent_id,
				    SaHpiDomainIdT peer_id);
SaErrorT oh_release_domain_id(unsigned int handler_id,
                              SaHpiDomainIdT did);
SaErrorT oh_destroy_domain(SaHpiDomainIdT did);
struct oh_domain *oh_get_domain(SaHpiDomainIdT did);
GArray *oh_list_domains(void);
SaErrorT oh_release_domain(struct oh_domain *domain);
SaErrorT oh_drt_entry_get(SaHpiDomainIdT    did,
			  SaHpiEntryIdT     entryid,
			  SaHpiEntryIdT     *nextentryid,
			  SaHpiDrtEntryT    *drt);
#endif /* __OH_DOMAIN_H */
