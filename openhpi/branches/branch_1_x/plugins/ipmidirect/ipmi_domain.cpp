/*
 *
 * Copyright (c) 2003,3004 by FORCE Computers
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
 */

#include <errno.h>
#include <assert.h>

#include "ipmi.h"
#include "ipmi_utils.h"


cIpmiDomain::cIpmiDomain()
  : m_con( 0 ), m_is_atca( false ), m_main_sdrs( 0 ),
    m_sensors_in_main_sdr( 0 ),
    m_major_version( 0 ), m_minor_version( 0 ), m_sdr_repository_support( false ),
    m_si_mc( 0 ),
    m_initial_discover( 0 ),
    m_mc_poll_interval( dIpmiMcPollInterval ),
    m_sel_rescan_interval( dIpmiSelQueryInterval )
{
  cIpmiMcVendorFactory::InitFactory();

  for( int i = 0; i < 256; i++ )
     {
       m_mc_thread[i]   = 0;
       m_atca_site_property[i].m_property    = 0;
       m_atca_site_property[i].m_max_side_id = 0;
       m_atca_site_property[i].m_mc_type     = 0;
     }

  // scan at least at 0x20 (ShMc entity is 0xf0)
  NewFruInfo( 0x20, 0, (SaHpiEntityTypeT)0xf0, 0,
              eIpmiAtcaSiteTypeDedicatedShMc,
	        dIpmiMcThreadInitialDiscover
	      | dIpmiMcThreadPollDeadMc
	      | dIpmiMcThreadPollAliveMc );

  // default site type properties
  unsigned int prop =   dIpmiMcThreadInitialDiscover
                      | dIpmiMcThreadCreateM0;

  // atca board
  SetAtcaSiteProperty( eIpmiAtcaSiteTypeAtcaBoard, prop, 32 );

  // power
  SetAtcaSiteProperty( eIpmiAtcaSiteTypePowerEntryModule, prop, 8 );

  // shelf fru information
  SetAtcaSiteProperty( eIpmiAtcaSiteTypeShelfFruInformation, prop, 4 );

  // dedicated ShMc
  SetAtcaSiteProperty( eIpmiAtcaSiteTypeDedicatedShMc, prop, 2 );

  // fan tray
  SetAtcaSiteProperty( eIpmiAtcaSiteTypeFanTray, prop, 8 );

  // fan filter tray
  SetAtcaSiteProperty( eIpmiAtcaSiteTypeFanFilterTray, prop, 8 );

  // alarm
  SetAtcaSiteProperty( eIpmiAtcaSiteTypeAtcaModule, prop, 32 );

  // atca module
  SetAtcaSiteProperty( eIpmiAtcaSiteTypePMC, prop, 32 );

  // rear transition module
  SetAtcaSiteProperty( eIpmiAtcaSiteTypeRearTransitionModule, prop, 4 );
}


cIpmiDomain::~cIpmiDomain()
{
  cIpmiMcVendorFactory::CleanupFactory();  
}


