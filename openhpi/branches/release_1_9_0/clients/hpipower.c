/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2004
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  This
 * file and program are licensed under a BSD style license.  See
 * the Copying file included with the OpenHPI distribution for
 * full licensing terms.
 *
 * Author(s):
 *      Carl McAdams <carlmc@us.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "SaHpi.h"
#include <getopt.h>



SaHpiUint8T PrgVer[]="1.0";
SaHpiUint8T PrgName[] = "hpipower";


#define  PROGRAM_HEADER (printf("%s ver %s\n", PrgName,PrgVer))
#define  MAX_MANAGED_SYSTEMS 80
#define  HPI_POWER_DEBUG_PRINT(a) if(DebugPrints==TRUE)printf(a)

typedef struct COMPUTER_DATA_
{
        SaHpiResourceIdT     ResID;
        SaHpiInt32T          number;     //Enumeration of which computer or blade
        SaHpiEntityLocationT Instance;
        SaHpiRdrT            ResDataRec;  
        char                 NameStr[80];//Text Name for the computer or plade
} COMPUTER_DATA;


//Prototypes
void UsageMessage(char *ProgramName);
void AppHpiPrintError(SaErrorT Error, char *ProgramName);

/*+**************************************
*  main
*  
*  Hpi Power Utility Entry point routine
*
*  Parameters:
*     int - argc, 
*     char pointer pointer - argv
*  Program Command line Arguments:
*         'd' power down 
*         'p' power up 
*         'r' hard reset 
*         'u' unattended  
*         'b:n' Blade number
*         '?' useage - help
*         'x' debug messages 
*       Reserved Platform Specific for future use:
*         'c' 'o' 'n' 's'
************************************************/

