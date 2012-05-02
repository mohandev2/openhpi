#!/usr/bin/python

from openhpi_baselib.HpiDataTypes import *
from openhpi_baselib.Hpi import *
from openhpi_baselib import HpiUtil
from openhpi_baselib import HpiIterators

( rv, sid ) = saHpiSessionOpen( SAHPI_UNSPECIFIED_DOMAIN_ID, None )
if rv != SA_OK:
    print "ERROR: saHpiSessionOpen: %s " % HpiUtil.fromSaErrorT( rv )
    exit()

for rpte in HpiIterators.Rpt( sid ):
    tag = HpiUtil.fromSaHpiTextBufferT( rpte.ResourceTag )
    print "Resource Id: %d, Tag: %s" % ( rpte.ResourceId, tag )

rv = saHpiSessionClose( sid )
if rv != SA_OK:
    print "ERROR: saHpiSessionClose: %s " % HpiUtil.fromSaErrorT( rv )

