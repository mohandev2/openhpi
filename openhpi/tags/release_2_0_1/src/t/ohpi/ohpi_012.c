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
#include <SaHpi.h>
#include <oHpi.h>

/**
 * Load the plugin passin bogus plugin name as argument.
 * Pass on error, otherwise test failed.
 **/
 
int main(int argc, char **argv)
{
        SaHpiSessionIdT sid = 0;
        
        setenv("OPENHPI_CONF","./noconfig", 1);
        
        if (saHpiSessionOpen(1, &sid, NULL))
                return -1;
                
        
        return !oHpiPluginLoad("bogusplugin");
}