bool
cIpmiDomain::Init( cIpmiCon *con )
{
  assert( m_con == 0 );
  m_con = con;

  // create system interface
  cIpmiAddr si( eIpmiAddrTypeSystemInterface);

  m_si_mc = new cIpmiMc( this, si );

  if ( m_si_mc == 0 )
     {
       stdlog << "cannot create system interface !\n";
       return false;
     }

  // create main sdr
  m_main_sdrs = new cIpmiSdrs( m_si_mc, false );
  assert( m_main_sdrs );

  // send get device id to system interface
  cIpmiMsg msg( eIpmiNetfnApp, eIpmiCmdGetDeviceId );
  cIpmiMsg rsp;

  SaErrorT rv = m_si_mc->SendCommand( msg, rsp );

  if ( rv != SA_OK )
     {
       stdlog << "cannot send IPMI get device id to system interface: " << rv << ", " << strerror( rv ) << " !\n";
       return false;
     }

  if (    rsp.m_data[0] != 0
       || rsp.m_data_len < 12 )
     {
       // At least the get device id has to work.
       stdlog << "get device id fails " << rsp.m_data[0] << " !\n";

       return false;
     }

  m_major_version          = rsp.m_data[5] & 0xf;
  m_minor_version          = (rsp.m_data[5] >> 4) & 0xf;
  m_sdr_repository_support = (rsp.m_data[6] & 0x02) == 0x02;

  m_si_mc->SdrRepositorySupport() = m_sdr_repository_support;

  if ( m_major_version < 1 )
     {
       // We only support 1.0 and greater.
       stdlog << "ipmi version " << m_major_version << "." 
              << m_minor_version << " not supported !\n";

       return false;
     }

  // set vendor
  unsigned int mid =    rsp.m_data[7]
                     | (rsp.m_data[8] << 8)
                     | (rsp.m_data[9] << 16);

  unsigned int pid = IpmiGetUint16( rsp.m_data + 10 );

  cIpmiMcVendor *mv = cIpmiMcVendorFactory::GetFactory()->Get( mid, pid );
  m_si_mc->SetVendor( mv );

  // initialize system interface MC
  if ( mv->InitMc( m_si_mc, rsp ) == false )
     {
       stdlog << "cannot initialize system interface !\n";

       return false;
     }

  int num = m_max_outstanding;

  if ( m_max_outstanding == 0 )
     {
       // Check the number of outstanding requests.
       // This is fine tuning if the BMC/ShMc has
       // a block transfer interface.
       // If not supported use the defaults
       // given in openhpi.conf.
       msg.m_netfn = eIpmiNetfnApp;
       msg.m_cmd   = eIpmiCmdGetBtInterfaceCapabilities;
       msg.m_data_len = 0;

       rv = m_si_mc->SendCommand( msg, rsp, 0, 1 );

       // ignore on error
       if ( rv == SA_OK && rsp.m_data[0] == 0 && rsp.m_data_len >= 6 )
          {
            num = rsp.m_data[1];

            stdlog << "reading bt capabilities: max outstanding " << num 
                   << ", input " << (int)rsp.m_data[2]
                   << ", output " << (int)rsp.m_data[3]
                   << ", retries " << (int)rsp.m_data[5] << ".\n";

            // check
            if ( num < 1 )
                 num = 1;

            if ( num > 32 )
                 num = 32;
          }
     }

  if ( num == 0 )
     {
       num = 1;
     }
                                                                                                                    
  stdlog << "max number of outstanding = " << num << ".\n";
  m_con->SetMaxOutstanding( num );

  // check for ATCA an modify m_mc_to_check
  CheckAtca();

  if ( m_sdr_repository_support )
     {
       stdlog << "reading repository SDR.\n";

       rv = m_main_sdrs->Fetch();

       if ( rv )
            // Just report an error, it shouldn't be a big deal if this
            // fails.
            stdlog << "could not get main SDRs, error " << rv << " !\n";
       else if ( !m_is_atca )
          {
            // for each mc device locator record in main repository
            // create an entry in m_mc_to_check.
            for( unsigned int i = 0; i < m_main_sdrs->NumSdrs(); i++ )
               {
                 cIpmiSdr *sdr = m_main_sdrs->Sdr( i );

                 if ( sdr->m_type != eSdrTypeMcDeviceLocatorRecord )
                      continue;

                 unsigned char addr = sdr->m_data[5];

                 cIpmiFruInfo *fi = FindFruInfo( addr, 0 );

                 if ( fi == 0 )
                      NewFruInfo( addr, 0,
                                  SAHPI_ENT_OTHER, GetFreeSlotForOther( addr ),
                                  eIpmiAtcaSiteTypeUnknown,
                                    dIpmiMcThreadInitialDiscover
                                  | dIpmiMcThreadPollDeadMc
                                  | dIpmiMcThreadPollAliveMc );
               }
          }
     }

  rv = GetChannels();

  if ( rv )
       return false;

  // Start all MC threads with the
  // properties found in m_mc_to_check.
  m_initial_discover = 0;
  m_num_mc_threads   = 0;

  for( GList *list = GetFruInfoList(); list; list = g_list_next( list ) )
     {
       cIpmiFruInfo *fi = (cIpmiFruInfo *)list->data;

       if ( fi->FruId() != 0 )
            continue;

       int addr = fi->Address();

       assert( m_mc_thread[addr] == 0 );

       m_mc_thread[addr] = new cIpmiMcThread( this, addr, fi->Properties()
                                           /*, m_mc_to_check[i],
                                             m_mc_type[i], m_mc_slot[i] */ );

       // Count MC thread with initial discover.
       // This counter is used in cIpmi::IfDiscoverResources
       // to wait until discover is done
       if ( fi->Properties() & dIpmiMcThreadInitialDiscover )
          {
            m_initial_discover_lock.Lock();
            m_initial_discover++;
            m_initial_discover_lock.Unlock();
          }

       m_mc_thread[addr]->Start();
     }

  return true;
}


