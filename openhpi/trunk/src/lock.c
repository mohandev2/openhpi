/*      -*- linux-c -*-
 *
 * Copyright (c) 2003 by Intel Corp.
 * (C) Copyright IBM Corp. 2003
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Authors:
 *     Louis Zhuang <louis.zhuang@linux.intel.com>
 */
#include <oh_lock.h>

oh_will_block = 0;
		 
#ifdef HAVE_THREAD_SAFE
/* multi-threading support, use Posix mutex for data access */
/* initialize mutex used for data locking */
#include <glib/gthread.h>

GStaticRecMutex oh_main_lock = G_STATIC_REC_MUTEX_INIT;

int data_access_block_times(void) 
{
        return(oh_will_block);
}

#else 

GStaticRecMutex oh_main_lock = NULL;

int data_access_block_times(void){ return(0);}

#endif/*HAVE_THREAD_SAFE*/
