/*
 * ipmi_entity.h
 *
 * Copyright (c) 2003,2004 by FORCE Computers
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
 */

#ifndef dIpmiEntity_h
#define dIpmiEntity_h


#include <glib.h>
#include <string.h>


__BEGIN_DECLS
#include "SaHpi.h"
__END_DECLS


#ifndef dIpmiEvent_h
#include "ipmi_event.h"
#endif


enum tIpmiEntityId
{
  eIpmiEntityInvalid                        =  0,
  eIpmiEntityIdOther                        =  1,
  eIpmiEntityIdUnkown                       =  2,
  eIpmiEntityIdProcessor                    =  3,
  eIpmiEntityIdDisk                         =  4,
  eIpmiEntityIdPeripheral                   =  5,
  eIpmiEntityIdSystemManagementModule       =  6,
  eIpmiEntityIdSystemBoard                  =  7,
  eIpmiEntityIdMemoryModule                 =  8,
  eIpmiEntityIdProcessorModule              =  9,
  eIpmiEntityIdPowerSupply                  = 10,
  eIpmiEntityIdAddInCard                    = 11,
  eIpmiEntityIdFrontPanelBoard              = 12,
  eIpmiEntityIdBackPanelBoard               = 13,
  eIpmiEntityIdPowerSystemBoard             = 14,
  eIpmiEntityIdDriveBackplane               = 15,
  eIpmiEntityIdSystemInternalExpansionBoard = 16,
  eIpmiEntityIdOtherSystemBoard             = 17,
  eIpmiEntityIdProcessorBoard               = 18,
  eIpmiEntityIdPowerUnit                    = 19,
  eIpmiEntityIdPowerModule                  = 20,
  eIpmiEntityIdPowerManagementBoard         = 21,
  eIpmiEntityIdChassisBackPanelBoard        = 22,
  eIpmiEntityIdSystemChassis                = 23,
  eIpmiEntityIdSubChassis                   = 24,
  eIpmiEntityIdOtherChassisBoard            = 25,
  eIpmiEntityIdDiskDriveBay                 = 26,
  eIpmiEntityIdPeripheralBay                = 27,
  eIpmiEntityIdDeviceBay                    = 28,
  eIpmiEntityIdFanCooling                   = 29,
  eIpmiEntityIdCoolingUnit                  = 30,
  eIpmiEntityIdCableInterconnect            = 31,
  eIpmiEntityIdMemoryDevice                 = 32,
  eIpmiEntityIdSystemManagementSoftware     = 33,
  eIpmiEntityIdBios                         = 34,
  eIpmiEntityIdOperatingSystem              = 35,
  eIpmiEntityIdSystemBus                    = 36,
  eIpmiEntityIdGroup                        = 37,
  eIpmiEntityIdRemoteMgmtCommDevice         = 38,
  eIpmiEntityIdExternalEnvironment          = 39,
  eIpmiEntityIdBattery                      = 40,
  eIpmiEntityIdProcessingBlade              = 41,
  eIpmiEntityIdConnectivitySwitch           = 42,
  eIpmiEntityIdProcessorMemoryModule        = 43,
  eIpmiEntityIdIoModule                     = 44,
  eIpmiEntityIdProcessorIoModule            = 45,
  eIpmiEntityIdMgmtControllerFirmware       = 46,

  // PIGMIG entity ids
  eIpmiEntityIdPigMgFrontBoard              = 0xa0,
  eIpmiEntityIdPigMgRearTransitionModule    = 0xc0,
  eIpmiEntityIdAtcaShelfManager             = 0xf0,
  eIpmiEntityIdAtcaFiltrationUnit           = 0xf1,
};

const char *IpmiEntityIdToString( tIpmiEntityId id );


// wrapper class for entity path
class cIpmiEntityPath
{
public:
  SaHpiEntityPathT m_entity_path;

  cIpmiEntityPath();
  cIpmiEntityPath( const SaHpiEntityPathT &entity_path );

  operator SaHpiEntityPathT()
  {
    return m_entity_path;
  }

  cIpmiEntityPath &operator+=( const cIpmiEntityPath &epath );
  bool AppendRoot();
};


cIpmiLog &operator<<( cIpmiLog &log, const cIpmiEntityPath &epath );


#endif