void
cIpmiDomain::Cleanup()
{
  int i;

  // stop MC threads
  for( i = 0; i < 256; i++ )
       if ( m_mc_thread[i] )
            m_mc_thread[i]->m_exit = true;

  // wait until all threads are finish
  bool loop = true;

  while( loop )
     {
       m_mc_thread_lock.Lock();
       loop = (bool)m_num_mc_threads;
       m_mc_thread_lock.Unlock();

       usleep( 100000 );
     }

  // wait for threads exit
  for( i = 0; i < 256; i++ )
       if ( m_mc_thread[i] )
          {
            void *rv;

            m_mc_thread[i]->Wait( rv );

            delete m_mc_thread[i];
            m_mc_thread[i] = 0;
          }

  // stop reader thread
  if ( m_con && m_con->IsOpen() )
       m_con->Close();

  // Delete the sensors from the main SDR repository.
  while( m_sensors_in_main_sdr )
     {
       cIpmiSensor *sensor = (cIpmiSensor *)m_sensors_in_main_sdr->data;
       m_sensors_in_main_sdr = g_list_remove( m_sensors_in_main_sdr, sensor );
       sensor->Resource()->RemRdr( sensor );
       delete sensor;
     }

  // cleanup all MCs
  for( i = m_mcs.Num() - 1; i >= 0; i-- )
     {
       cIpmiMc *mc = m_mcs[i];
       CleanupMc( mc );
     }

  // now all mc's are ready to destroy
  while( m_mcs.Num() )
     {
       cIpmiMc *mc = m_mcs[0];

       if ( CleanupMc( mc ) == false )
            assert( 0 );
     }

  // destroy si
  if ( m_si_mc )
     {
       bool rr = m_si_mc->Cleanup();
       assert( rr );
       delete m_si_mc;
       m_si_mc = 0;
     }

  /* Destroy the main SDR repository, if it exists. */
  if ( m_main_sdrs )
     {
       delete m_main_sdrs;
       m_main_sdrs = 0;
     }
}


void
cIpmiDomain::AddMc( cIpmiMc *mc  )
{
  m_mcs.Add( mc );
}


bool
cIpmiDomain::CleanupMc( cIpmiMc *mc )
{
  if ( !mc->Cleanup() )
       return false;

  int idx = m_mcs.Find( mc );
  
  if ( idx == -1 )
     {
       stdlog << "unable to find mc at " << (unsigned char)mc->GetAddress() << " in mc list !\n";
       return false;
     }
  
  m_mcs.Rem( idx );
  delete mc;

  return true;
}


int 
cIpmiDomain::GetChannels()
{
  int rv = 0;

  return rv;
}


