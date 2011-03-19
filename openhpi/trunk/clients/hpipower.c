/*      -*- linux-c -*-
 *
 * (C) Copyright IBM Corp. 2004-2006
 * (C) Copyright Nokia Siemens Networks 2010
 * (C) Copyright Ulrich Kleber 2011
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
 *      Ulrich Kleber <ulikleber@users.sourceforge.net>
 *
 * Changes:
 *      09/06/2010  ulikleber  New option -D to select domain
 *      20/01/2011  ulikleber  Refactoring to use glib for option parsing and
 *                             introduce common options for all clients
 */

#include "oh_clients.h"

#define OH_SVN_REV "$Revision$"

#define  MAX_MANAGED_SYSTEMS 80
#define  HPI_POWER_DEBUG_PRINT(a) if(copt.debug==TRUE)DBG(a)
#define  HPI_POWER_DEBUG_PRINT1(a) if(copt.debug==TRUE)printf(a)


typedef struct COMPUTER_DATA_
{
        SaHpiResourceIdT     ResID;
        SaHpiInt32T          number;     //Enumeration of which computer or blade
        SaHpiEntityLocationT Instance;
        SaHpiRdrT            ResDataRec;
        char                 NameStr[80];//Text Name for the computer or plade
} COMPUTER_DATA;

static gboolean f_down       = FALSE;
static gboolean f_up         = FALSE;
static gboolean f_reset      = FALSE;
static gboolean f_unattended = FALSE;
static gint f_blade          = 0;   
static oHpiCommonOptionsT copt;

static GOptionEntry my_options[] =
{
  { "power-down", 'd', 0, G_OPTION_ARG_NONE, &f_down,       "Power down target object", NULL },
  { "power-up",   'p', 0, G_OPTION_ARG_NONE, &f_up,         "Power on target object",   NULL },
  { "reset",      'r', 0, G_OPTION_ARG_NONE, &f_reset,      "Reset target object",      NULL },
  { "unattended", 'u', 0, G_OPTION_ARG_NONE, &f_unattended, "Unattended",               NULL },
  { "blade",      'b', 0, G_OPTION_ARG_INT,  &f_blade,      "Specify blade n (1...n)",  "n" },
  { NULL }
};


/* Utility function to work with SaHpiTextBufferT */
static void HpiTextBuffer2CString( const SaHpiTextBufferT * tb, char * cstr )
{
    if ( (!tb) || (!cstr) ) {
        return;
    }
    memcpy( cstr, &tb->Data[0], tb->DataLength );
    cstr[tb->DataLength] = '\0';
}


/* Prototypes */
void UsageMessage(char *ProgramName);

/****************************************
 *  main
 *
 *  Hpi Power Utility Entry point routine
 ************************************************/

