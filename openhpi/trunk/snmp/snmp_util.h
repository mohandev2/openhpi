/*      -*- linux-c -*-
 *
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
 *      Renier Morales <renierm@users.sf.net>
 *
 */

#ifndef SNMP_BC_UTIL_H
#define SNMP_BC_UTIL_H

#include <config.h>
#include <SaHpi.h>

/****** net-snmp config template ******/
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif

#ifdef WIN32
#include <winsock.h>
#endif

#define NETSNMP_IMPORT extern
#define NETSNMP_INLINE
#define RETSIGTYPE void
#define NET_SNMP_CONFIG_H
/**************************************/
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/library/transform_oids.h>

#define MAX_ASN_STR_LEN 300

#define SA_SNMP_ERR_BASE -10000
#define SA_SNMP_NOSUCHOBJECT	(SaErrorT)(SA_SNMP_ERR_BASE - SNMP_NOSUCHOBJECT)
#define SA_SNMP_NOSUCHINSTANCE	(SaErrorT)(SA_SNMP_ERR_BASE - SNMP_NOSUCHINSTANCE)
#define SA_SNMP_ENDOFMIBVIEW	(SaErrorT)(SA_SNMP_ERR_BASE - SNMP_ENDOFMIBVIEW)
#define SA_SNMP_TIMEOUT	 	(SaErrorT)(SA_SNMP_ERR_BASE - 0) 


#define CHECK_END(a) ((a != SNMP_ENDOFMIBVIEW) &&  \
                  (a != SNMP_NOSUCHOBJECT) &&  \
                  (a != SNMP_NOSUCHINSTANCE))? 1:0 

/* Place-holder for values set and returned by snmp
 */
struct snmp_value {
        u_char type;
        char string[MAX_ASN_STR_LEN];
        unsigned int str_len;
        long integer;
};

SaErrorT snmp_get(
        struct snmp_session *ss,
        const char *objid,
        struct snmp_value *value);

SaErrorT snmp_set(
        struct snmp_session *ss,
        char *objid,
        struct snmp_value value);

SaErrorT snmp_get2(struct snmp_session *ss, 
		   oid *objid, 
		   size_t objid_len,
		   struct snmp_value *value);

SaErrorT snmp_set2(struct snmp_session *ss, 
	           oid *objid,
	           size_t objid_len,
                   struct snmp_value *value);

SaErrorT snmp_getn_bulk( struct snmp_session *ss, 
		    oid *bulk_objid, 
		    size_t bulk_objid_len,
		    struct snmp_pdu *bulk_pdu, 
		    struct snmp_pdu **bulk_response,
		    int num_repetitions );

void sc_free_pdu(struct snmp_pdu **p);

#endif