SaErrorT
cIpmiDomain::CheckAtca()
{
  cIpmiMsg msg( eIpmiNetfnPicmg, eIpmiCmdGetPicMgProperties );
  msg.m_data_len = 1;
  msg.m_data[0]  = dIpmiPigMgId;

  cIpmiMsg rsp;
  SaErrorT rv;
  int i;

  m_is_atca = false;

  assert( m_si_mc );

  stdlog << "checking for ATCA system.\n";

  rv = m_si_mc->SendCommand( msg, rsp );

  if ( rv != SA_OK || rsp.m_data[0] || rsp.m_data[1] != dIpmiPigMgId )
     {
       stdlog << "not an ATCA system.\n";

       return (rv != SA_OK) ? rv : SA_ERR_HPI_DATA_LEN_INVALID;
     }

  unsigned char minor = (rsp.m_data[2] >> 4) & 0x0f;
  unsigned char major = rsp.m_data[2] & 0x0f;

  stdlog << "found a PigMg system version " << major << "." << minor << ".\n";

  if ( major != 2 || ( ( minor != 0 ) && ( minor != 1 ) ) )
       return SA_OK;

  stdlog << "found an ATCA system.\n";

  // use atca timeout
  stdlog << "set timeout to " << m_con_atca_timeout << ".\n";
  m_con->m_timeout = m_con_atca_timeout;

  m_is_atca = true;

  // read all fru addr
  msg.m_netfn   = eIpmiNetfnPicmg;
  msg.m_cmd     = eIpmiCmdGetAddressInfo;
  msg.m_data[0] = dIpmiPigMgId;
  msg.m_data[1] = 0; // fru id 0
  msg.m_data[2] = 0x03; // physical addr
  msg.m_data_len = 5;

  static const char *map[] =
  {
    "ATCA Board",
    "Power Entry Module",
    "Shelf FRU Information",
    "Dedicated ShMc",
    "Fan Tray",
    "Fan Filter Tray",
    "Alarm",
    "ATCA Module",
    "PMC",
    "Rear Transition Module"
  };

  static int map_num = sizeof( map ) / sizeof( char * );

  for( i = 0; i < 256; i++ )
     {
       if ( !m_atca_site_property[i].m_property )
            continue;

       if ( m_atca_poll_alive_mcs == true )
            m_atca_site_property[i].m_property |= dIpmiMcThreadPollAliveMc;

       if ( i < map_num )
            stdlog << "checking for " << map[i] << ".\n";
       else
            stdlog << "checking for " << (unsigned char)i << ".\n";

       SaHpiEntityTypeT entity = MapAtcaSiteTypeToEntity( (tIpmiAtcaSiteType)i );

       for( int j = 0; j < m_atca_site_property[i].m_max_side_id; j++ )
          {
            msg.m_data[3] = j + 1;
            msg.m_data[4] = i;

            rv = m_si_mc->SendCommand( msg, rsp );

            if ( rv != SA_OK )
               {
                 stdlog << "cannot send get address info: " << rv << " !\n";
                 break;
               }

            if ( rsp.m_data[0] )
                 break;

            if ( i < map_num )
                 stdlog << "\tfound " << map[i] << " at " << rsp.m_data[3] << ".\n";
            else
                 stdlog << "\tfound " << (unsigned char)i << " at " <<  rsp.m_data[3] << ".\n";

            // add slot for initial scan
            NewFruInfo( rsp.m_data[3], rsp.m_data[5], entity, j + 1, 
                        (tIpmiAtcaSiteType)i, m_atca_site_property[i].m_property );
          }
     }

  return SA_OK;
}


cIpmiMc *
cIpmiDomain::FindMcByAddr( const cIpmiAddr &addr )
{
  if (    ( addr.m_type == eIpmiAddrTypeSystemInterface )
       && ( addr.m_channel == dIpmiBmcChannel ) )
       return m_si_mc;

  for( int i = 0; i < m_mcs.Num(); i++ )
     {
       cIpmiMc *mc = m_mcs[i];

       if ( addr == mc->Addr() )
            return mc;
     }

  return 0;
}


SaErrorT
cIpmiDomain::SendCommand( const cIpmiAddr &addr, const cIpmiMsg &msg,
                          cIpmiMsg &rsp_msg, int retries )
{
  if ( m_con == 0 )
     {
       assert( 0 );
       return SA_ERR_HPI_NOT_PRESENT;
     }

  return m_con->ExecuteCmd( addr, msg, rsp_msg, retries );
}


/*
cIpmiMc *
cIpmiDomain::FindOrCreateMcBySlaveAddr( unsigned int slave_addr )
{
  cIpmiAddr addr( eIpmiAddrTypeIpmb, 0, 0, slave_addr );

  cIpmiMc *mc = FindMcByAddr( addr );

  if ( mc )
       return mc;

  mc = NewMc( addr );
  mc->SetActive( false );

  return mc;
}
*/


GList *
cIpmiDomain::GetSdrSensors( cIpmiMc *mc )
{
  if ( mc )
       return mc->GetSdrSensors();

  return m_sensors_in_main_sdr;
}


