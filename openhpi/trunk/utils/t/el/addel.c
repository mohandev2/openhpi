/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2004
 * Copyright (c) 2004 by Intel Corp.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Authors:
 *      David Ashley<dashley@us.ibm.com>
 */


#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <SaHpi.h>
#include <openhpi.h>
#include <oh_utils.h>


#include "el_test.h"


/* note: if OH_ELTEST_MAX_ENTRIES changes we may need additional members here */
static char *data[10] = {
        "Test data one",
        "Test data two",
        "Test data three",
        "Test data four",
        "Test data five",
        "Test data six",
        "Test data seven",
        "Test data eight",
        "Test data nine",
        "Test data ten"
};


/**
* add an SaHpiUserEventT event to the EL
*
* Note that the event data we use is just a string from the above array
*/
int add_event(oh_el *el, int idx) {
        SaHpiEventLogEntryT *entry, *fetchentry;
        SaHpiEventT event;
        SaErrorT retc;
        SaHpiEventLogEntryIdT oldId = el->nextId, next, prev;

        if (idx >= sizeof(data) / sizeof(char *)) {
                dbg("ERROR: idx invalid.");
                return 1;
        }

        /* add a single event */
        event.Source = 1;
        event.EventType = SAHPI_ET_USER;
        event.Timestamp = SAHPI_TIME_UNSPECIFIED;
        event.Severity = SAHPI_DEBUG;
        strcpy((char *) &event.EventDataUnion.UserEvent.UserEventData.Data, data[idx]);
        retc = oh_el_append(el, &event);

        if (retc != SA_OK) {
                dbg("ERROR: oh_el_add failed.");
                return 1;
        }
	
	entry = (SaHpiEventLogEntryT *)(g_list_last(el->elentries)->data);
        /* check correct id */
        if (entry->EntryId != el->nextId - 1) {
                dbg("ERROR: entry.EntryId invalid.");
                return 1;
        }

        if (entry->Timestamp == 0) {
                dbg("ERROR: entry.Timestamp invalid.");
                return 1;
        }

        /* inspect oh_el struct values */
        if(el->enabled != TRUE) {
                dbg("ERROR: el->enabled invalid.");
                return 1;
        }

        if(el->overflow != FALSE) {
                dbg("ERROR: el->overflow invalid.");
                return 1;
        }

        if(el->lastUpdate != entry->Timestamp) {
                dbg("ERROR: el->lastUpdate invalid.");
                return 1;
        }

        if(el->offset != 0) {
                dbg("ERROR: el->offset invalid.");
                return 1;
        }

        if(el->nextId != oldId + 1) {
                dbg("ERROR: el->nextId invalid.");
                return 1;
        }

        if(el->elentries == NULL) {
                dbg("ERROR: el->elentries == NULL.");
                return 1;
        }

        /* now fetch the event and compare it */
        retc = oh_el_get(el, entry->EntryId, &prev, &next, &fetchentry);
        if (retc != SA_OK) {
                dbg("ERROR: oh_el_get failed.");
                return 1;
        }

        if (entry->EntryId != fetchentry->EntryId) {
                dbg("ERROR: entry->EntryId invalid.");
                return 1;
        }

        if (prev != entry->EntryId - 1) {
                if (prev != SAHPI_NO_MORE_ENTRIES) {
                        dbg("ERROR: prev invalid.");
                        return 1;
                }
        }

        if (next != entry->EntryId + 1) {
                if (next != SAHPI_NO_MORE_ENTRIES) {
                        dbg("ERROR: next invalid.");
                        return 1;
                }
        }

        if (fetchentry->Timestamp == 0) {
                dbg("ERROR: fetchentry->Timestamp invalid.");
                return 1;
        }

        if (fetchentry->Event.Source != 1) {
                dbg("ERROR: fetchentry->Event.Source invalid.");
                return 1;
        }

        if (fetchentry->Event.EventType != SAHPI_ET_USER) {
                dbg("ERROR: fetchentry->Event.EventType invalid.");
                return 1;
        }

        if (fetchentry->Event.Timestamp != SAHPI_TIME_UNSPECIFIED) {
                dbg("ERROR: fetchentry->Event.Timestamp invalid.");
                return 1;
        }

        if (fetchentry->Event.Severity != SAHPI_DEBUG) {
                dbg("ERROR: fetchentry->Event.Severity invalid.");
                return 1;
        }

        if (strcmp((char *)fetchentry->Event.EventDataUnion.UserEvent.UserEventData.Data,
                    data[idx])) {
                dbg("ERROR: fetchentry->Event.EventDataUnion.UserEvent.UserEventData invalid.");
                return 1;
        }

        return 0;
}

