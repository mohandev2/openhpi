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
 *     Chris Chia <cchia@users.sf.net>
 *
 */

#include <string.h>
#include <stdio.h>
#include <SaHpi.h>
#include <epath_utils.h>

/**
 * set_ep_instance test2.
 *   call set_ep_instance with entity path that has 1 element
 *   expected result: instance number changed
 *
 * Return value: 0 on success, 1 on failure
 **/
int main(int argc, char **argv)
{
        SaHpiEntityPathT ep = {{{SAHPI_ENT_OTHER, 1},{0}}};
        SaHpiEntityLocationT x = 5;
        int mydebug = 0;
         
        if (mydebug) printf(" test2\n");
        if(set_ep_instance(&ep, SAHPI_ENT_OTHER, x)) {
                if (mydebug) printf("set_ep_inst test2 checkpoint 1 failed\n");
                return 1;
        }
        if (ep.Entry[0].EntityLocation != x) {
                if (mydebug) printf("set_ep_inst test2 failed, entInst %d != %d\n",
                                   ep.Entry[0].EntityLocation, x);
                return 1;
        }
        if (ep.Entry[0].EntityType != SAHPI_ENT_OTHER) {
                if (mydebug) printf("set_ep_inst test2 failed, entType %d != SAHPI_ENT_OTHER\n",
                                   ep.Entry[0].EntityType);
                return 1;
        }
        return 0;
}