void
cIpmiDomain::SetSdrSensors( cIpmiMc *mc,
                            GList   *sensors )
{
  if ( mc )
       mc->SetSdrSensors( sensors );
  else
       m_sensors_in_main_sdr = sensors;
}


cIpmiMc *
cIpmiDomain::GetEventRcvr()
{
  for( int i = 0; i < m_mcs.Num(); i++ )
     {
       cIpmiMc *mc = m_mcs[i];

       if ( mc->SelDeviceSupport() )
            return mc;
     }

  return 0;
}


void
cIpmiDomain::HandleEvents( GList *list )
{
  while( list )
     {
       cIpmiEvent *event = (cIpmiEvent *)list->data;
       list = g_list_remove( list, event );

       HandleEvent( event );
     }
}


void
cIpmiDomain::HandleAsyncEvent( const cIpmiAddr &addr, const cIpmiMsg &msg )
{
  // It came from an MC, so find the MC.
  cIpmiMc *mc = FindMcByAddr( addr );

  if ( !mc )
     {
       stdlog << "cannot find mc for event !\n";
       return;
     }

  cIpmiEvent *event = new cIpmiEvent;

  event->m_mc        = mc;
  event->m_record_id = IpmiGetUint16( msg.m_data );
  event->m_type      = msg.m_data[2];
  memcpy( event->m_data, msg.m_data + 3, dIpmiMaxSelData );

  // Add it to the MCs event log.
  mc->Sel()->AddAsyncEvent( event );

  HandleEvent( event );
}


void
cIpmiDomain::HandleEvent( cIpmiEvent *event )
{
  // find MC thread
  unsigned char a = event->m_data[4];

  bool hotswap = ( event->m_data[7] == eIpmiSensorTypeAtcaHotSwap ) ? true : false;

  // if there is no MC thread => create MC thread
  if ( m_mc_thread[a] == 0 )
     {
       int slot = GetFreeSlotForOther( a );

       cIpmiFruInfo *fi = NewFruInfo( a, 0, SAHPI_ENT_OTHER, slot,
                                      eIpmiAtcaSiteTypeUnknown, 
                                        dIpmiMcThreadInitialDiscover
                                      | hotswap ? (   dIpmiMcThreadPollAliveMc
                                                    | dIpmiMcThreadCreateM0) : 0 );

       m_mc_thread[a] = new cIpmiMcThread( this, a, fi->Properties() );
       m_mc_thread[a]->Start();
     }

  m_mc_thread[a]->AddEvent( event );
}


cIpmiResource *
cIpmiDomain::VerifyResource( cIpmiResource *res )
{
  for( int i = 0; i < m_mcs.Num(); i++ )
     {
       cIpmiMc *mc = m_mcs[i];

       if ( mc->FindResource( res ) )
	    return res;
     }

  return 0;
}


cIpmiMc *
cIpmiDomain::VerifyMc( cIpmiMc *mc )
{
  if ( m_si_mc == mc )
       return mc;

  int idx = m_mcs.Find( mc );
  
  if ( idx == -1 )
       return 0;

  return mc;
}


cIpmiRdr *
cIpmiDomain::VerifyRdr( cIpmiRdr *rdr )
{
  for( int i = 0; i < m_mcs.Num(); i++ )
     {
       cIpmiMc *mc = m_mcs[i];

       if ( mc->FindRdr( rdr ) )
            return rdr;
     }

  return 0;
}


cIpmiSensor *
cIpmiDomain::VerifySensor( cIpmiSensor *s )
{
  for( int i = 0; i < m_mcs.Num(); i++ )
     {
       cIpmiMc *mc = m_mcs[i];

       if ( mc->FindRdr( s ) )
	    return s;
     }

  return 0;
}


cIpmiControl *
cIpmiDomain::VerifyControl( cIpmiControl *c )
{
  for( int i = 0; i < m_mcs.Num(); i++ )
     {
       cIpmiMc *mc = m_mcs[i];

       if ( mc->FindRdr( c ) )
            return c;
     }

  return 0;
}