int main(int argc, char **argv)
{
        SaHpiInt32T         ComputerNumber;  //0..n-1
        SaHpiInt32T         SelectedSystem;  //0..n-1
        SaHpiHsPowerStateT  Action;         
        COMPUTER_DATA       *ComputerPtr;
        SaHpiBoolT          BladeSelected;
        SaHpiBoolT          MultipleBlades;
        SaHpiBoolT          ActionSelected;
        SaHpiBoolT          PrintUsage;
        SaHpiBoolT          DebugPrints;
        GSList*             Computer;
        GSList*             ComputerListHead;
        int                 option;
        SaHpiSessionIdT     SessionId;
        SaErrorT            Status, Clean_Up_Status;
        SaHpiEntryIdT       RptEntry, RptNextEntry;
        SaHpiRptEntryT      Report;
        SaHpiInt32T         Index, EntityElement;
        SaHpiHsPowerStateT  PowerState;
        char                PowerStateString[3][7]={"off\0","on\0","cycled\0"};
        SaHpiVersionT       HpiVersion;

        /*
        // Print out the Program name and Version
        */
        PROGRAM_HEADER;

        /* Set Program Defaults */
        ComputerNumber = 0;
        SelectedSystem = 0;
        Action = 255;  //set it out of range to stand for status
        BladeSelected  = FALSE;
        MultipleBlades = FALSE;
        ActionSelected = FALSE;
        PrintUsage     = FALSE;
        DebugPrints    = FALSE;
        RptEntry       = SAHPI_FIRST_ENTRY;

        /* Parse out option instructions */
        while (1)
        {
                option = getopt(argc, argv, "dpruxb:");
                if ((option == EOF) || (PrintUsage == TRUE))
                {
                break;  //break out of the while loop
                }
                switch (option)
                {
                case 'd':   
                        Action = SAHPI_HS_POWER_OFF;
                        ActionSelected = TRUE;
                break;
                case 'p':   
                        Action = SAHPI_HS_POWER_ON;
                        ActionSelected = TRUE;
                        break;
                case 'r':   
                        Action = SAHPI_HS_POWER_CYCLE;
                        ActionSelected = TRUE;
                        break;
                case 'u':   
                        BladeSelected = TRUE;
                        ActionSelected = TRUE;
                        break;
                case 'x':   
                        DebugPrints = TRUE;
                        break;
                case 'b':   
                        if (*optarg == 0)
                        {
                                PrintUsage = TRUE;
                                break;  //no argument
                        }
                        SelectedSystem = atoi(optarg) - 1;  //Normalizing to 0...n-1
                        if ((SelectedSystem > MAX_MANAGED_SYSTEMS) ||
                            (SelectedSystem < 0))
                        {
                                //Argument is out of Range 
                                PrintUsage = TRUE;
                        }
                        BladeSelected = TRUE;
                        break;
                default:    
                        PrintUsage = TRUE;              
                        break;
                }  //end of switch statement
        } //end of argument parsing while loop

        if (PrintUsage == TRUE)
        {
                UsageMessage(PrgName);
/*BUG:  what is the exit code for bad argument?*/
                exit(0);   //When we exit here, there is nothing to clean up
        }

        /* Initialize the first of a list of computers */

        HPI_POWER_DEBUG_PRINT("1.0 Initializing the List Structure for the computers\n"); 
        Computer = g_slist_alloc();
        ComputerListHead = Computer;
        HPI_POWER_DEBUG_PRINT("1.1 Allocating space for the information on each computer\n");
        ComputerPtr = (COMPUTER_DATA*)malloc(sizeof(COMPUTER_DATA));

        Computer->data = (gpointer)ComputerPtr;

        /* Initialize HPI domain and session */
        HPI_POWER_DEBUG_PRINT("2.0 Initalizing HPI\n");
        Status = saHpiInitialize(&HpiVersion);
        if (Status == SA_OK)
        {
                HPI_POWER_DEBUG_PRINT("2.1 Initalizing HPI Session\n");
                Status = saHpiSessionOpen(SAHPI_DEFAULT_DOMAIN_ID,
                                          &SessionId,
                                          NULL);
        }
        if (Status == SA_OK)
        {
                /* Find all of the individual systems */
                // regenerate the Resource Presence Table(RPT)
                HPI_POWER_DEBUG_PRINT("2.2 Hpi Discovery\n");
                Status = saHpiResourcesDiscover(SessionId);
        }

        HPI_POWER_DEBUG_PRINT("3.0 Walking through all of the Report Tables\n");
        while ((Status == SA_OK) && (RptEntry != SAHPI_LAST_ENTRY))
        {
                HPI_POWER_DEBUG_PRINT("@");
                Status = saHpiRptEntryGet(SessionId,
                                          RptEntry,
                                          &RptNextEntry,
                                          &Report);
                RptEntry = RptNextEntry;

                // Blades will have the first Element of the Entity Path set to SBC_BLADE
                EntityElement = 0;
                HPI_POWER_DEBUG_PRINT(".");
                if (Report.ResourceEntity.Entry[EntityElement].EntityType == 
                    SAHPI_ENT_SBC_BLADE)
                {
                        HPI_POWER_DEBUG_PRINT("#");
                        // We have found a Blade
                        ComputerPtr->ResID = Report.ResourceId;
                        /* enumerate this list as created */
                        ComputerPtr->number = ComputerNumber;
                        ComputerNumber++;
                        ComputerPtr->Instance = 
                                Report.ResourceEntity.Entry[EntityElement].EntityLocation;
                        // find a Name string for this blade
                        sprintf(ComputerPtr->NameStr,
                                "%s %d",
                                (char*)Report.ResourceTag.Data,
                                (int) ComputerPtr->Instance);

                        // Create a new allocation for another system
                        ComputerPtr = (COMPUTER_DATA*)malloc(sizeof(COMPUTER_DATA));
                        // Add another member to the list
                        Computer = g_slist_append(Computer,(gpointer)ComputerPtr);
                        // set a flag that we are working with blades
                        MultipleBlades = TRUE;

                }
        }

        HPI_POWER_DEBUG_PRINT("\n4.0 Generating Listing of options to choose from:\n");
        /* If parsed option does not select blade and 
       more than one is found */
        if ((MultipleBlades == TRUE) && (BladeSelected == FALSE) && (Status == SA_OK))
        {
                HPI_POWER_DEBUG_PRINT("4.1 Printing out a listing of all the blades\n");
                for (Index = 0; Index < ComputerNumber; Index++)
                {
                        HPI_POWER_DEBUG_PRINT("$");
                        // obtain the information for this computer
                        ComputerPtr = g_slist_nth_data(ComputerListHead, Index);
                        if (ComputerPtr == NULL)
                        {
                                printf("Call returned a NULL\n");
                                break;
                        }

                        // retrieve the power status for this computer
                        HPI_POWER_DEBUG_PRINT("%%");
                        PowerState = 0;
                        Status = saHpiResourcePowerStateGet(SessionId,
                                                            ComputerPtr->ResID,
                                                            &PowerState);
                        if (Status != SA_OK)
                        {
                                printf("%s does not support PowerStateGet", 
                                       ComputerPtr->NameStr);
                        }

                        /* Print out all of the systems */
                        printf("%2d) %20s  - %s \n\r", (Index + 1), 
                               ComputerPtr->NameStr, 
                               PowerStateString[PowerState]);
                }
                /* Prompt user to select one */
                while ((Index >= ComputerNumber) || (Index < 0))
                {
                        printf("\nEnter the number for the desired blade: ");
                        scanf("%d",&Index);
                        Index--; //normalize to 0..n-1
                        printf("\n");
                }
                BladeSelected = TRUE;
                SelectedSystem = Index;
        }
        HPI_POWER_DEBUG_PRINT("4.2 Generating Listing of Actions to choose from\n");
        /* If action is not selected */
        if ((ActionSelected == FALSE) && (Status == SA_OK))
        {
                /* prompt user to select an action */
                printf("\nSelect Action: 0 - Off; 1 - On; 2 - Reset; 3 - Status \n\r");
                printf("Enter a number 0 to 3:  ");
                scanf("%d", &Index);
                switch (Index)
                {
                case 0:    
                        Action = SAHPI_HS_POWER_OFF;
                        break;
                case 1:     
                        Action = SAHPI_HS_POWER_ON;
                        break;
                case 2:     
                        Action = SAHPI_HS_POWER_CYCLE;
                        break;
                default:    
                        Action = 255;  //Out of Range for "Status"
                        break;
                }
        }
        /* execute the command */

        if (Status == SA_OK)
        {
                HPI_POWER_DEBUG_PRINT("5.0 Executing the command\n\r");
                // obtain the information for this computer
                ComputerPtr = g_slist_nth_data(ComputerListHead, SelectedSystem);

                if (Action <= SAHPI_HS_POWER_CYCLE)
                {
                        HPI_POWER_DEBUG_PRINT("5.1 Setting a New Power State\n\r");
                        // Set the new power status for this computer
                        Status = saHpiResourcePowerStateSet(SessionId,
                                                            ComputerPtr->ResID,
                                                            Action);
                        /* return status */
                        if (Status == SA_OK)
                        {
                                printf("\n%s -- %20s has been successfully powered %s\n",
                                       PrgName, 
                                       ComputerPtr->NameStr, 
                                       PowerStateString[Action]);
                        }
                } 
                else   // Report Power status for the system
                {
                        HPI_POWER_DEBUG_PRINT("5.2 Getting the Power Status\n\r");
                        // retrieve the power status for this computer
                        PowerState = 0;
                        Status = saHpiResourcePowerStateGet(SessionId,
                                                            ComputerPtr->ResID,
                                                            &PowerState);
                        if (Status != SA_OK)
                        {
                                printf("%s does not support PowerStateGet", 
                                       ComputerPtr->NameStr);
                        }

                        /* Print out Status for this system */
                        printf("%2d) %20s  - %s \n\r", (ComputerPtr->number + 1), 
                               ComputerPtr->NameStr, 
                               PowerStateString[PowerState]);
                }
        }
        HPI_POWER_DEBUG_PRINT("6.0 Clean up");
        /* clean up */
        Clean_Up_Status = saHpiSessionClose(SessionId);
        Clean_Up_Status = saHpiFinalize();
        //Free all of the Allocations for the Computer data
        Computer = ComputerListHead;
        while (Computer != NULL)
        {
                free(Computer->data);
                Computer = g_slist_next(Computer);
        }
        //Free the whole List
        g_slist_free(ComputerListHead);

        /* return status code and exit */

        if (Status != SA_OK)
        {
                HPI_POWER_DEBUG_PRINT("7.0 Reporting Bad Status");
                AppHpiPrintError(Status, PrgName);
        }

        return(Status);
}

