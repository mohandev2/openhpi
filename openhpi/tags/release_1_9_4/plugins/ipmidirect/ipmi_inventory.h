/*
 *
 * Copyright (c) 2004 by FORCE Computers.
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

#ifndef dIpmiInventory_h
#define dIpmiInventory_h


#ifndef dIpmiRdr_h
#include "ipmi_rdr.h"
#endif


#ifndef dIpmiInventoryParser_h
#include "ipmi_inventory_parser.h"
#endif


enum tInventoryAccessMode
{
  eInventoryAccessModeByte = 0,
  eInventoryAccessModeWord = 1
};


#define dMaxFruFetchBytes 20


class cIpmiInventory : public cIpmiRdr, public cIpmiInventoryParser
{
protected:
  unsigned char  m_fru_device_id; // fru device id
  tInventoryAccessMode m_access;
  unsigned int   m_size;
  bool           m_fetched;

  unsigned int   m_oem;

  SaErrorT GetFruInventoryAreaInfo( unsigned int &size, tInventoryAccessMode &byte_access );
  SaErrorT ReadFruData( unsigned short offset, unsigned int num, unsigned int &n, unsigned char *data );

public:
  cIpmiInventory( cIpmiMc *mc, unsigned int fru_device_id );
  ~cIpmiInventory();

  SaErrorT Fetch();

  virtual unsigned int Num() const { return m_fru_device_id; }
  unsigned int &Oem() { return m_oem; }

  // create an RDR inventory record
  virtual bool CreateRdr( SaHpiRptEntryT &resource, SaHpiRdrT &rdr );

public:
  virtual void Dump( cIpmiLog &dump, const char *name ) const;
};


#endif
