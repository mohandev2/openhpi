/*
 * Copyright (C) 2007-2008, Hewlett-Packard Development Company, LLP
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the Hewlett-Packard Corporation, nor the names
 * of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author(s)
 *      Raghavendra P.G. <raghavendra.pg@hp.com>
 *
 * This file has the OA related events handling
 *
 *      process_oa_insertion_event()    - Processes the OA insertion event
 *
 *      process_oa_extraction_event()   - Processes the OA extraction event
 *
 *      process_oa_failover_event()     - Processes the OA failover event
 *                                        points the active_con filed of
 *                                        oa_handler to currently active OA
 *                                        Starts the re-discovery to sync up
 *                                        with current status of the system
 *
 *      process_oa_info_event()         - Processes the OA info event
 *                                        If OA info event is just after OA
 *                                        insertion event, then it processed.
 *                                        Else, it is ignored
 *
 *      add_oa_inv_area()               - Adds the OA inventory area for newly
 *                                        inserted OA.  This function is called
 *                                        while processing the OA info event.
 *
 *      build_inserted_oa_rdr()         - Builds the RDRs for the newly
 *                                        inserted OA
 *
 *      build_inserted_oa_inv_rdr()     - Builds the bare minimum inventory
 *                                        RDR for newly inserted OA
 */

#include <oa_soap_plugin.h>

/**
 * process_oa_insertion_event
 *      @oh_handler: Pointer to openhpi handler structure
 *      @con:        Pointer to the SOAP_CON structure
 *      @oa_event:   Pointer to oa event response structure
 *
 * Purpose:
 *      Gets the OA insertion event.
 *      Adds the newly inserted OA information into RPT and RDR table
 *      Creates the hot swap event
 *
 * Detailed Description: NA
 *
 * Return values:
 *      SA_OK                     - on success.
 *      SA_ERR_HPI_INVALID_PARAMS - on wrong parameters.
 *      SA_ERR_HPI_INTERNAL_ERROR - on failure
 **/
SaErrorT process_oa_insertion_event(struct oh_handler_state *oh_handler,
                                    SOAP_CON *con,
                                    struct eventInfo *oa_event)
{
        SaErrorT rv = SA_OK;
        struct getOaInfo request;
        struct oaInfo response;
        struct oa_soap_handler *oa_handler = NULL;
        struct oh_event event;
        SaHpiInt32T bay_number;

