/*
 * Copyright (C) 2007-2008, Hewlett-Packard Development Company, LLP
 * (C) Copyright 2015-2018 Hewlett Packard Enterprise Development LP
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the Hewlett Packard Enterprise, nor the names
 * of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author(s)
 *      Raghavendra M.S. <raghavendra.ms@hp.com>
 */

#include "hpi_test.h"

static void print_value(SaHpiSensorReadingT *item, char *mes)
{
        char *val;

        if (item->IsSupported != SAHPI_TRUE)
                return;
        switch (item->Type) {
                case SAHPI_SENSOR_READING_TYPE_INT64:
                        printf("%lld %s\n", item->Value.SensorInt64, mes);
                        return;
                case SAHPI_SENSOR_READING_TYPE_UINT64:
                        printf("%llu %s\n", item->Value.SensorUint64, mes);
                        return;
                case SAHPI_SENSOR_READING_TYPE_FLOAT64:
                        printf("%10.3f %s\n", item->Value.SensorFloat64, mes);
                        return;
                case SAHPI_SENSOR_READING_TYPE_BUFFER:
                        val = (char *)(item->Value.SensorBuffer);
                        if (val != NULL)
                                printf("%s %s\n", val, mes);
                        return;
        }
}


static void ShowThres(SaHpiSensorThresholdsT *sensbuff)
{
        printf("    Supported Thresholds:\n");
        print_value(&(sensbuff->LowCritical),
                    "      Lower Critical Threshold(lc):");
        print_value(&(sensbuff->LowMajor), "      Lower Major Threshold(la):");
        print_value(&(sensbuff->LowMinor), "      Lower Minor Threshold(li):");
        print_value(&(sensbuff->UpCritical),
                    "      Upper Critical Threshold(uc):");
        print_value(&(sensbuff->UpMajor), "      Upper Major Threshold(ua):");
        print_value(&(sensbuff->UpMinor), "      Upper Minor Threshold(ui):");
        print_value(&(sensbuff->PosThdHysteresis),
                    "      Positive Threshold Hysteresis(ph):");
        print_value(&(sensbuff->NegThdHysteresis),
                    "      Negative Threshold Hysteresis(nh):");
        printf("\n");
}

int main(int argc, char **argv)
{
        int number_resources=0;
        SaErrorT rv;
        SaHpiSessionIdT sessionid;
        SaHpiResourceIdT resourceid;
        SaHpiResourceIdT resourceid_list[RESOURCE_CAP_LENGTH] = {0};
        SaHpiSensorNumT sensor_num;
        SaHpiSensorDataFormatT format;
        SaHpiCapabilitiesT capability = SAHPI_CAPABILITY_SENSOR;
        SaHpiSensorThresholdsT threshold;

        memset(&format, 0, sizeof(format));
        memset(&threshold, 0, sizeof(SaHpiSensorThresholdsT));
        printf("saHpiSensorReadingGet: Test for hpi sensor reading "
               "get function\n");

        rv = saHpiSessionOpen(SAHPI_UNSPECIFIED_DOMAIN_ID, &sessionid, NULL);
        if (rv != SA_OK) {
                printf("saHpiSessionOpen failed with error: %s\n",
                       oh_lookup_error(rv));
                return rv;
        }

        /* Discover the resources with sensor capability */
        printf("\nListing the resource with sensor capability \n");
        rv = discover_resources(sessionid, capability, resourceid_list,
                                &number_resources);
        if (rv != SA_OK) {
                exit(-1);
        }

        printf("\nPlease enter the resource id: ");
        scanf("%d", &resourceid);

        printf("Press 1 for TEMPERATURE sensor\n");
        printf("Press 2 for POWER sensor\n");
        printf("Press 3 for FAN SPEED sensor\n");
        printf("Enter your choice: ");
        scanf("%d", &sensor_num);

        switch (sensor_num) {
                case 1:
                        sensor_num = OA_SOAP_RES_SEN_TEMP_NUM;
                        format.BaseUnits = SAHPI_SU_DEGREES_C;
                        break;
                case 2:
                        sensor_num = OA_SOAP_RES_SEN_POWER_NUM;
                        format.BaseUnits = SAHPI_SU_WATTS;
                        break;
                case 3:
                        sensor_num = OA_SOAP_RES_SEN_FAN_NUM;
                        format.BaseUnits = SAHPI_SU_RPM;
                        break;
                default :
                        printf("Wrong choice. Exiting");
                        exit (-1);
        }

        format.ReadingType = SAHPI_SENSOR_READING_TYPE_FLOAT64;

        rv = saHpiSensorThresholdsGet(sessionid, resourceid,
                                      sensor_num, &threshold);
        if (rv != SA_OK) {
                printf("saHpiSensorThresholdGet failed with error: %s\n",
                       oh_lookup_error(rv));
                printf("Test case - FAIL\n");
                exit(-1);
        }

        printf("  Thresholds:\n");
        ShowThres(&threshold);

        printf("\nTest case - PASS\n");


        rv = saHpiSessionClose(sessionid);
        return 0;
}
