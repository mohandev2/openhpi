/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2003
 * Copyright (c) 2003 by Intel Corp.
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
#include <time.h>

#include <SaHpi.h>
#include <oh_error.h>
#include <sel_utils.h>


/* allocate and initialize an SEL */
oh_sel *oh_sel_create(SaHpiUint32T size)
{
        oh_sel *sel;

        sel = (oh_sel *) g_malloc0(sizeof(oh_sel));
        if (sel != NULL) {
                sel->enabled = TRUE;
                sel->overflow = FALSE;
                sel->gentimestamp = TRUE;
                sel->lastUpdate = SAHPI_TIME_UNSPECIFIED;
                sel->offset = 0;
                sel->maxsize = size;
                sel->nextId = SAHPI_OLDEST_ENTRY + 1; // always start at 1
                sel->selentries = NULL;
        }
        return sel;
}


/* close and free all memory associated with an SEL */
SaErrorT oh_sel_close(oh_sel *sel)
{

        if (sel == NULL) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        oh_sel_clear(sel);
        free(sel);
        return SA_OK;
}


/* add a new entry to the SEL 
 *
 * This API will be removed in a later version.
 * You should use oh_sel_append instead.
 */
SaErrorT oh_sel_add(oh_sel *sel, SaHpiEventLogEntryT *entry)
{
        return oh_sel_append(sel, entry);
}


/* append a new entry to the SEL */
SaErrorT oh_sel_append(oh_sel *sel, SaHpiEventLogEntryT *entry)
{
        SaHpiEventLogEntryT * myentry;
        time_t tt1;
        GList *temp;

        /* check for valid sel params and state */
        if (sel == NULL || entry == NULL) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        if (sel->enabled == FALSE) {
                return SA_ERR_HPI_INVALID_REQUEST;
        }

        /* alloc the new entry */
        myentry = (SaHpiEventLogEntryT *) g_malloc0(sizeof(SaHpiEventLogEntryT));
        if (myentry == NULL) {
                sel->overflow = TRUE;
                return SA_ERR_HPI_OUT_OF_SPACE;
        }

        /* if necessary, wrap the sel entries */
        if (sel->maxsize != OH_SEL_MAX_SIZE && g_list_length(sel->selentries) == sel->maxsize) {
                gpointer tempdata;
                temp = g_list_first(sel->selentries);
                tempdata = temp->data;
                sel->selentries = g_list_remove(sel->selentries, temp->data);
                g_free(tempdata);
        }

        /* appenc the new entry */
        entry->EntryId = sel->nextId;
        sel->nextId++;
        if (sel->gentimestamp) {
                time(&tt1);
                sel->lastUpdate = (SaHpiTimeT) (tt1 * 1000000000) + sel->offset;
        } else {
                sel->lastUpdate = entry->Timestamp;
        }
        entry->Timestamp = sel->lastUpdate;
        memcpy(myentry, entry, sizeof(SaHpiEventLogEntryT));
        sel->selentries = g_list_append(sel->selentries, myentry);
        return SA_OK;
}


/* prepend a new entry to the SEL */
SaErrorT oh_sel_prepend(oh_sel *sel, SaHpiEventLogEntryT *entry)
{
        SaHpiEventLogEntryT * myentry, * tmpentry;
        time_t tt1;
        GList *sellist;

        /* check for valid sel params and state */
        if (sel == NULL || entry == NULL) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        if (sel->enabled == FALSE) {
                return SA_ERR_HPI_INVALID_REQUEST;
        }

        /* see if sel is full */
        if (sel->maxsize != OH_SEL_MAX_SIZE && g_list_length(sel->selentries) == sel->maxsize) {
                return SA_ERR_HPI_OUT_OF_SPACE;
        }

        /* alloc the new entry */
        myentry = (SaHpiEventLogEntryT *) g_malloc0(sizeof(SaHpiEventLogEntryT));
        if (myentry == NULL) {
                sel->overflow = TRUE;
                return SA_ERR_HPI_OUT_OF_SPACE;
        }

        /* since we are adding entries in reverse order we have to renumber
         * existing entries
         */
        sellist = g_list_first(sel->selentries);
        while (sellist != NULL) {
                tmpentry = (SaHpiEventLogEntryT *) sellist->data;
                tmpentry->EntryId++;
                sellist = g_list_next(sellist);
        }

        /* prepend the new entry */
        entry->EntryId = 1;
        sel->nextId++;
        if (sel->gentimestamp) {
                time(&tt1);
                sel->lastUpdate = (SaHpiTimeT) (tt1 * 1000000000) + sel->offset;
        } else {
                sel->lastUpdate = entry->Timestamp;
        }
        entry->Timestamp = sel->lastUpdate;
        memcpy(myentry, entry, sizeof(SaHpiEventLogEntryT));
        sel->selentries = g_list_prepend(sel->selentries, myentry);
        return SA_OK;
}


/* delete an entry in the SEL (not supported, per errata) */
SaErrorT oh_sel_delete(oh_sel *sel, SaHpiEntryIdT *entryid)
{
        return SA_ERR_HPI_UNSUPPORTED_API;
}


/* clear all SEL entries */
SaErrorT oh_sel_clear(oh_sel *sel)
{
        GList *temp;

        if (sel == NULL) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        if (sel->enabled) {
                /* free the list data elements */
                temp = g_list_first(sel->selentries);
                while (temp != NULL) {
                        g_free(temp->data);
                        temp = g_list_next(temp);
                }
                /* free the list nodes */
                g_list_free(sel->selentries);
                /* reset the control structure */
                sel->overflow = FALSE;
                sel->lastUpdate = SAHPI_TIME_UNSPECIFIED;
                sel->nextId = SAHPI_OLDEST_ENTRY + 1; // always start at 1
                sel->selentries = NULL;
                return SA_OK;
        }
        return SA_ERR_HPI_INVALID_REQUEST;
}


