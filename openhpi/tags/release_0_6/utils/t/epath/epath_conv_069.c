/* -*- linux-c -*-
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
 *     Sean Dague <http://dague.net/sean>
 *
 */

#include <string.h>
#include <SaHpi.h>
#include <epath_utils.h>

/**
 * main: epathstr -> epath test
 * 
 * This test tests whether an entity path string is converted into
 * an entity path properly.  
 *
 * TODO: a more extensive set of tests would be nice, might need to create a
 * perl program to generate that code
 * 
 * Return value: 0 on success, 1 on failure
 **/
int main(int argc, char **argv) 
{
        char new[255];
        SaHpiEntityPathT tmp_ep;
        char *entity_root = "{MEMORY_MODULE,29}{ALARM_MANAGER_BLADE,96}";
        
        string2entitypath(entity_root, &tmp_ep);
         
        if(tmp_ep.Entry[0].EntityType != SAHPI_ENT_ALARM_MANAGER_BLADE)
                return 1;
                
        if(tmp_ep.Entry[0].EntityInstance != 96)
                return 1;
        
        if(tmp_ep.Entry[1].EntityType != SAHPI_ENT_MEMORY_MODULE)
                return 1;
        
        if(tmp_ep.Entry[1].EntityInstance != 29)
                return 1;

        if(entitypath2string(&tmp_ep, new, 255) < 0) 
                return 1;
        
        if(strcmp(new,entity_root) != 0)
                return 1;

        return 0;
}
