/*
 * (C) Copyright 2016-2017 Hewlett Packard Enterprise Development LP
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
 * Neither the name of the Hewlett Packard Enterprise, nor the names
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
 *      Mohan Devarajulu <mohan.devarajulu@hpe.com>
 *      Hemantha Beecherla <hemantha.beecherla@hpe.com>
 *      Chandrashekhar Nandi <chandrashekhar.nandi@hpe.com>
 *      Shyamala Hirepatt  <shyamala.hirepatt@hpe.com>
 *
 **/

#ifndef _OV_REST_CONTROL_H
#define _OV_REST_CONTROL_H

/* Include files */
#include <SaHpiOvRest.h>
#include "ov_rest.h"
#include "ov_rest_power.h"
#include "ov_rest_resources.h"
#include "ov_rest_parser_calls.h"

/* Tag for the control rdr */
#define SERVER_CONTROL_STRING "Server Power Control"
#define INTERCONNECT_CONTROL_STRING "Interconnect Power Control"

/* Declaration of the functions related to control functionality */
SaErrorT ov_rest_get_control_state(void *oh_handler,
                                   SaHpiResourceIdT rid,
                                   SaHpiCtrlNumT rdr_num,
                                   SaHpiCtrlModeT *mode,
                                   SaHpiCtrlStateT *state);

SaErrorT ov_rest_set_control_state(void *oh_handler,
                                   SaHpiResourceIdT rid,
                                   SaHpiCtrlNumT rdr_num,
                                   SaHpiCtrlModeT mode,
                                   SaHpiCtrlStateT *state);

/* Set analog limits to 0 if building a non-analog control. */
SaErrorT ov_rest_build_control_rdr(struct oh_handler_state *oh_handler,
                           SaHpiRdrT *rdr,
                           SaHpiResourceIdT resource_id,
                           SaHpiCtrlNumT control_num,
                           int analogLimitLow,
                           int analogLimitHigh);


#endif  //_OV_REST_CONTROL_H


