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
 * Authors:
 *     Renier Morales <renierm@users.sf.net>
 */
 
#include <stdlib.h>
#include <string.h>
#include <SaHpi.h>
#include <oHpi.h>

/**
 * Load 'libdummy', create handler, get handler info and check for
 * expected information. Destroy handler, unload plugin.
 * Pass on success, otherwise failure.
 **/
 
int main(int argc, char **argv)
{
        SaHpiSessionIdT sid = 0;
        char *config_file = NULL;
        GHashTable *config = g_hash_table_new(g_str_hash, g_str_equal);
        oHpiHandlerIdT hid = 0;
        oHpiHandlerInfoT hinfo;
        
        /* Save config file env variable and unset it */
        config_file = getenv("OPENHPI_CONF");
        setenv("OPENHPI_CONF","./noconfig", 1);
        
        if (saHpiSessionOpen(1, &sid, NULL))
                return -1;
                
        if (oHpiPluginLoad("libdummy"))
                return -1;
                
        /* Set configuration. */
        g_hash_table_insert(config, "plugin", "libdummy");
        g_hash_table_insert(config, "entity_root", "{SYSTEM_CHASSIS,1}");
        g_hash_table_insert(config, "name", "test");
        g_hash_table_insert(config, "addr", "0");
        
        if (oHpiHandlerCreate(config, &hid))
                return -1;
                
        if (oHpiHandlerInfo(hid, &hinfo))
                return -1;
                
        if (strcmp("libdummy",hinfo.plugin_name))
                return -1;
                
        if (oHpiHandlerDestroy(hid))
                return -1;
                
        
        return oHpiPluginUnload("libdummy");
}