/* get an SEL entry */
SaErrorT oh_sel_get(oh_sel *sel, SaHpiEventLogEntryIdT entryid, SaHpiEventLogEntryIdT *prev,
                    SaHpiEventLogEntryIdT *next, SaHpiEventLogEntryT **entry)
{
        SaHpiEventLogEntryT *myentry;
        GList *sellist;
        SaHpiEventLogEntryIdT srchentryid, firstid, lastid;

        if (sel == NULL || prev == NULL || next == NULL || entry == NULL) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        if (g_list_length(sel->selentries) == 0) {
                return SA_ERR_HPI_NOT_PRESENT;
        }

        /* get the first and last entry ids for possible translation */
        sellist = g_list_last(sel->selentries);
        myentry = (SaHpiEventLogEntryT *)sellist->data;
        lastid = myentry->EntryId;
        sellist = g_list_first(sel->selentries);
        myentry = (SaHpiEventLogEntryT *)sellist->data;
        firstid = myentry->EntryId;
        if (entryid == SAHPI_NEWEST_ENTRY) {
                srchentryid = lastid;
        }
        else if (entryid == SAHPI_OLDEST_ENTRY) {
                srchentryid = firstid;
        }
        else {
                srchentryid = entryid;
        }

        sellist = g_list_first(sel->selentries);
        while (sellist != NULL) {
                myentry = (SaHpiEventLogEntryT *) sellist->data;
                if (srchentryid == myentry->EntryId) {
                        *entry = myentry;
                        /* is this the first entry? */
                        if (myentry->EntryId == firstid) {
                                *prev = SAHPI_NO_MORE_ENTRIES;
                        }
                        else {
                                *prev = myentry->EntryId - 1;
                        }
                        /* is this the last entry? */
                        if (myentry->EntryId == lastid) {
                                *next = SAHPI_NO_MORE_ENTRIES;
                        }
                        else {
                                *next = myentry->EntryId + 1;
                        }
                        return SA_OK;
                }
                else if (entryid < myentry->EntryId) {
                        return SA_ERR_HPI_NOT_PRESENT;
                }
                sellist = g_list_next(sellist);
        }
        return SA_ERR_HPI_NOT_PRESENT;
}


/* get SEL info */
SaErrorT oh_sel_info(oh_sel *sel, SaHpiEventLogInfoT *info)
{
        time_t tt1;

        if (sel == NULL || info == NULL) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        info->Entries = g_list_length(sel->selentries);
        info->Size = sel->maxsize;
        info->UpdateTimestamp = sel->lastUpdate;
        time(&tt1);
        info->CurrentTime = (SaHpiTimeT) (tt1 * 1000000000) + sel->offset;
        info->Enabled = sel->enabled;
        info->OverflowFlag = sel->overflow;
        info->OverflowAction = SAHPI_EL_OVERFLOW_OVERWRITE;
        return SA_OK;
}


/* write a SEL entry list to a file */
SaErrorT oh_sel_map_to_file(oh_sel *sel, char *filename)
{
        int file;
        GList *sellist;

        if (sel == NULL || filename == NULL) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0660 );
        if (file < 0) {
                dbg("SEL file '%s' could not be opened", filename);
                return SA_ERR_HPI_ERROR;
        }

        sellist = g_list_first(sel->selentries);
        while (sellist != NULL) {
                write(file, (void *)sellist->data, sizeof(SaHpiEventLogEntryT));
                sellist = g_list_next(sellist);
        }

        if(close(file) != 0) {
                dbg("Couldn't close file '%s'.", filename);
                return SA_ERR_HPI_ERROR;
        }

        return SA_OK;
}


/* read a SEL entry list from a file */
SaErrorT oh_sel_map_from_file(oh_sel *sel, char *filename)
{
        int file;
        SaHpiEventLogEntryT entry;
        SaErrorT retc;

        /* check sel params and state */
        if (sel == NULL || filename == NULL) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }
        if (sel->enabled == FALSE) {
                return SA_ERR_HPI_INVALID_REQUEST;
        }

        file = open(filename, O_RDONLY);
        if (file < 0) {
                dbg("SEL file '%s' could not be opened", filename);
                return SA_ERR_HPI_ERROR;
        }

        oh_sel_clear(sel); // ensure list is empty
        while (read(file, &entry, sizeof(SaHpiEventLogEntryT)) == sizeof(SaHpiEventLogEntryT)) {
                retc = oh_sel_add(sel, &entry);
                if (retc) {
                        close(file);
                        return retc;
                }
        }
        sel->nextId = entry.EntryId + 1;

        if(close(file) != 0) {
                dbg("Couldn't close file '%s'.", filename);
                return SA_ERR_HPI_ERROR;
        }

        return SA_OK;
}


/* set the SEL timestamp offset */
SaErrorT oh_sel_timeset(oh_sel *sel, SaHpiTimeT timestamp)
{
        if (sel == NULL || timestamp > SAHPI_TIME_MAX_RELATIVE ||
            timestamp == SAHPI_TIME_UNSPECIFIED) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        sel->offset = timestamp;
        return SA_OK;
}


/* set the timestamp generate flag */
SaErrorT oh_sel_setgentimestampflag(oh_sel *sel, SaHpiBoolT flag)
{
        if (sel == NULL) {
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        sel->gentimestamp = flag;
        return SA_OK;
}

