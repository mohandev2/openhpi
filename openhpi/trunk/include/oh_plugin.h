#ifndef __OH_PLUGIN_H
#define __OH_PLUGIN_H

#include <uuid/uuid.h>

#include <sys/time.h>
#include <SaHpi.h>

enum oh_id_type {
	OH_ID_RESOURCE,
	OH_ID_RDR,
};

/* 
 * struct oh_id is filled by plugin and used by Open HPI.
 * Open HPI never assume any detail in struct oh_id.
 * Plugin can store any data pointer in ptr member so that
 * it can map id back to solid data
 */
struct oh_id {
	enum oh_id_type type;
	void *ptr;
};
	
/*
 * The event is used for plugin to report its resources
 * (Domain, SEL and RDR etc.).
 */
struct oh_resource_event {
	/* XXX: upper layer will fill some fields which does not 
	 * owned by plugins (such as domain Id)
	 */
	SaHpiRptEntryT		entry;
};

/* 
 * The event is used for plugin to report its RDR against resource.
 */
struct oh_rdr_event {
	struct oh_id	parent; /*This is resource oid the RDR relate*/
	void *          private;
	SaHpiRdrT	rdr;
};

/* 
 * This is the main event structure. It is used for plugin report
 * its discovery about new resource/rdr or what happend on resource
 */
struct oh_event {
	enum {
		OH_ET_RESOURCE,
		OH_ET_RDR,
		OH_ET_HPI
	}type;
	struct oh_id			oid;
	union {
		struct oh_resource_event res_event;
		struct oh_rdr_event	 rdr_event;
		/* XXX: upper layer will fill some fields which does not
		 * owned by plugins (ResourceId etc.). */
		SaHpiEventT		 hpi_event;
	} u;		    
};

/* UUID is ee778a5f-32cf-453b-a650-518814dc956c */
static const uuid_t UUID_OH_ABI_V1 = {
	0xee, 0x77, 0x8a, 0x5f, 0x32, 0xcf, 0x45, 0x3b,
	0xa6, 0x50, 0x51, 0x88, 0x14, 0xdc, 0x95, 0x6c
};

struct oh_abi_v1 {
	/**
	 * The function create an instance 
	 * @return the handler of the instance, this can be recognised 
	 * as a domain in upper layer
	 * @param name the mechanism's name. 
	 * for example, "snmp" for SNMP, "smi" for IPMI SMI
	 * @param addr the interface name.
	 * for example, "ipaddr:port" for SNMP, "if_num" for IPMI SMI
	 */
	void *(*open)(const char *name, const char *addr);
	
	/**
	 * open the domain which is on the corresponding resource oid.
	 * Note, the id must be resource id.
	 */
	void *(*open_domain)(void *hnd, struct oh_id *id);
	
	void (*close)(void *hnd);
	/**
	 * The function wait for event. 
	 * 
	 *
	 * @remark at the start-up, plugins must send out res/rdr event for all
	 * resources and rdrs so as to OpenHPI can build up RPT/RDR.
	 * @return >0 if an event is returned; 0 if timeout; otherwise an error
	 * occur.
	 * @param event if existing, plugin store the event. 
	 * @param timeout is an upper bound on the amount of time elapsed
	 * before returns. It may be zero, causing select to return 
	 * immediately.
	 */
	int (*get_event)(void *hnd, struct oh_event *event, 
			 struct timeval *timeout);
	
	/**
	 * get the id which the caller is running
	 */
	int (*get_sel_id)(void *hnd, struct oh_id *id);

	/**
	 * get info from system event log
	 */
	int (*get_sel_info)(void *hnd, struct oh_id *id, SaHpiSelInfoT *info);

	/**
	 * set time to system event log
	 */
	int (*set_sel_time)(void *hnd, struct oh_id *id, struct timeval *time);

	/**
	 * set state to system event log
	 */
	int (*set_sel_state)(void *hnd, struct oh_id *id, int enabled);
	