cIpmiInventory *
cIpmiDomain::VerifyInventory( cIpmiInventory *inv )
{
  for( int i = 0; i < m_mcs.Num(); i++ )
     {
       cIpmiMc *mc = m_mcs[i];

       if ( mc->FindRdr( inv ) )
            return inv;
     }

  return 0;
}


void 
cIpmiDomain::Dump( cIpmiLog &dump ) const
{
  if ( dump.IsRecursive() )
     {
       dump << "#include \"Mc.sim\"\n";
       dump << "#include \"Entity.sim\"\n";
       dump << "#include \"Sensor.sim\"\n";
       dump << "#include \"Sdr.sim\"\n";
       dump << "#include \"Sel.sim\"\n";
       dump << "#include \"Fru.sim\"\n\n\n";

       // main sdr
       if ( m_main_sdrs )
	  {
	    dump << "// repository SDR\n";
	    m_main_sdrs->Dump( dump, "MainSdr1" );
	  }

       // dump all MCs
       for( int i = 0; i < 256; i++ )
	  {
	    if ( m_mc_thread[i] == 0 || m_mc_thread[i]->Mc() == 0 )
		 continue;

	    cIpmiMc *mc = m_mc_thread[i]->Mc();
	    char str[80];
	    sprintf( str, "Mc%02x", i );
	    mc->Dump( dump, str );
	  }
     }

  // sim
  dump.Begin( "Sim", "Dump" );

  for( GList *list = GetFruInfoList(); list; list = g_list_next( list ) )
     {
       cIpmiFruInfo *fi = (cIpmiFruInfo *)list->data;
       const char *site = 0;

       switch( fi->Site() )
          {
            case eIpmiAtcaSiteTypeAtcaBoard:
                 site = "AtcaBoard";
                 break;

            case eIpmiAtcaSiteTypePowerEntryModule:
                 site = "PowerUnit";
                 break;

            case eIpmiAtcaSiteTypeShelfFruInformation:
                 site = "ShelfFruInformation";
                 break;

            case eIpmiAtcaSiteTypeDedicatedShMc:
                 site = "Fan";
                 break;

            case eIpmiAtcaSiteTypeFanTray:
                 site = "FanTray";
                 break;

            case eIpmiAtcaSiteTypeFanFilterTray:
                 site = "FanFilterTray";
                 break;

            case eIpmiAtcaSiteTypeAlarm:
                 site = "Alarm";
                 break;

            case eIpmiAtcaSiteTypeAtcaModule:
                 site = "AtcaModule";
                 break;

            case eIpmiAtcaSiteTypePMC:
                 site = "PMC";
                 break;

            case eIpmiAtcaSiteTypeRearTransitionModule:
                 site = "RearTransitionModule";
                 break;

            default:
                 assert( 0 );
                 site = "Unknown";
                 break;
          }

       dump.Entry( site ) << fi->Slot() << ", " << (unsigned char)fi->Address() << ";\n";
     }

  if ( dump.IsRecursive() )
     {
       dump << "\n";

       if ( m_main_sdrs )
	    dump.Entry( "MainSdr" ) << "MainSdr1\n";

       for( int i = 0; i < 256; i++ )
	  {
	    if (    m_mc_thread[i] == 0 
		    || m_mc_thread[i]->Mc() == 0 )
		 continue;

            cIpmiFruInfo *fi = FindFruInfo( i, 0 );

            if ( fi == 0 )
               {
                 assert( 0 );
                 continue;
               }

            const char *type = 0;

            if ( fi->Site() == eIpmiAtcaSiteTypeAtcaBoard )
                 type = "AtcaBoard";
            else if ( fi->Site() == eIpmiAtcaSiteTypePowerEntryModule )
                 type = "PowerUnit";
            else if ( fi->Site() == eIpmiAtcaSiteTypeFanTray )
                 type = "FanTray";
            else
               {
                 continue;
                 assert( 0 );
               }

	    char str[30];
	    sprintf( str, "Mc%02x", i );
            dump.Entry( "Mc" ) << str << ", " << type << ", " << fi->Slot() << ";\n";
	  }
     }

  dump.End();
}


bool
cIpmiDomain::Populate()
{
  for( int i = 0; i < m_mcs.Num(); i++ )
       if ( m_mcs[i]->Populate() == false )
            return false;

  return true;
}