int main(int argc, char **argv)
{
        SaHpiInt32T         ComputerNumber = 0;  //0..n-1
        SaHpiInt32T         SelectedSystem = 0;  //0..n-1
        SaHpiPowerStateT    Action = 255;  //set it out of range to stand for status;
        COMPUTER_DATA       *ComputerPtr;
        SaHpiBoolT          BladeSelected  = FALSE;
        SaHpiBoolT          MultipleBlades = FALSE;
        SaHpiBoolT          ActionSelected = FALSE;
        GSList*             Computer;
        GSList*             ComputerListHead;
        SaHpiSessionIdT     SessionId;
        SaErrorT            Status;
        SaHpiEntryIdT       RptEntry = SAHPI_FIRST_ENTRY;
        SaHpiEntryIdT       RptNextEntry;
        SaHpiRptEntryT      Report;
        SaHpiInt32T         Index, EntityElement;
        SaHpiPowerStateT    PowerState;
        char                PowerStateString[3][7]={"off\0","on\0","cycled\0"};
        GOptionContext      *context;

        /* Print version strings */
	oh_prog_version(argv[0]);

        /* Parsing options */
        static char usetext[]="- Exercise HPI Power Management APIs\n  "
                              OH_SVN_REV; 
        OHC_PREPARE_REVISION(usetext);
        context = g_option_context_new (usetext);
        g_option_context_add_main_entries (context, my_options, NULL);

        if (!ohc_option_parse(&argc, argv, 
                context, &copt, 
                OHC_ALL_OPTIONS 
                    - OHC_ENTITY_PATH_OPTION //TODO: Feature 880127
                    - OHC_VERBOSE_OPTION )) {    // no verbose mode implemented
                g_option_context_free (context);
		return 1;
	}
        g_option_context_free (context);
        if (f_down) {
            Action = SAHPI_POWER_OFF;
            ActionSelected = TRUE;
        } 
        if (f_up) {
            Action = SAHPI_POWER_ON;
            ActionSelected = TRUE;
        } 
        if (f_reset) {
            Action = SAHPI_POWER_CYCLE;
            ActionSelected = TRUE;
        } 
        if (f_unattended) {
            BladeSelected = TRUE;
            ActionSelected = TRUE;
        } 
        if (f_blade > 0) {
            BladeSelected = TRUE;
            SelectedSystem = f_blade - 1;  //Normalizing to 0...n-1
            if ((SelectedSystem > MAX_MANAGED_SYSTEMS) ||
                (SelectedSystem < 0)) {
               CRIT("hpipower: blade number out of range");
               return 1;   //When we exit here, there is nothing to clean up
            }
        }


        /* Initialize the first of a list of computers */

        HPI_POWER_DEBUG_PRINT("Initializing the List Structure for the computers");
        Computer = g_slist_alloc();
        ComputerListHead = Computer;

        HPI_POWER_DEBUG_PRINT("Allocating space for the information on each computer");
        ComputerPtr = (COMPUTER_DATA*)malloc(sizeof(COMPUTER_DATA));

        Computer->data = (gpointer)ComputerPtr;

        /* Initialize HPI domain and session */
        HPI_POWER_DEBUG_PRINT("Initalizing HPI Session");
        Status = ohc_session_open_by_option ( &copt, &SessionId);
        if (Status == SA_OK)
        {
                /* Find all of the individual systems */
                // regenerate the Resource Presence Table(RPT)
                HPI_POWER_DEBUG_PRINT("Hpi Discovery");
                Status = saHpiDiscover(SessionId);
        } else {
                CRIT("Initalizing HPI Session FAILED, code %s", oh_lookup_error(Status));
                return -1;
        }

        HPI_POWER_DEBUG_PRINT("Walking through all of the Report Tables");
        while ((Status == SA_OK) && (RptEntry != SAHPI_LAST_ENTRY))
        {
                HPI_POWER_DEBUG_PRINT1("@");
                Status = saHpiRptEntryGet(SessionId,
                                          RptEntry,
                                          &RptNextEntry,
                                          &Report);
                RptEntry = RptNextEntry;

                // Blades will have the first Element of the Entity Path set to SBC_BLADE
                EntityElement = 0;
                HPI_POWER_DEBUG_PRINT1(".");
                if (Report.ResourceCapabilities & SAHPI_CAPABILITY_POWER)
                {
                        char tagbuf[SAHPI_MAX_TEXT_BUFFER_LENGTH + 1];

                        HPI_POWER_DEBUG_PRINT1("#");
                        // We have found a Blade
                        ComputerPtr->ResID = Report.ResourceId;
                        /* enumerate this list as created */
                        ComputerPtr->number = ComputerNumber;
                        ComputerNumber++;
                        ComputerPtr->Instance =
                                Report.ResourceEntity.Entry[EntityElement].EntityLocation;
                        // find a Name string for this blade
                        HpiTextBuffer2CString( &Report.ResourceTag, tagbuf );
                        snprintf(ComputerPtr->NameStr, sizeof(ComputerPtr->NameStr),
                                "%s %d",
                                tagbuf,
                                (int) ComputerPtr->Instance);

                        // Create a new allocation for another system
                        ComputerPtr = (COMPUTER_DATA*)malloc(sizeof(COMPUTER_DATA));
                        // Add another member to the list
                        Computer = g_slist_append(Computer,(gpointer)ComputerPtr);
                        // set a flag that we are working with blades
                        MultipleBlades = TRUE;

                }
        }

        HPI_POWER_DEBUG_PRINT("Generating Listing of options to choose from:");
        /* If parsed option does not select blade and
       more than one is found */
        if ((MultipleBlades == TRUE) && (BladeSelected == FALSE) && (Status == SA_OK))
        {
                HPI_POWER_DEBUG_PRINT("Printing out a listing of all the blades");
                for (Index = 0; Index < ComputerNumber; Index++)
                {
                        HPI_POWER_DEBUG_PRINT1("$");
                        // obtain the information for this computer
                        ComputerPtr = g_slist_nth_data(ComputerListHead, Index);
                        if (ComputerPtr == NULL)
                        {
                                printf("Call returned a NULL\n");
                                break;
                        }

                        // retrieve the power status for this computer
                        HPI_POWER_DEBUG_PRINT1("%%");
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
                        if (scanf("%d",&Index) == 0) {
				printf("Incorrect number\n");
			}
                        Index--; //normalize to 0..n-1
                        printf("\n");
                }
                BladeSelected = TRUE;
                SelectedSystem = Index;
        }
        HPI_POWER_DEBUG_PRINT("Generating Listing of Actions to choose from");
        /* If action is not selected */
        if ((ActionSelected == FALSE) && (Status == SA_OK))
        {
                /* prompt user to select an action */
                printf("\nSelect Action: 0 - Off; 1 - On; 2 - Reset; 3 - Status \n\r");
                printf("Enter a number 0 to 3:  ");
                if (scanf("%d", &Index) == 0) {
			Index = -1;
		}
                switch (Index)
                {
                case 0:
                        Action = SAHPI_POWER_OFF;
                        break;
                case 1:
                        Action = SAHPI_POWER_ON;
                        break;
                case 2:
                        Action = SAHPI_POWER_CYCLE;
                        break;
                default:
                        Action = 255;  //Out of Range for "Status"
                        break;
                }
        }
        /* execute the command */

        if (Status == SA_OK)
        {
                HPI_POWER_DEBUG_PRINT("Executing the command");
                // obtain the information for this computer
                ComputerPtr = g_slist_nth_data(ComputerListHead, SelectedSystem);
                if (ComputerPtr == NULL)
                {
                        printf("Error: Selected system %d was not found.\n", SelectedSystem);
                        return -1;
                }

                if (Action <= SAHPI_POWER_CYCLE)
                {
                        HPI_POWER_DEBUG_PRINT("Setting a New Power State");
                        // Set the new power status for this computer
                        Status = saHpiResourcePowerStateSet(SessionId,
                                                            ComputerPtr->ResID,
                                                            Action);
                        /* return status */
                        if (Status == SA_OK)
                        {
                                printf("\n%s -- %20s has been successfully powered %s\n",
                                       argv[0],
                                       ComputerPtr->NameStr,
                                       PowerStateString[Action]);
                        }
                }
                else   // Report Power status for the system
                {
                        HPI_POWER_DEBUG_PRINT("Getting the Power Status\r");
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
        HPI_POWER_DEBUG_PRINT("Clean up");
        /* clean up */
        saHpiSessionClose(SessionId);

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
                HPI_POWER_DEBUG_PRINT("Reporting Bad Status");
                CRIT("Error: %s", oh_lookup_error(Status));
        }

        return(Status);
}


/* end hpipower.c */
