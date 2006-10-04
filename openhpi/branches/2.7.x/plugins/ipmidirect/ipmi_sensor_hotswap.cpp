/*
 * ipmi_sensor_hotswap.cpp
 *
 * Copyright (c) 2004 by FORCE Computers
 * Copyright (c) 2005 by ESO Technologies.
 *
 * Note that this file is based on parts of OpenIPMI
 * written by Corey Minyard <minyard@mvista.com>
 * of MontaVista Software. Corey's code was helpful
 * and many thanks go to him. He gave the permission
 * to use this code in OpenHPI under BSD license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Authors:
 *     Thomas Kanngieser <thomas.kanngieser@fci.com>
 *     Pierre Sangouard  <psangouard@eso-tech.com>
 */

#include "ipmi_sensor_hotswap.h"
#include "ipmi_domain.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <math.h>
#include <oh_utils.h>

cIpmiSensorHotswap::cIpmiSensorHotswap( cIpmiMc *mc )
  : cIpmiSensorDiscrete( mc )
#ifdef FAKE_ECN_BEHAVIOR
  ,m_deactivation_locked( 1 ),
  m_fake_deactivation_locked( false )
#endif
{
}


cIpmiSensorHotswap::~cIpmiSensorHotswap()
{
}


bool
cIpmiSensorHotswap::GetDataFromSdr( cIpmiMc *mc, cIpmiSdr *sdr )
{
  if ( !cIpmiSensorDiscrete::GetDataFromSdr( mc, sdr ) )
       return false;

#ifdef FAKE_ECN_BEHAVIOR
  if ( mc->IsNotEcn() )
      m_fake_deactivation_locked = true;
#endif

  return true;
}


bool
cIpmiSensorHotswap::CreateRdr( SaHpiRptEntryT &resource,
                               SaHpiRdrT &rdr )
{
  if ( cIpmiSensorDiscrete::CreateRdr( resource, rdr ) == false )
       return false;

  if ( !(resource.ResourceCapabilities & SAHPI_CAPABILITY_MANAGED_HOTSWAP) )
     {
       // update resource capabilities
       resource.ResourceCapabilities |= SAHPI_CAPABILITY_MANAGED_HOTSWAP;
       resource.HotSwapCapabilities = SAHPI_HS_CAPABILITY_INDICATOR_SUPPORTED;

       struct oh_event *e = (struct oh_event *)g_malloc0( sizeof( struct oh_event ) );

       e->event.EventType = SAHPI_ET_HOTSWAP;
       e->event.EventDataUnion.HotSwapEvent.HotSwapState = SAHPI_HS_STATE_ACTIVE;
       e->event.EventDataUnion.HotSwapEvent.PreviousHotSwapState = SAHPI_HS_STATE_ACTIVE;
       e->resource = resource;
       e->event.Source = resource.ResourceId;
       oh_gettimeofday(&e->event.Timestamp);
       e->event.Severity = resource.ResourceSeverity;

       stdlog << "cIpmiSensorHotswap::CreateRdr SAHPI_ET_RESOURCE Event resource " << resource.ResourceId << "\n";
       m_mc->Domain()->AddHpiEvent( e );
     }

  return true;
}


SaHpiHsStateT
cIpmiSensorHotswap::ConvertIpmiToHpiHotswapState( tIpmiFruState h )
{
  switch( h )
     {
       case eIpmiFruStateNotInstalled:
            return SAHPI_HS_STATE_NOT_PRESENT;

       case eIpmiFruStateInactive:
            return SAHPI_HS_STATE_INACTIVE;

       case eIpmiFruStateActivationRequest:
            return SAHPI_HS_STATE_INSERTION_PENDING;

       case eIpmiFruStateActivationInProgress:
            return SAHPI_HS_STATE_INSERTION_PENDING;

       case eIpmiFruStateActive:
            return SAHPI_HS_STATE_ACTIVE;

       case eIpmiFruStateDeactivationRequest:
            return SAHPI_HS_STATE_EXTRACTION_PENDING;

       case eIpmiFruStateDeactivationInProgress:
            return SAHPI_HS_STATE_EXTRACTION_PENDING;

       case eIpmiFruStateCommunicationLost:
       default:
            return SAHPI_HS_STATE_NOT_PRESENT;
     }
}


SaErrorT
cIpmiSensorHotswap::CreateEvent( cIpmiEvent *event, SaHpiEventT &h )
{
  memset( &h, 0, sizeof( SaHpiEventT ) );

  cIpmiResource *res = Resource();

  if (!res)
  {
      return SA_ERR_HPI_NOT_PRESENT;
  }

  h.Source    = res->m_resource_id;
  h.EventType = SAHPI_ET_HOTSWAP;

  // Hot swap events must be dated here otherwise
  // hot swap policy won't work properly !
  oh_gettimeofday(&h.Timestamp);

  // Do not find the severity of hotswap event
  h.Severity = SAHPI_INFORMATIONAL;

  SaHpiHotSwapEventT &he = h.EventDataUnion.HotSwapEvent;

  tIpmiFruState state = (tIpmiFruState)(event->m_data[10] & 0x07);
  he.HotSwapState = ConvertIpmiToHpiHotswapState( state );

  state    = (tIpmiFruState)(event->m_data[11] & 0x07);
  he.PreviousHotSwapState = ConvertIpmiToHpiHotswapState( state );

  // Nothing changed -> no HS event will be sent
  if (he.HotSwapState == he.PreviousHotSwapState)
      return SA_ERR_HPI_DUPLICATE;

#ifdef FAKE_ECN_BEHAVIOR
  if (he.HotSwapState == SAHPI_HS_STATE_ACTIVE)
      m_deactivation_locked = 1;
  else
      m_deactivation_locked = 0;
#endif

  return SA_OK;
}


SaErrorT
cIpmiSensorHotswap::GetState( tIpmiFruState &state )
{
  // read hotswap state
  cIpmiMsg rsp;

  // Default value just in case
  state = eIpmiFruStateCommunicationLost;

  SaErrorT rv = GetSensorData( rsp );

  if ( rv != SA_OK )
     {
       stdlog << "cannot get hotswap state !\n";
       return rv;
     }

  // Reading should be 0 according to PICMG 3.0 specification
  // However if it's not this is not a fatal error so just flag it
  if ( rsp.m_data[1] != 0 )
     {
       stdlog << "WARNING: hotswap sensor reading not 0 : " << rsp.m_data[1] << " !\n";
     }

  unsigned int value = rsp.m_data[3];

  for( int i = 0; i < 8; i++ )
       if ( value & ( 1 << i ) )
          {
            state = (tIpmiFruState)i;
            return SA_OK;
          }

  stdlog << "WRONG Hot Swap State " << value << "\n";

  return SA_ERR_HPI_INVALID_DATA;
}


SaErrorT
cIpmiSensorHotswap::GetState( SaHpiHsStateT &state )
{
  tIpmiFruState fs;
  SaErrorT rv = GetState( fs );

  if ( rv != SA_OK )
       return rv;

  state = ConvertIpmiToHpiHotswapState( fs );

#ifdef FAKE_ECN_BEHAVIOR
  if (( state == SAHPI_HS_STATE_ACTIVE )
      && ( m_fake_deactivation_locked )
      && ( m_deactivation_locked == 0 ))
                state = SAHPI_HS_STATE_EXTRACTION_PENDING;
#endif
  return SA_OK;
}