void AppHpiPrintError(SaErrorT Error,char *ProgramName)
{
        printf("\n%s Generated an Error: ",ProgramName);
        switch (Error)
        {
        case SA_ERR_HPI_ERROR:              
                printf("Unspecified Error Occurred \n\r");
                break;
        case SA_ERR_HPI_UNSUPPORTED_API:    
                printf("HPI does not support one of the calls \n\r");
                break;
        case SA_ERR_HPI_BUSY:               
                printf("Target Device is Busy \n\r");
                break;
        case SA_ERR_HPI_INVALID:            
                printf("Request is Fundamentaly Invalid \n\r");
                break;
        case SA_ERR_HPI_INVALID_CMD:        
                printf("Object does not support this command\n\r");
                break;
        case SA_ERR_HPI_TIMEOUT:            
                printf("Requested operation Timed-out\n\r");
                break;
        case SA_ERR_HPI_OUT_OF_SPACE:       
                printf("Command Failed due to lack of Resources\n\r");
                break;
        case SA_ERR_HPI_DATA_TRUNCATED:     
                printf("Unsuffient Data Buffer Size\n\r");
                break;
        case SA_ERR_HPI_DATA_LEN_INVALID:        
                printf("Specified Length is Invalid \n\r");
                break;
        case SA_ERR_HPI_DATA_EX_LIMITS:     
                printf("Supplied Data Exceeds Limits\n\r");
                break;
        case SA_ERR_HPI_INVALID_PARAMS:     
                printf("One or more of the Parameters are invalid\n\r");
                break;
        case SA_ERR_HPI_NOT_PRESENT:        
                printf("Object was not Present \n\r");
                break;
        case SA_ERR_HPI_INVALID_DATA_FIELD: 
                printf("Invalid Data Field \n\r");
                break;
        case SA_ERR_HPI_INVALID_SENSOR_CMD: 
                printf("Invalid Sensor Command\n\r");
                break;
        case SA_ERR_HPI_NO_RESPONSE:        
                printf("There was no Response from Target\n\r");
                break;
        case SA_ERR_HPI_DUPLICATE:          
                printf("Duplicate Request\n\r");
                break;
        case SA_ERR_HPI_UPDATING:           
                printf("Target object is in updating mode\n\r");
                break;
        case SA_ERR_HPI_INITIALIZING:       
                printf("Target object is initializing\n\r");
                break;
        case SA_ERR_HPI_INVALID_SESSION:    
                printf("Invalid Session ID supplied\n\r");
                break;
        case SA_ERR_HPI_INVALID_DOMAIN:     
                printf("Invalid Domain ID supplied\n\r");
                break;
        case SA_ERR_HPI_INVALID_RESOURCE:   
                printf("Invalid Resource ID supplied\n\r");
                break;
        case SA_ERR_HPI_INVALID_REQUEST:    
                printf("Request is invalid in the current context\n\r");
                break;
        case SA_ERR_HPI_ENTITY_NOT_PRESENT: 
                printf("Managed Object is no longer Present \n\r");
                break;
        case SA_ERR_HPI_UNINITIALIZED:      
                printf("HPI has not yet been initialized \n\r");
                break;
        case SA_ERR_HPI_UNKNOWN:            
                printf("HPI does not understand command\n\r");
                break;
        default:                            
                printf("Error Code not handled\n\r");
                break;
        }
        return;
}

void UsageMessage(char *ProgramName)
{
        printf("%s usage: \n\r",ProgramName);
        printf("\n %s [dprubx]\n\r",ProgramName);
        printf("     -d  power down target object \n\r");
        printf("     -p  power on target object \n\r");
        printf("     -r  reset target object \n\r");
        printf("     -u  unattended  \n\r");
        printf("     -b <n>  Specify blade <n> (1...n) \n\r");
        printf("     -x  debug messages \n\r");
        printf("\n");
        return;
}

/* end hpipower.c */
