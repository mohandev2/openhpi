/*
 * ipmi_mc_vendor.h
 *
 * Copyright (c) 2004 by FORCE Computers
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

#ifndef dIpmiMcVendor_h
#define dIpmiMcVendor_h


#ifndef dIpmiSensorHotswap_h
#include "ipmi_sensor_hotswap.h"
#endif

#ifndef dIpmiSensorThreshold_h
#include "ipmi_sensor_threshold.h"
#endif

#ifndef dIpmiMsg_h
#include "ipmi_msg.h"
#endif

#ifndef dIpmiFruInfo_h
#include "ipmi_fru_info.h"
#endif


class cIpmiMc;


class cIpmiMcVendor
{
  static SaHpiEntityInstanceT m_unique_instance;

public:
  unsigned int m_manufacturer_id;
  unsigned int m_product_id;
  char         m_description[80];

  cIpmiMcVendor( unsigned int manufacturer_id,
                 unsigned int product_id,
                 const char *desc );

  virtual ~cIpmiMcVendor();

  SaHpiEntityInstanceT GetUniqueInstance()
  {
    return m_unique_instance++;
  }

  // a new MC is found
  virtual bool InitMc( cIpmiMc *mc, const cIpmiMsg &devid );

  // cleanup code for an MC
  virtual void CleanupMc( cIpmiMc *mc );

  // give vendors a chance to modify the sdrs
  virtual bool ProcessSdr( cIpmiDomain *domain, cIpmiMc *mc, cIpmiSdrs *sdrs );

  // called after reading an SDR to create sensors, controls, frus, sel
  virtual bool CreateRdrs( cIpmiDomain *domain, cIpmiMc *mc, cIpmiSdrs *sdrs );

  // create resources from sdr
  virtual bool CreateResources( cIpmiDomain *domain, cIpmiMc *source_mc, cIpmiSdrs *sdrs );

  // find or create resource
  virtual cIpmiResource *FindOrCreateResource( cIpmiDomain *domain, cIpmiMc *mc, unsigned int fru_id,
					       cIpmiSdr *sdr, cIpmiSdrs *sdrs );

protected:
  // create a new entity
  virtual cIpmiResource *CreateResource( cIpmiDomain *domain, cIpmiMc *mc, unsigned int fru_id,
					 cIpmiSdr *sdr, cIpmiSdrs *sdrs );
  cIpmiEntityPath CreateEntityPath( cIpmiDomain *domain, unsigned int mc_addr,
                                    unsigned int fru_id, SaHpiEntityTypeT type,
                                    SaHpiEntityInstanceT instance, cIpmiSdrs *sdrs );

public:
  // create sensors
  virtual bool CreateSensors( cIpmiDomain *domain, cIpmiMc *source_mc, 
                              cIpmiSdrs *sdrs );

protected:
  virtual GList *GetSensorsFromSdrs( cIpmiDomain *domain, cIpmiMc *source_mc,
                                     cIpmiSdrs *sdrs );
  virtual GList *CreateSensorFromFullSensorRecord( cIpmiDomain *domain, cIpmiMc *source_mc,
                                                   cIpmiSdr *sdr, cIpmiSdrs *sdrs );

  virtual GList *CreateSensorHotswap( cIpmiDomain *domain, cIpmiMc *mc,
                                      cIpmiSdr *sdr, cIpmiSdrs *sdrs );
  virtual GList *CreateSensorThreshold( cIpmiDomain *domain, cIpmiMc *mc,
                                        cIpmiSdr *sdr, cIpmiSdrs *sdrs );
  virtual GList *CreateSensorDiscrete( cIpmiDomain *domain, cIpmiMc *mc,
                                       cIpmiSdr *sdr , cIpmiSdrs *sdrs);
  virtual GList *CreateSensorDefault( cIpmiDomain *domain, cIpmiMc *mc,
                                      cIpmiSdr *sdr, cIpmiSdrs *sdrs );

  void CreateSensorEntityPath( cIpmiDomain *domain, cIpmiSensor *s, 
                               cIpmiMc *source_mc,
                               cIpmiSdr *sdr, cIpmiSdrs *sdrs ); 

  virtual cIpmiMc *FindMcBySdr( cIpmiDomain *domain, cIpmiSdr *sdr );

public:
  // create controls
  virtual bool CreateControls( cIpmiDomain *domain, cIpmiMc *mc,
                               cIpmiSdrs *sdrs );

protected:
  virtual bool CreateControlsAtca( cIpmiDomain *domain, cIpmiMc *mc, cIpmiSdrs *sdrs,
                                   tIpmiAtcaSiteType type );
  virtual bool CreateControlAtcaFan( cIpmiDomain *domain, cIpmiResource *res, cIpmiSdrs *sdrs );

public:
  // create FRUs
  virtual bool CreateFrus( cIpmiDomain *domain, cIpmiMc *mc, cIpmiSdrs *sdrs );

protected:
  virtual bool CreateFru( cIpmiDomain *domain, cIpmiMc *mc, cIpmiSdr *sdr,
                          cIpmiSdrs *sdrs );

public:
  // create SEL
  virtual bool CreateSels( cIpmiDomain *domain, cIpmiMc *source_mc, cIpmiSdrs *sdrs );
};


class cIpmiMcVendorFactory
{
  static cIpmiMcVendorFactory *m_factory;

  cIpmiMcVendorFactory();
  ~cIpmiMcVendorFactory();
public:

  static void InitFactory();
  static void CleanupFactory();
  static cIpmiMcVendorFactory *GetFactory() { return m_factory; }

protected:
  // list of all vendor MCs
  GList *m_mc_vendors;

  // default mc vendor
  cIpmiMcVendor *m_default;

public:
  // register a new cIpmiMcVendor
  bool Register( cIpmiMcVendor *mv );

  // unregister cIpmiMcVendor
  bool Unregister( unsigned int manufacturer_id,
                   unsigned int product_id );

  // find a cIpmiMcVendor to a give manufacturer id and product id
  cIpmiMcVendor *Find( unsigned int manufacturer_id,
                       unsigned int product_id );

  // returns the default if not found
  cIpmiMcVendor *Get( unsigned int manufacturer_id,
                      unsigned int product_id );

  cIpmiMcVendor *Default()
  {
    return m_default;
  }
};


#endif
