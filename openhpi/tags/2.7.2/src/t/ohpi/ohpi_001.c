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
#include <stdio.h>
#include <SaHpi.h>
#include <oHpi.h>

/**
 * Load the simulator plugin and unload it.
 * Pass on success, otherwise a failure.
 **/
 
int main(int argc, char **argv)
{
        SaHpiSessionIdT sid = 0;
        
        setenv("OPENHPI_CONF","./noconfig", 1);
        
        if (saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID, &sid, NULL)) {
		printf("Failed to open a session.");
                return -1;
	}
                    
        if (oHpiPluginLoad("libsimulator")) {
		printf("Failed to load simulator pluin.");
	        return -1;
	}
                
        if (oHpiPluginUnload("libsimulator")) {
		printf("Failed to unload simulator plugin.");
		return -1;
	}

	return 0;
}
