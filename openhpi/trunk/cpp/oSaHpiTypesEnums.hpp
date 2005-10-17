/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2005
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *    W. David Ashley <dashley@us.ibm.com>
 */


#ifndef Included_oSaHpiTypesEnums
#define Included_oSaHpiTypesEnums

extern "C"
{
#include <SaHpi.h>
}


class oSaHpiTypesEnums {
    public:
        // all the real methods in this class are static
        // so they can be used from any other class
        static SaHpiBoolT oSaHpiTypesEnums::str2torf(const char *str);
        static const char * oSaHpiTypesEnums::torf2str(SaHpiBoolT f);
        static SaHpiLanguageT str2language(const char *strtype);
        static const char * language2str(SaHpiLanguageT value);
        static SaHpiTextTypeT str2texttype(const char *type);
        static const char * texttype2str(SaHpiTextTypeT value);
        static SaHpiEntityTypeT str2entitytype(const char *strtype);
        static const char * entitytype2str(SaHpiEntityTypeT value);
        static SaHpiSensorReadingTypeT str2sensorreadingtype(const char *strtype);
        static const char * sensorreadingtype2str(SaHpiSensorReadingTypeT value);
        static SaHpiSensorUnitsT str2sensorunits(const char *strtype);
        static const char * sensorunits2str(SaHpiSensorUnitsT value);
        static SaHpiSensorModUnitUseT str2sensoruse(const char *strtype);
        static const char * sensoruse2str(SaHpiSensorModUnitUseT value);
        static SaHpiSensorThdMaskT str2sensorthdmask(const char *strtype);
        static const char * sensorthdmask2str(SaHpiSensorThdMaskT value);
        static SaHpiSensorEventCtrlT str2sensoreventctrl(const char *strtype);
        static const char * sensoreventctrl2str(SaHpiSensorEventCtrlT value);
        static SaHpiSensorTypeT str2sensortype(const char *strtype);
        static const char * sensortype2str(SaHpiSensorTypeT value);
        static SaHpiEventCategoryT str2eventcategory(const char *strtype);
        static const char * eventcategory2str(SaHpiEventCategoryT value);
        static SaHpiEventStateT str2eventstate(const char *strtype);
        static const char * eventstate2str(SaHpiEventStateT value);
        static SaHpiCtrlTypeT str2ctrltype(const char *strtype);
        static const char * ctrltype2str(SaHpiCtrlTypeT value);
        static SaHpiCtrlStateDigitalT str2ctrlstatedigital(const char *strtype);
        static const char * ctrlstatedigital2str(SaHpiCtrlStateDigitalT value);
        static SaHpiUint32T str2aggregatestatus(const char *strtype);
        static const char * aggregatestatus2str(SaHpiUint32T value);
        static SaHpiCtrlOutputTypeT str2ctrloutputtype(const char *strtype);
        static const char * ctrloutputtype2str(SaHpiCtrlOutputTypeT value);
        static SaHpiCtrlModeT str2ctrlmode(const char *strtype);
        static const char * ctrlmode2str(SaHpiCtrlModeT value);
};

#endif

