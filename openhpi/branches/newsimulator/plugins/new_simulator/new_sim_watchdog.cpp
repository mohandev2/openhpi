/** 
 * @file    new_sim_watchdog.cpp
 *
 * The file includes a class for watchdog handling:\n
 * NewSimulatorWatchdog
 * 
 * @todo Generating of events and maybe some watchdog actions
 * 
 * @author  Lars Wetzel <larswetzel@users.sourceforge.net>
 * @version 0.1
 * @date    2010
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 * 
 */
 
#include "new_sim_watchdog.h"
#include "new_sim_utils.h"
#include "new_sim_domain.h"

/**
 * Constructor
 **/
NewSimulatorWatchdog::NewSimulatorWatchdog( NewSimulatorResource *res )
                    : NewSimulatorRdr( res, SAHPI_WATCHDOG_RDR ),
                      m_state( NONE ) {

   memset( &m_wdt_rec, 0, sizeof( SaHpiWatchdogRecT ));
   memset( &m_wdt_data, 0, sizeof( SaHpiWatchdogT ));
   
}


/**
 * Full qualified constructor to fill an object with the parsed data
 **/
NewSimulatorWatchdog::NewSimulatorWatchdog( NewSimulatorResource *res, 
                      SaHpiRdrT rdr, 
                      SaHpiWatchdogT wdt_data)
                    : NewSimulatorRdr( res, SAHPI_WATCHDOG_RDR, rdr.Entity, rdr.IsFru, rdr.IdString ),
                      m_state( NONE ) {

   memcpy( &m_wdt_rec, &rdr.RdrTypeUnion.WatchdogRec, sizeof( SaHpiWatchdogRecT ));
   memcpy( &m_wdt_data, &wdt_data, sizeof( SaHpiWatchdogT ));
}


/**
 * Destructor
 **/
NewSimulatorWatchdog::~NewSimulatorWatchdog() {}


/**
 * A rdr structure is filled with the data
 * 
 * This method is called by method NewSimulatorRdr::Populate().
 * 
 * @param resource Address of resource structure
 * @param rdr Address of rdr structure
 * 
 * @return true
 **/
bool NewSimulatorWatchdog::CreateRdr( SaHpiRptEntryT &resource, SaHpiRdrT &rdr ) {

   if ( NewSimulatorRdr::CreateRdr( resource, rdr ) == false )
      return false;

   /// Watchdog record
   memcpy(&rdr.RdrTypeUnion.WatchdogRec, &m_wdt_rec, sizeof(SaHpiWatchdogRecT));
   

   return true;
}


/** 
 * Dump the Watchdog information
 * 
 * @param dump address of the log
 * 
 **/
void NewSimulatorWatchdog::Dump( NewSimulatorLog &dump ) const {
   char str[256];
   IdString().GetAscii( str, 256 );

   dump << "Watchdog: " << m_wdt_rec.WatchdogNum << " " << str << "\n";
   dump << "Oem:      " << m_wdt_rec.Oem << "\n";
   dump << "Watchdog data:\n";
   dump << "Log:                " << m_wdt_data.Log << "\n";
   dump << "Running:            " << m_wdt_data.Running << "\n";
   dump << "TimerUse:           " << m_wdt_data.TimerUse << "\n";
   dump << "TimerAction:        " << m_wdt_data.TimerAction << "\n";
   dump << "PretimerInterrupt:  " << m_wdt_data.PretimerInterrupt << "\n";
   dump << "PreTimeoutInterval: " << m_wdt_data.PreTimeoutInterval << "\n";
   dump << "TimerUseExpFlags:   " << m_wdt_data.TimerUseExpFlags << "\n";
   dump << "InitialCount:       " << m_wdt_data.InitialCount << "\n";
   dump << "PresentCount:       " << m_wdt_data.PresentCount << "\n";
   
}


/**
 * Check whether the watchdog timer is running or not. 
 * 
 * Trigger proper action if necessary.
 * 
 * @return true if it is running, false if not
 **/
bool NewSimulatorWatchdog::CheckWatchdogTimer() {
	
   if ( m_wdt_data.Running == SAHPI_FALSE )
      return false;
      
   if ( ! m_start.IsSet() )
      return false;
   
   // Ok, we have a running wdt
   cTime now( cTime::Now() );
   now -= m_start;
   
   if ( now.GetMsec() >= m_wdt_data.InitialCount ) {
   	
      TriggerAction( TIMEOUT );
      m_wdt_data.Running = SAHPI_FALSE;
      m_wdt_data.PresentCount = 0;
      m_start.Clear();
      
      stdlog << "DBG: WatchdogTimer expires.\n";
      
      return false;
   }
   
   if ( now.GetMsec() >= m_wdt_data.InitialCount - m_wdt_data.PreTimeoutInterval ) {
   	
   	  TriggerAction( PRETIMEOUT );
   	  m_wdt_data.PresentCount = m_wdt_data.InitialCount - now.GetMsec();
   	  return true;
   }
   
   m_wdt_data.PresentCount = m_wdt_data.InitialCount - now.GetMsec();

   return true;
}