        if (oh_handler == NULL || con == NULL || oa_event == NULL) {
                err("Invalid parameters");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        oa_handler = (struct oa_soap_handler *) oh_handler->data;
        if (oa_handler == NULL) {
                err("OA SOAP handler is NULL");
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        /* The OA is sending the wrong bay_number for the inserted OA
         * Hence if the bay number in the event_info is 1,
         * process the event for bay number 2, or vice versa
         * TODO: Remove this workaround when the fix is made to OA firmware
         */
        switch (oa_event->eventData.oaStatus.bayNumber) {
                case 1:
                        bay_number = 2;
                        break;
                case 2:
                        bay_number = 1;
                        break;
                default:
                        err("Wrong OA bay number <%d> detected",
                             oa_event->eventData.oaStatus.bayNumber);
                        return SA_ERR_HPI_INTERNAL_ERROR;
        }

        request.bayNumber = bay_number;
        rv = soap_getOaInfo(con, &request, &response);
        if (rv != SOAP_OK) {
                err("Get OA info failed");
                return SA_ERR_HPI_INTERNAL_ERROR;
        }


        memset(&event, 0, sizeof(struct oh_event));
        update_hotswap_event(oh_handler, &event);
        /* Build the RPT for inserted OA */
        rv = build_oa_rpt(oh_handler, bay_number, &(event.resource));
        if (rv != SA_OK) {
                err("Failed to build OA RPT");
               return rv;
        }

        /* Build the RDR for inserted OA */
        rv = build_inserted_oa_rdr(oh_handler, con, response.bayNumber, &event);
        if (rv != SA_OK) {
                err("Failed to build OA RDR");
                rv = oh_remove_resource(oh_handler->rptcache,
                                        event.resource.ResourceId);
                return rv;
        }

        event.event.Source = event.resource.ResourceId;
        event.event.EventDataUnion.HotSwapEvent.PreviousHotSwapState =
                SAHPI_HS_STATE_NOT_PRESENT;
        event.event.EventDataUnion.HotSwapEvent.HotSwapState =
                SAHPI_HS_STATE_ACTIVE;
        oh_evt_queue_push(oh_handler->eventq, copy_oa_soap_event(&event));

        /* Set the presence state of the OA to PRESENT */
        oa_handler->oa_soap_resources.oa.presence[bay_number - 1] = RES_PRESENT;
        return SA_OK;
}

/**
 * process_oa_extraction_event
 *      @oh_handler: Pointer to openhpi handler structure
 *      @oa_event:   Pointer to oa event response structure
 *
 * Purpose:
 *      Gets the OA extraction event.
 *      Removes OA information from RPT and RDR table
 *      Creates the hot swap event
 *
 * Detailed Description: NA
 *
 * Return values:
 *      SA_OK                     - on success.
 *      SA_ERR_HPI_INVALID_PARAMS - on wrong parameters.
 *      SA_ERR_HPI_INTERNAL_ERROR - on failure
 **/
SaErrorT process_oa_extraction_event(struct oh_handler_state *oh_handler,
                                     struct eventInfo *oa_event)
{
        SaErrorT rv = SA_OK;
        char* entity_root = NULL;
        SaHpiEntityPathT entity_path;
        SaHpiEntityPathT root_entity_path;
        SaHpiRptEntryT *rpt = NULL;
        struct oh_event event;
        struct oa_soap_handler *oa_handler = NULL;
        SaHpiInt32T bay_number;

        if (oh_handler == NULL || oa_event == NULL) {
                err("Invalid parameters");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        oa_handler = (struct oa_soap_handler *) oh_handler->data;
        if (oa_handler == NULL) {
                err("OA SOAP handler is NULL");
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        /* The OA is sending the wrong bay_number for the removed OA
         * Hence if the bay number in the oa_event is 1,
         * process the event for bay number 2, or vice versa
         * TODO: Remove this workaround when the fix is made to OA firmware
         */
        switch (oa_event->eventData.oaStatus.bayNumber) {
                case 1:
                        g_mutex_lock(oa_handler->oa_2->mutex);
                        oa_handler->oa_2->oa_status = ABSENT;
                        g_mutex_unlock(oa_handler->oa_2->mutex);
                        bay_number = 2;
                        break;
                case 2:
                        g_mutex_lock(oa_handler->oa_1->mutex);
                        oa_handler->oa_1->oa_status = ABSENT;
                        g_mutex_unlock(oa_handler->oa_1->mutex);
                        bay_number = 1;
                        break;
                default:
                        err("Wrong OA bay number <%d> detected",
                             oa_event->eventData.oaStatus.bayNumber);
                        return SA_ERR_HPI_INTERNAL_ERROR;
        }

        /* Set the presence state of the OA to ABSENT */
        oa_handler->oa_soap_resources.oa.presence[bay_number - 1] = RES_ABSENT;
        update_hotswap_event(oh_handler, &event);

        /* Build the entity path */
        entity_root = (char *)g_hash_table_lookup(oh_handler->config,
                                                  "entity_root");
        rv = oh_encode_entitypath(entity_root, &root_entity_path);
        if (rv != SA_OK) {
                err("Encoding entity path failed");
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        memset(&entity_path, 0, sizeof(SaHpiEntityPathT));
        entity_path.Entry[1].EntityType = SAHPI_ENT_ROOT;
        entity_path.Entry[1].EntityLocation = 0;
        entity_path.Entry[0].EntityType=SAHPI_ENT_SYS_MGMNT_MODULE;
        entity_path.Entry[0].EntityLocation = bay_number;
        rv = oh_concat_ep(&entity_path, &root_entity_path);
        if (rv != SA_OK) {
                err("concat of entity path failed");
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        /* Get the RPT entry from entity path */
        rpt = oh_get_resource_by_ep(oh_handler->rptcache, &entity_path);
        if (rpt == NULL) {
                err("resource rpt is NULL");
                return SA_ERR_HPI_INTERNAL_ERROR;
        }
        memcpy(&(event.resource), rpt, sizeof(SaHpiRptEntryT));
        event.event.Source = event.resource.ResourceId;

        event.event.EventDataUnion.HotSwapEvent.HotSwapState =
                SAHPI_HS_STATE_NOT_PRESENT;
        event.event.EventDataUnion.HotSwapEvent.PreviousHotSwapState =
                SAHPI_HS_STATE_ACTIVE;

        oh_evt_queue_push(oh_handler->eventq, copy_oa_soap_event(&event));
        /* Remove the resource from RPT and RDR tables */
        rv = oh_remove_resource(oh_handler->rptcache,
                                event.resource.ResourceId);

        return SA_OK;
}

/**
 * process_oa_failover_event
 *      @oh_handler: Pointer to openhpi handler structure
 *      @oa:         Pointer to the OA structure
 *
 * Purpose:
 *      Gets the OA Failover event.
 *      Waits till the OA Transition Complete event is recieved from OA
 *      Gets the OA status of the other OA and updates the other OA data
 *      structure
 *
 * Detailed Description:
 *      - If the OA failover event is recieved just after the discovery on
 *        active OA, then the OA failoer event will be ignored
 *      - The active_con field of oa_handler is pointed to current active OA
 *      - Till OA_TRANSITION_COMPLETE event is recieved or maximum 90 seconds,
 *        all the events are ignored
 *      - OA needs some time to stabilize, after getting the OA failover event,
 *        plug-in starts checking for events after 90 seconds
 *      - Since there are high chances for missing the information of changes
 *        in the resources, re-discovery will be done before start listening
 *        for events.
 *
 * Return values:
 *      SA_OK                     - on success.
 *      SA_ERR_HPI_INVALID_PARAMS - on wrong parameters.
 *      SA_ERR_HPI_INTERNAL_ERROR - on failure
 **/
SaErrorT process_oa_failover_event(struct oh_handler_state *oh_handler,
                                   struct oa_info *oa)
{
        SaErrorT rv = SA_OK;
        SaHpiBoolT is_transition_complete = SAHPI_FALSE;
        SaHpiInt32T sleep_time = 0;
        struct oa_soap_handler *oa_handler = NULL;
        struct getAllEvents request;
        struct getAllEventsResponse response;
        struct eventInfo event;
        GTimer *timer = NULL;
        gulong micro_seconds;
        gdouble time_elapsed = 0;

        if (oh_handler == NULL || oa == NULL) {
                err("Invalid parameters");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        /* If the switchover happens during discovery,
         * then, we will get failover event on Active OA
         * Discovery error recovery mechanism has handled the switchover.
         * Hence, ignore the failover event
         */
        if (oa->oa_status == ACTIVE) {
                dbg("OA failover event is recieved in active OA");
                dbg("Ignoring the OA failover event");
                return SA_OK;
        }

        err("OA got switched over");
        oa_handler = (struct oa_soap_handler *) oh_handler->data;

        /* Always lock the oa_handler mutex and then oa_info mutex
         * This is to avoid the deadlock
         */
        g_mutex_lock(oa_handler->mutex);
        g_mutex_lock(oa->mutex);

        /* Point the active_con to the current active OA's hpi_con */
        oa_handler->active_con = oa->hpi_con;
        /* This OA has become ACTIVE from STANDBY */
        oa->oa_status = ACTIVE;
        g_mutex_unlock(oa->mutex);

        request.pid = oa->event_pid;
        request.waitTilEventHappens = HPOA_TRUE;
        request.lcdEvents = HPOA_FALSE;

        /* Start the timer */
        timer = g_timer_new();
        /* Loop through till the TRANSITION_COMPLETE event is recieved
         * Or OA stabilization (90 seconds) time has reached
         */
        while (is_transition_complete != SAHPI_TRUE &&
              time_elapsed < OA_STABILIZE_MAX_TIME) {

                g_mutex_lock(oa->mutex);
                rv = soap_getAllEvents(oa->event_con, &request, &response);
                g_mutex_unlock(oa->mutex);
                if (rv != SOAP_OK) {
                        err("Get all events failed during OA switchover"
                             "processing for OA %s", oa->server);
                        /* Unlock the oa_handler mutex*/
                        g_mutex_unlock(oa_handler->mutex);
                        /* Cleanup the timer */
                        g_timer_destroy(timer);

                        /* May be OA is out of network or
                         * consecuting switch over has happened
                         * Try to recover from the problem
                         */
                        oa_soap_error_handling(oh_handler, oa);
                        /* Re-discovery is already in error handling
                         * hence skip the below code
                         */
                        return SA_OK;
                }

                /* OA returns empty event response payload for LCD status
                 * change events.  Ignore empty event response.
                 */
                if (response.eventInfoArray == NULL) {
                        dbg("Ignoring empty event response");
                        time_elapsed = g_timer_elapsed(timer, &micro_seconds);
                        continue;
                }

                /* Check for transition complete event */
                while (response.eventInfoArray) {
                       soap_getEventInfo(response.eventInfoArray, &event);
                       if (event.event == EVENT_OA_TRANSITION_COMPLETE) {
                                   is_transition_complete = SAHPI_TRUE;
                                   break;
                       }
                       response.eventInfoArray =
                                soap_next_node(response.eventInfoArray);
                 }
                /* Get the time (in seconds) since the timer has been started */
                time_elapsed = g_timer_elapsed(timer, &micro_seconds);
        }

        /* Unlock the oa_handler mutex */
        g_mutex_unlock(oa_handler->mutex);

        /* Get the time (in seconds) since the timer has been started */
        time_elapsed = g_timer_elapsed(timer, &micro_seconds);
        g_timer_destroy(timer);
        /* OA requires some time to Stabilize. Wait for max 90 seconds */
        sleep_time = OA_STABILIZE_MAX_TIME - time_elapsed;
        if (sleep_time > 0) {
               sleep(sleep_time);
        }

        /* Check the OA staus there may be change in OA state */
        rv = check_oa_status(oa_handler, oa, oa->event_con);
        if (rv != SA_OK) {
                err("Check OA staus failed for OA %s", oa->server);
                oa_soap_error_handling(oh_handler, oa);
                /* Re-discovery is handled in error handling hence skip the
                 * below code
                 */
                return SA_OK;
        }

        /* Check the OA status, if it is not ACTIVE (switchover might have
         * happened while waiting for OA stabilization)
         * Return without doing re-discovery
         */
        g_mutex_lock(oa->mutex);
        if (oa->oa_status != ACTIVE) {
                g_mutex_unlock(oa->mutex);
                return SA_OK;
        }
        g_mutex_unlock(oa->mutex);

        /* Re-discover the resources as there is a high chances
         * that we might have missed some events
         */
        g_mutex_lock(oa_handler->mutex);
        g_mutex_lock(oa->mutex);
        rv = oa_soap_re_discover_resources(oh_handler, oa->event_con);
        g_mutex_unlock(oa->mutex);
        g_mutex_unlock(oa_handler->mutex);

        if (rv != SA_OK) {
                err("Re-discovery failed for OA %s", oa->server);
                oa_soap_error_handling(oh_handler, oa);
        }

        return SA_OK;
}

/**
 * process_oa_info_event
 *      @oh_handler: Pointer to openhpi handler structure
 *      @con:        Pointer to SOAP_CON structure
 *      @oa_event:   Pointer to the OA event structure
 *
 * Purpose:
 *      Gets the OA Info event.
 *      Creates the OA inventory RDR.
 *      Fills the OA URL and creates the session id
 *
 * Detailed Description:
 *      - If the OA_INFO event is recived after the OA insertion event,
 *        then it is processed, else it is ignored
 *      - The OA_INFO event (after the OA insertion event) indicates
 *        the stabilization of OA.
 *      - When OA insertion event recieved, OA will not be stabilized
 *        Hence, the inventory RDR will not be created.
 *      - The OA IP address will not be available to start listening for events
 *
 * Return values:
 *      SA_OK                     - on success.
 *      SA_ERR_HPI_INVALID_PARAMS - on wrong parameters.
 *      SA_ERR_HPI_INTERNAL_ERROR - on failure
 **/
SaErrorT process_oa_info_event(struct oh_handler_state *oh_handler,
                               SOAP_CON *con,
                               struct eventInfo *oa_event)
{
        SaErrorT rv = SA_OK;
        SaHpiInt32T bay_number;
        struct oa_soap_handler *oa_handler = NULL;
        struct oa_info *temp = NULL;
        struct getOaNetworkInfo request;
        struct oaNetworkInfo response;
        struct getOaStatus status_request;
        struct oaStatus status_response;
        char url[MAX_URL_LEN], *user_name = NULL, *password = NULL;

        if (oh_handler == NULL || con == NULL || oa_event == NULL) {
                err("Invalid parameters");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        /* Try to create the OA inventory RDR
         * If the OA invenotry RDR is present,
         * then SA_ERR_HPI_INVALID_CMD will be returned.
         * This means that OA_INFO event is not generated just after OA
         * insertion.  Ignore this event on getting SA_ERR_HPI_INVALID_CMD
         * error code.
         */
        rv = add_oa_inv_area(oh_handler, &(oa_event->eventData.oaInfo));
        if (rv == SA_ERR_HPI_INVALID_CMD) {
                dbg("Ignore the EVENT_OA_INFO event");
                return SA_OK;
        } else if (rv != SA_OK) {
                err("Adding OA inventory area for slot <%d> has failed",
                    oa_event->eventData.oaInfo.bayNumber);
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        oa_handler = (struct oa_soap_handler *) oh_handler->data;
        if (oa_handler == NULL) {
                err("OA SOAP handler is NULL");
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        bay_number = oa_event->eventData.oaInfo.bayNumber;
        switch (bay_number) {
                case 1:
                        temp = oa_handler->oa_1;
                        break;
                case 2:
                        temp = oa_handler->oa_2;
                        break;
                default :
                        err("Wrong bay number %d detected", bay_number);
                        return SA_ERR_HPI_INTERNAL_ERROR;
        }

        /* Get the inserted OA role */
        g_mutex_lock(temp->mutex);
        status_request.bayNumber = bay_number;
        rv = soap_getOaStatus(con, &status_request, &status_response);
        if (rv != SOAP_OK) {
                err("Get OA status failed");
                g_mutex_unlock(temp->mutex);
                return SA_ERR_HPI_INTERNAL_ERROR;
        }
        temp->oa_status = status_response.oaRole;

        /* Get the inserted OA IP address and construct the URL */
        request.bayNumber = bay_number;
        rv = soap_getOaNetworkInfo(con, &request, &response);
        if (rv != SOAP_OK) {
                err("Get OA network info failed");
                g_mutex_unlock(temp->mutex);
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        memset(url, 0, MAX_URL_LEN);
        snprintf(url, strlen(response.ipAddress) + strlen(PORT) + 1,
                 "%s" PORT, response.ipAddress);

        /* Copy the server name to oa_info structure*/
        memset(temp->server, 0, MAX_URL_LEN);
        strncpy(temp->server, response.ipAddress, strlen(response.ipAddress));


        /* Get the OA user name and password from config file */
        user_name = (char *) g_hash_table_lookup(oh_handler->config,
                                                 "OA_User_Name");
        password = (char *) g_hash_table_lookup(oh_handler->config,
                                                "OA_Password");

        /* Close the SOAP_CON structure, if already created */
        if (temp->hpi_con != NULL) {
                soap_close(temp->hpi_con);
                temp->hpi_con = NULL;
        }
        if (temp->event_con != NULL) {
                soap_close(temp->event_con);
                temp->event_con = NULL;
        }

        /* Create the SOAP_CON structure for hpi_con and event_con for the
         * inserted OA
         */
        temp->hpi_con = soap_open(url, user_name, password, HPI_CALL_TIMEOUT);
        if (temp->hpi_con == NULL) {
                err("soap_open for OA <%s> has failed", response.ipAddress);
                g_mutex_unlock(temp->mutex);
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        temp->event_con = soap_open(url, user_name, password,
                                    EVENT_CALL_TIMEOUT);
        if (temp->event_con == NULL) {
                err("soap_open for OA <%s> has failed", response.ipAddress);
                soap_close(temp->hpi_con);
                temp->hpi_con = NULL;
                g_mutex_unlock(temp->mutex);
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        g_mutex_unlock(temp->mutex);
        return SA_OK;
}


/**
 * add_oa_inv_area
 *      @oh_handler: Pointer to openhpi handler
 *      @info:       Pointer OA info response structure
 *
 * Purpose:
 *      Populate the OA Inventory RDR.
 *
 * Detailed Description:
 *      - When OA insertion event recieved, OA will not be stabilized
 *        Hence, the inventory RDR will not be created.
 *        After recived the OA info event, inventory RDR is created
 *      - If the OA inventory is already created, then OA_INFO event recived is
 *        not after the OA insertiong event and ignore the event
 *        Else, create the inventory RDR
 *
 * Return values:
 *      SA_OK                     - on success.
 *      SA_ERR_HPI_INVALID_PARAMS - on wrong parameters
 *      SA_ERR_HPI_INVALID_CMD    - if OA inventory is populated
 *      SA_ERR_HPI_OUT_OF_MEMORY  - on malloc failure
 *      SA_ERR_HPI_INTERNAL_ERROR - on failure.
 **/
SaErrorT add_oa_inv_area(struct oh_handler_state *oh_handler,
                         struct oaInfo *info)
{
        SaErrorT rv = SA_OK;
        SaHpiEntityPathT root_entity_path, entity_path;
        SaHpiRptEntryT *rpt = NULL;
        SaHpiRdrT *rdr = NULL;
        SaHpiIdrIdT idr = SAHPI_DEFAULT_INVENTORY_ID;
        struct oa_soap_inventory *inventory = NULL;
        struct oa_soap_area *head_area = NULL;
        SaHpiInt32T add_success_flag = 0, area_count = 0;
        char *entity_root = NULL;

        if (oh_handler == NULL || info == NULL) {
                err("Invalid parameters");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        /* Construct the entity path */
        entity_root = (char *)g_hash_table_lookup(oh_handler->config,
                                                  "entity_root");
        rv = oh_encode_entitypath(entity_root, &root_entity_path);
        if (rv != SA_OK) {
                err("Encoding entity path failed");
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        memset(&entity_path, 0, sizeof(SaHpiEntityPathT));
        entity_path.Entry[1].EntityType = SAHPI_ENT_ROOT;
        entity_path.Entry[1].EntityLocation = 0;
        entity_path.Entry[0].EntityType = SAHPI_ENT_SYS_MGMNT_MODULE;
        entity_path.Entry[0].EntityLocation = info->bayNumber;

        rv = oh_concat_ep(&entity_path, &root_entity_path);
        if (rv != SA_OK) {
                err("concat of entity path failed");
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        /* Get the RPT entry from entity path */
        rpt = oh_get_resource_by_ep(oh_handler->rptcache, &entity_path);
        if (rpt == NULL) {
                err("resource RPT is NULL");
                return SA_ERR_HPI_NOT_PRESENT;
        }

        /* Get the inventory RDR from resource id */
        rdr = oh_get_rdr_by_type(oh_handler->rptcache, rpt->ResourceId,
                                 SAHPI_INVENTORY_RDR, idr);

        if (rdr == NULL) {
                err("INVALID RESOURCE ID");
                return SA_ERR_HPI_NOT_PRESENT;
        }

        /* Get the inventory from RDR data */
        inventory = (struct oa_soap_inventory *)
                oh_get_rdr_data(oh_handler->rptcache,
                                rpt->ResourceId, rdr->RecordId);
        if (inventory == NULL) {
                err("IDR inventory not present");
                return SA_ERR_HPI_NOT_PRESENT;
        }

        /* Check for the presence of OA inventory RDR
         * If the OA invenotry RDR is present,
         * then SA_ERR_HPI_INVALID_CMD will be returned.
         * This means that OA_INFO event is not generated just after OA
         * insertion.  Ignore this event on getting SA_ERR_HPI_INVALID_CMD
         * error code.
         */
        if (inventory->info.area_list != NULL) {
                dbg("OA inventory is already populated");
                return SA_ERR_HPI_INVALID_CMD;
        }

        /* Create and add product area if resource name and(or) manufacturer
         * information exist
         */
        rv = add_product_area(&inventory->info.area_list, info->name,
                              info->manufacturer, &add_success_flag);
        if (rv != SA_OK) {
                err("Out of memory");
                return rv;
        }
        if (add_success_flag != SAHPI_FALSE) {
                (inventory->info.idr_info.NumAreas)++;
                if (area_count == 0) {
                        head_area = inventory->info.area_list;
                }
                ++area_count;
        }

        /* Create and add board area if resource part number and(or)
         * serial number exist
         */
        rv = add_board_area(&inventory->info.area_list, info->partNumber,
                            info->serialNumber, &add_success_flag);
        if (rv != SA_OK) {
                err("Out of memory");
                return rv;
        }
        if (add_success_flag != SAHPI_FALSE) {
                (inventory->info.idr_info.NumAreas)++;
                if (area_count == 0) {
                        head_area = inventory->info.area_list;
                }
                ++area_count;
        }

        inventory->info.area_list = head_area;

        return SA_OK;
}

/**
 * build_inserted_oa_rdr
 *      @oh_handler: Pointer to openhpi handler
 *      @con:        Pointer to SOAP_CON
 *      @bay_number: Bay number of the inserted OA
 *      @event:      Pointer to event structure
 *
 * Purpose:
 *      Populate the OA RDR for the inserted OA.
 *      Pushes the RDRs to infrastructure
 *
 * Detailed Description:
 *      - On inserting a OA, OA takes some times to stabilize.
 *        Except inventory RDR and OA IP address, all other information is
 *        available.
 *      - This method populates all the RDRs except inventory rdr
 *      - Bare minimum inventory RDR is created which gets populated later
 *        after OA_INFO event is recieved.
 *
 * Return values:
 *      SA_OK                     - on success.
 *      SA_ERR_HPI_INVALID_PARAMS - on wrong parameters
 *      SA_ERR_HPI_OUT_OF_MEMORY  - on malloc failure
 *      SA_ERR_HPI_INTERNAL_ERROR - on failure.
 **/
SaErrorT build_inserted_oa_rdr(struct oh_handler_state *oh_handler,
                               SOAP_CON *con,
                               SaHpiInt32T bay_number,
                               struct oh_event *event)
{
        SaErrorT rv = SA_OK;
        SaHpiRdrT *rdr = NULL;
        struct oa_soap_inventory *inventory = NULL;
        struct oa_soap_sensor_info *sensor_info=NULL;

        if (oh_handler == NULL || con == NULL || event == NULL) {
                err("Invalid parameters");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        rdr = (SaHpiRdrT *)g_malloc0(sizeof(SaHpiRdrT));
        if (rdr == NULL) {
                err("Out of memory");
                return SA_ERR_HPI_OUT_OF_MEMORY;
        }
        /* Build the bare minimum inventory RDR without any inventory
         * information
         */
        rv = build_inserted_oa_inv_rdr(oh_handler, bay_number, rdr, &inventory);
        if (rv != SA_OK) {
                err("Failed to build OA inventory RDR");
                g_free(rdr);
                return rv;
        }
        rv = oh_add_rdr(oh_handler->rptcache, event->resource.ResourceId,
                        rdr, inventory, 0);
        if (rv != SA_OK) {
                err("Failed to add rdr");
                g_free(rdr);
                return rv;
        }
        event->rdrs = g_slist_append(event->rdrs, rdr);

        rdr = NULL;
        rdr = (SaHpiRdrT *)g_malloc0(sizeof(SaHpiRdrT));
        if (rdr == NULL) {
                err("Out of memory");
                del_rdr_from_event(event);
                return SA_ERR_HPI_OUT_OF_MEMORY;
        }
        /* Build the thermal RDR */
        rv = build_oa_thermal_sensor_rdr(oh_handler, con,
                                         bay_number, rdr, &sensor_info);
        if (rv != SA_OK) {
                err("Failed to get sensor rdr for OA");
                del_rdr_from_event(event);
                g_free(rdr);
                return rv;
        }
        rv = oh_add_rdr(oh_handler->rptcache, event->resource.ResourceId,
                        rdr, sensor_info, 0);
        if (rv != SA_OK) {
                err("Failed to add rdr");
                del_rdr_from_event(event);
                g_free(rdr);
                return rv;
        }
        event->rdrs = g_slist_append(event->rdrs, rdr);

        return rv;
}

/**
 * build_oa_inv_rdr:
 *      @oh_handler: Handler data pointer.
 *      @bay_number: Bay number of the inserted OA.
 *      @rdr:        Rdr Structure for inventory data.
 *      @inventory:  Rdr private data structure.
 *
 * Purpose:
 *      Creates an bare minimum inventory rdr without any inventory information
 *      for the newly inserted Onboard Administator.
 *
 * Detailed Description: NA
 *
 * Return values:
 *      SA_OK                     - Normal case.
 *      SA_ERR_HPI_INVALID_PARAMS - on wrong parameters
 *      SA_ERR_HPI_NOT_PRESENT    - Requested object not present.
 *      SA_ERR_HPI_OUT_OF_MEMORY  - Request failed due to insufficient memory
 **/
SaErrorT build_inserted_oa_inv_rdr(struct oh_handler_state *oh_handler,
                                   SaHpiInt32T bay_number,
                                   SaHpiRdrT *rdr,
                                   struct oa_soap_inventory **inventory)
{
        SaErrorT rv = SA_OK;
        SaHpiEntityPathT entity_path;
        char *entity_root = NULL;
        char *oa_inv_str = OA_INVENTORY_STRING;
        struct oa_soap_inventory *oa_inventory = NULL;

        if (oh_handler == NULL || rdr== NULL || inventory == NULL) {
                err("Invalid parameters");
                return SA_ERR_HPI_INVALID_PARAMS;
        }

        /* Build the entity path */
        entity_root = (char *)g_hash_table_lookup(oh_handler->config,
                                                  "entity_root");
        rv = oh_encode_entitypath(entity_root, &entity_path);
        if (rv != SA_OK) {
                err("Encoding entity path failed");
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        rdr->Entity.Entry[1].EntityType = SAHPI_ENT_ROOT;
        rdr->Entity.Entry[1].EntityLocation = 0;
        rdr->Entity.Entry[0].EntityType = SAHPI_ENT_SYS_MGMNT_MODULE;
        rdr->Entity.Entry[0].EntityLocation = bay_number;
        rv = oh_concat_ep(&rdr->Entity, &entity_path);
        if (rv != SA_OK) {
                err("concat of entity path failed");
                return SA_ERR_HPI_INTERNAL_ERROR;
        }

        /* Populate the inventory RDR with default values */
        rdr->RecordId = 0;
        rdr->RdrType  = SAHPI_INVENTORY_RDR;
        rdr->RdrTypeUnion.InventoryRec.IdrId = SAHPI_DEFAULT_INVENTORY_ID;
        rdr->IdString.DataType = SAHPI_TL_TYPE_TEXT;
        rdr->IdString.Language = SAHPI_LANG_ENGLISH;
        rdr->IdString.DataLength = strlen(OA_NAME) + 1;
        snprintf((char *)rdr->IdString.Data, strlen(OA_NAME)+ 1, OA_NAME);

        /* Create inventory IDR and populate the IDR header and keep
         * area list as empty
         */
        oa_inventory = (struct oa_soap_inventory*)
                g_malloc0(sizeof(struct oa_soap_inventory));
        if (oa_inventory == NULL) {
                err("Out of memory");
                return SA_ERR_HPI_OUT_OF_MEMORY;
        }
        oa_inventory->inv_rec.IdrId = rdr->RdrTypeUnion.InventoryRec.IdrId;
        oa_inventory->info.idr_info.IdrId =
                rdr->RdrTypeUnion.InventoryRec.IdrId;
        oa_inventory->info.idr_info.UpdateCount = 1;
        oa_inventory->info.idr_info.ReadOnly = SAHPI_FALSE;
        oa_inventory->info.idr_info.NumAreas = 0;
        oa_inventory->info.area_list = NULL;
        oa_inventory->info.idr_info.NumAreas = 0;
        oa_inventory->info.area_list = NULL;
        oa_inventory->comment = (char *)g_malloc0(strlen(oa_inv_str) + 1);

        snprintf(oa_inventory->comment, strlen(oa_inv_str) + 1,
                 "%s", oa_inv_str);
        *inventory = oa_inventory;
        return SA_OK;
}

