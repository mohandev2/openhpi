/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2005
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *        Renier Morales <renierm@users.sf.net>
 *        Daniel de Araujo <ddearauj@us.ibm.com>
 */
#include <rtas_event.h> 

SaErrorT rtas_get_event(void *hnd, struct oh_event *event)
{
        return SA_ERR_HPI_INTERNAL_ERROR;
}

void * oh_get_event (void *, struct oh_event *)
        __attribute__ ((weak, alias("rtas_get_event")));