/**
 * Trigger an action, like sending an event and setting the exp_mask
 * 
 * @param state watchdog state which should be triggered 
 **/
void NewSimulatorWatchdog::TriggerAction( WdtStateT state ) {
   SaHpiWatchdogActionEventT wdtaction;
   SaHpiSeverityT sev = SAHPI_MAJOR;
   
   if ( ( state == PRETIMEOUT ) &&
         ( m_state != PRETIMEOUT ) ) {
     
      m_state = PRETIMEOUT;
      wdtaction = SAHPI_WAE_TIMER_INT;
      sev = SAHPI_MAJOR;
      if ( m_wdt_data.Log == SAHPI_TRUE )
         SendEvent( wdtaction, sev );
   }
  
   if ( state == TIMEOUT ) {
      m_state = TIMEOUT;
      m_wdt_data.Running = SAHPI_FALSE;
      m_wdt_data.PresentCount = 0;
      m_start.Clear();
      
      switch ( m_wdt_data.TimerAction ) {
         case SAHPI_WA_NO_ACTION:
            sev = SAHPI_INFORMATIONAL;
            wdtaction = SAHPI_WAE_NO_ACTION;
            break;
            
         case SAHPI_WA_RESET:
            sev = SAHPI_MAJOR;
            wdtaction = SAHPI_WAE_RESET;
            break;
            
         case SAHPI_WA_POWER_DOWN:
            sev = SAHPI_MAJOR;
            wdtaction = SAHPI_WAE_POWER_DOWN;
            break;
            
         case SAHPI_WA_POWER_CYCLE:
            sev = SAHPI_MAJOR;
            wdtaction = SAHPI_WAE_POWER_CYCLE;
            break;
            
         default:
            err("Invalid TimerAction is configured inside Watchdog");
            sev = SAHPI_INFORMATIONAL;
            wdtaction = SAHPI_WAE_NO_ACTION;
            break;
      }
      
      switch ( m_wdt_data.TimerUse ) {
      	  case SAHPI_WTU_NONE:
      	  case SAHPI_WTU_UNSPECIFIED:
      	     break;
      	     
          case SAHPI_WTU_BIOS_FRB2:
             m_wdt_data.TimerUseExpFlags |= SAHPI_WATCHDOG_EXP_BIOS_FRB2;
             break;
             
          case SAHPI_WTU_BIOS_POST:
             m_wdt_data.TimerUseExpFlags |= SAHPI_WATCHDOG_EXP_BIOS_POST;
             break;
             
          case SAHPI_WTU_OS_LOAD:
             m_wdt_data.TimerUseExpFlags |= SAHPI_WATCHDOG_EXP_OS_LOAD;
             break;
             
          case SAHPI_WTU_SMS_OS:
             m_wdt_data.TimerUseExpFlags |= SAHPI_WATCHDOG_EXP_SMS_OS;
             break;
             
          case SAHPI_WTU_OEM:
             m_wdt_data.TimerUseExpFlags |= SAHPI_WATCHDOG_EXP_OEM;
             break;
             
          default:
             err("Invalid TimerUse is configured inside Watchdog");
             break;
      }
      
      if ( m_wdt_data.Log == SAHPI_TRUE )
         SendEvent( wdtaction, sev ); 
   }
}


/**
 * Send a watchdog event
 * 
 * @param wdtaction watchdog action event flag to be set
 * @param sev severity of event to be set 
 **/
void NewSimulatorWatchdog::SendEvent( SaHpiWatchdogActionEventT wdtaction, SaHpiSeverityT sev ) {

   NewSimulatorResource *res = Resource();
   if( !res ) {
      stdlog << "DBG: Watchdog::TriggerAction: No resource !\n";
      return;
   }
  
   oh_event *e = (oh_event *)g_malloc0( sizeof( struct oh_event ) );

   e->event.EventType = SAHPI_ET_WATCHDOG;

   SaHpiRptEntryT *rptentry = oh_get_resource_by_id( res->Domain()->GetHandler()->rptcache, res->ResourceId() );
   SaHpiRdrT *rdrentry = oh_get_rdr_by_id( res->Domain()->GetHandler()->rptcache, res->ResourceId(), m_record_id );

   if ( rptentry )
      e->resource = *rptentry;
   else
      e->resource.ResourceCapabilities = 0;

   if ( rdrentry )
      e->rdrs = g_slist_append(e->rdrs, g_memdup(rdrentry, sizeof(SaHpiRdrT)));
   else
      e->rdrs = NULL;

   // hpi events
   e->event.Source    = res->ResourceId();
   e->event.EventType = SAHPI_ET_WATCHDOG;
   e->event.Severity  = sev;
  
   oh_gettimeofday(&e->event.Timestamp);
  
   SaHpiWatchdogEventT *wdte = &e->event.EventDataUnion.WatchdogEvent;
   wdte->WatchdogNum            = m_wdt_rec.WatchdogNum;
   wdte->WatchdogAction         = wdtaction;
   wdte->WatchdogPreTimerAction = m_wdt_data.PretimerInterrupt;
   wdte->WatchdogUse            = m_wdt_data.TimerUse;
  
   stdlog << "DBG: NewSimWatchdog::SendEvent Wdt for resource " << res->ResourceId() << "\n";
   res->Domain()->AddHpiEvent( e );

}
 
