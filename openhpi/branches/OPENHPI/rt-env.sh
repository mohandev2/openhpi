#!/bin/sh -x
#
OPENHPI_CONF=./examples/openhpi.conf
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./src/.libs 
PLUGIN_ROOT=./plugins
OPENHPI_PATH=${PLUGIN_ROOT}/dummy:${PLUGIN_ROOT}/ipmi:${PLUGIN_ROOT}/ipmidirect:${PLUGIN_ROOT}/watchdog:${PLUGIN_ROOT}/sysfs:${PLUGIN_ROOT}/text_remote:${PLUGIN_ROOT}/snmp_bc:${LTDL_LIBRARY_PATH}

export OPENHPI_CONF LD_LIBRARY_PATH OPENHPI_PATH