	/**
	 * get sensor info
	 */
	int (*get_sensor_info)(void *hnd, struct oh_id *id, 
			       void *private, SaHpiSensorDataFormatT *format);

	/**
	 * get sensor data
	 */
	int (*get_sensor_data)(void *hnd, struct oh_id *id, 
			       void *private, SaHpiSensorReadingT *data);

	/**
	 * get sensor thresholds
	 */
	int (*get_sensor_thresholds)(void *hnd, struct oh_id *id,void *private,
				     SaHpiSensorThresholdsT *thres);
	
	/**
	 * set sensor thresholds
	 */
	int (*set_sensor_thresholds)(void *hnd, struct oh_id *id,void *private,
				     const SaHpiSensorThresholdsT *thres);
	
	/**
	 * get control info
	 */
	int (*get_control_info)(void *hnd, struct oh_id *id, void *private,
				SaHpiCtrlTypeT *type);

	/**
	 * get control state
	 */
	int (*get_control_state)(void *hnd, struct oh_id *id, void *private,
				 SaHpiCtrlStateT *state);
	
	/**
	 * set control state
	 */
	int (*set_control_state)(void *hnd, struct oh_id *id, void *private,
				 SaHpiCtrlStateT *state);
	
	/**
	 * set inventory state
	 */
	int (*get_inventory_size)(void *hnd, struct oh_id *id, void *private,
				  size_t *size);

	/**
	 * get inventory state
	 */
	int (*get_inventory_info)(void *hnd, struct oh_id *id, void *private,
				  SaHpiInventoryDataT *data);

	/**
	 * set inventory state
	 */
	int (*set_inventory_info)(void *hnd, struct oh_id *id, void *private,
				  SaHpiInventoryDataT *data);

	/**
	 * get watchdog timer info
	 */
	int (*get_watchdog_info)(void *hnd, struct oh_id *id, void *private,
				 SaHpiWatchdogT *wdt);

	/** 
	 * set watchdog timer info
	 */
	int (*set_watchdog_info)(void *hnd, struct oh_id *id, void *private,
				 SaHpiWatchdogT *wdt);

	/**
	 * reset watchdog timer info
	 */
	int (*reset_watchdog)(void *hnd, struct oh_id *id, void *private);

	/**
	 * get hotswap state
	 */
	int (*get_hotswap_state)(void *hnd, struct oh_id *id, 
				 SaHpiHsStateT *state);

	/**
	 * set hotswap state
	 */
	int (*set_hotswap_state)(void *hnd, struct oh_id *id, 
				 SaHpiHsStateT state);

	/**
	 * request hotswap state
	 */
	int (*request_hotswap_action)(void *hnd, struct oh_id *id, 
				      SaHpiHsActionT act);

	/**
	 * get power state
	 */
	int (*get_power_state)(void *hnd, struct oh_id *id, 
			       SaHpiHsPowerStateT *state);

	/**
	 * set power state
	 */
	int (*set_power_state)(void *hnd, struct oh_id *id, 
			       SaHpiHsPowerStateT state);
	
	/**
	 * get indicator state
	 */
	int (*get_indicator_state)(void *hnd, struct oh_id *id, 
				   SaHpiHsIndicatorStateT *state);

	/**
	 * set indicator state
	 */
	int (*set_indicator_state)(void *hnd, struct oh_id *id, 
				   SaHpiHsIndicatorStateT state);

	/**
	 * control parameter
	 */
	int (*control_parm)(void *hnd, struct oh_id *id, SaHpiParmActionT act);

	/**
	 * get reset state
	 */
	int (*get_reset_state)(void *hnd, struct oh_id *id, 
			       SaHpiResetActionT *act);

	/**
	 * set_reset state
	 */
	int (*set_reset_state)(void *hnd, struct oh_id *id, 
			       SaHpiResetActionT act);

};


#endif/*__OH_PLUGIN_H*/
