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

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <openhpi.h>

static enum {
        UNINIT,
        INIT
} oh_init_state;

SaErrorT oh_initialized()
{
        data_access_lock();
        if(oh_init_state == UNINIT) {
                data_access_unlock();
                return SA_ERR_HPI_ERROR;
        } else {
                data_access_unlock();
                return SA_OK;
        }
}

/**
 * oh_initialize
 *
 * Returns:
 **/
SaErrorT oh_initialize()
{
        struct oh_plugin_config *tmpp;
        GHashTable *tmph;

        unsigned int i;
        char *openhpi_conf;

        int rval;

        SaHpiDomainCapabilitiesT capabilities = 0x00000000;
        SaHpiTextBufferT tag;

        /* Initialize event process queue */
        oh_event_init();

        data_access_lock();


        /* Check ready state */
        if (oh_initialized() == SA_OK) {
                dbg("Cannot initialize twice");
                data_access_unlock();
                return SA_ERR_HPI_DUPLICATE;
        }

        /* Set openhpi configuration file location */
        openhpi_conf = getenv("OPENHPI_CONF");

        if (openhpi_conf == NULL) {
                openhpi_conf = OH_DEFAULT_CONF;
        }

        oh_init_ltdl(); /* Must initialize ltdl before loading plugins */
        if (oh_load_config(openhpi_conf) < 0) {
                dbg("Can not load config");
                data_access_unlock();
                return SA_ERR_HPI_NOT_PRESENT;
        }

        /* Initialize uid_utils */
        rval = oh_uid_initialize();
        if( (rval != SA_OK) && (rval != SA_ERR_HPI_ERROR) ) {
                dbg("uid_intialization failed");
                data_access_unlock();
                return(rval);
        }        
        trace("Initialized UID");

        /* Initialize plugins */
        for(i = 0; i < g_slist_length(global_plugin_list); i++) {
                tmpp = (struct oh_plugin_config *) g_slist_nth_data(
                        global_plugin_list, i);
                if(load_plugin(tmpp) == 0) {
                        trace("Loaded plugin %s", tmpp->name);
                        tmpp->refcount++;
                } else {
                        dbg("Couldn't load plugin %s", tmpp->name);
                }
        }

        /* Initialize handlers */
        for(i = 0; i < g_slist_length(global_handler_configs); i++) {
                tmph = (GHashTable *) g_slist_nth_data(
                        global_handler_configs, i);
                if(plugin_refcount((char *)g_hash_table_lookup(tmph, "plugin")) > 0) {
                        if(load_handler(tmph) == 0) {
                                trace("Loaded handler for plugin %s",
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

        /* Check if we have at least one handler */
        if ( global_handler_list == 0 ) {
                /* there is no handler => this can not work */
                dbg("no handler found. please check %s!", openhpi_conf);

                data_access_unlock();                

                return SA_ERR_HPI_NOT_PRESENT;
        }        

        /* Initialize domain table */
        oh_domains.table = g_hash_table_new(g_int_hash, g_int_equal);
        trace("Initialized domain table");

        /* Create first domain */
        tag.DataType = SAHPI_TL_TYPE_TEXT;
        tag.Language = SAHPI_LANG_ENGLISH;
        strcpy(tag.Data, "First Domain");
        if (!oh_create_domain(capabilities, SAHPI_FALSE, &tag)) {
                dbg("Could not create first domain!");
                return SA_ERR_HPI_ERROR;
        }        
        trace("Created first domain");

        /* Initialize session table */        
        oh_sessions.table = g_hash_table_new(g_int_hash, g_int_equal);
        trace("Initialized session table");
        
        oh_init_state = INIT;
        trace("Set init state");

        data_access_unlock();

        return SA_OK;
}

#if 0
/**
 * oh_finalize
 *
 * Returns:
 **/
SaErrorT oh_finalize()
{
        OH_STATE_READY_CHECK;

        /* free mutex */
        /* TODO: this wasn't here in branch, need to resolve history */
        data_access_lock();

        /* TODO: realy should have a oh_uid_finalize() that */
        /* frees memory,                                    */
        if(oh_uid_map_to_file())
                dbg("error writing uid entity path mapping to file");

        /* check for open sessions */
        if ( global_session_list ) {
                dbg("cannot saHpiFinalize because of open sessions" );
                data_access_unlock();
                return SA_ERR_HPI_BUSY;
        }

        /* close all plugins */
        while(global_handler_list) {
                struct oh_handler *handler = (struct oh_handler *)global_handler_list->data;
                /* unload_handler will remove handler from global_handler_list */
                unload_handler(handler);
        }

        /* unload plugins */
        while(global_plugin_list) {
                struct oh_plugin_config *plugin = (struct oh_plugin_config *)global_plugin_list->data;
                unload_plugin(plugin);
        }

        /* free global rpt */
        if (default_rpt) {
                oh_flush_rpt(default_rpt);
                g_free(default_rpt);
                default_rpt = 0;
        }

        /* free global_handler_configs and uninit_plugin */
        oh_unload_config();
        oh_exit_ltdl(); /* Free ltdl structures after unloading plugins */

        /* free domain list */
        oh_cleanup_domain();

        oh_hpi_state = OH_STAT_UNINIT;

        /* free mutex */
        data_access_unlock();

        return SA_OK;
}
#endif