// Official HPI functions
/** 
 * HPI function saHpiWatchdogTimerGet
 * 
 * See also the description of the function inside the specification or header file.
 * Copying the internal values and show the remaining time if the timer was started.
 * 
 * @param watchdog address of watchdog record to be filled
 * 
 * @return HPI return code
 **/
SaErrorT NewSimulatorWatchdog::GetWatchdogInfo( SaHpiWatchdogT &watchdog ) {
   
   if ( &watchdog == NULL)
      return SA_ERR_HPI_INVALID_PARAMS;
   
   memcpy( &watchdog, &m_wdt_data, sizeof( SaHpiWatchdogT ));   

   if ( m_start.IsSet() ) {
      cTime now( cTime::Now() );
      now -= m_start;
      
      if ( m_wdt_data.InitialCount - now.GetMsec() < 0 ) {
         watchdog.PresentCount = 0;
      } else {
      	 watchdog.PresentCount = m_wdt_data.InitialCount - now.GetMsec();
      }
      
      stdlog << "DBG: GetWatchdogInfo PresentCount == " << watchdog.PresentCount << "\n";
   }
   
   stdlog << "DBG: Call of GetWatchdogInfo: num " << m_wdt_rec.WatchdogNum << "\n";

   return SA_OK;
}


/** 
 * HPI function saHpiWatchdogTimerSet
 * 
 * See also the description of the function inside the specification or header file.
 * Copying the internal reading values (if a read is allowed).
 * 
 * @param watchdog address of watchdog record to be filled
 * 
 * @return HPI return code
 **/
SaErrorT NewSimulatorWatchdog::SetWatchdogInfo( SaHpiWatchdogT &watchdog ) {
   SaHpiWatchdogExpFlagsT origFlags; 
   
   if ( &watchdog == NULL )
      return SA_ERR_HPI_INVALID_PARAMS;

   if ( watchdog.PreTimeoutInterval > watchdog.InitialCount )
      return SA_ERR_HPI_INVALID_DATA;

   origFlags = m_wdt_data.TimerUseExpFlags;
   
   memcpy( &m_wdt_data, &watchdog, sizeof( SaHpiWatchdogT ));  

   if ( watchdog.Running == SAHPI_TRUE ) {
      if ( m_start.IsSet() ) {
         m_start = cTime::Now();  

      } else {
      	 m_start.Clear();
      	 m_wdt_data.Running = SAHPI_FALSE;
      	 m_wdt_data.PresentCount = 0;
      }
      
   } else {
      m_start.Clear();
      m_wdt_data.PresentCount = 0;
   }
   
   // ClearFlags
   m_wdt_data.TimerUseExpFlags = origFlags & ~watchdog.TimerUseExpFlags;

   stdlog << "DBG: SetWatchdogInfo successfully: num " << m_wdt_rec.WatchdogNum << "\n";

   return SA_OK;
}


/** 
 * HPI function saHpiWatchdogTimerReset
 * 
 * See also the description of the function inside the specification or header file.
 * Starting or resetting a watchdog timer if it is allowed
 * 
 * @return HPI return code
 **/
SaErrorT NewSimulatorWatchdog::ResetWatchdog() {

   if ( m_start.IsSet() ) {
      cTime now( cTime::Now() );
      now -= m_start;
      
      if ( now.GetMsec() > m_wdt_data.InitialCount - m_wdt_data.PreTimeoutInterval ) {
         
         stdlog << "DBG: ResetWatchdog not allowed: num " << m_wdt_rec.WatchdogNum << ":\n";
         stdlog << "DBG: Time expire in ms: " << now.GetMsec() << " > " 
                << (m_wdt_data.InitialCount - m_wdt_data.PreTimeoutInterval) << "\n";
         return SA_ERR_HPI_INVALID_REQUEST;
      }
   } else {
   	
      m_start = cTime::Now();  
   }

   m_wdt_data.Running = SAHPI_TRUE;
   Domain()->SetRunningWdt( true );
   stdlog << "DBG: ResetWatchdog successfully: num " << m_wdt_rec.WatchdogNum << "\n";
   
   return SA_OK;
}
