//
// Example event dump program
//
// K.Olchanski
//
// $Id: event_dump.cxx 70 2009-06-20 02:12:14Z olchansk $
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>

//#include "TMidasOnline.h"
#include "TMidasEvent.h"
#include "TMidasFile.h"

#include <vector>

int  gEventCutoff = 0;
//bool gSaveOdb = false;

void HandleMidasEvent(TMidasEvent& event)
{
  int eventId = event.GetEventId();
  int eventSN = event.GetSerialNumber();
  //event.Print("a");
  //event.Print();
  //  printf("EventSerial#: %i, EventID: %i\n",eventSN,eventId);

  // Look for some banks: 
  const char* fBank = "OMCM" ;
  int bankLength = 0;
  int bankType = 0;
  void *pdata = 0;

  int found = event.FindBank(fBank, &bankLength, &bankType, &pdata);
  if(found) 
    {
      if (0) {
	printf("Dumping bank %s in hex\n",fBank);
	for (int j = 0; j < bankLength; j++)
	  printf("0x%08x%c", ((uint32_t*)pdata)[j], (j%10==9)?'\n':' ');
	printf("\n");
	printf("Dumping bank %s in dec\n",fBank);
	for (int j = 0; j < bankLength; j++)
	  printf("  %8d%c", ((uint32_t*)pdata)[j], (j%10==9)?'\n':' ');
	printf("\n");
      }
      /// Trigger word definition (64 bit unsigned long):
      ///     - Bits  0-31:  Event number
      ///     - Bits 32-47: Spill number
      ///     - Bits 48-57: Trigger bits
      ///         - Bit 48: Beam spill
      ///         - Bit 49: Pedestal/noise
      ///         - Bit 50: TPC laser
      ///         - Bit 51: POD LED
      ///         - Bit 52: ECAL LED
      ///         - Bit 53: FGD LED
      ///         - Bit 54: FEE calibration
      ///         - Bit 55: TRIP-t cosmic
      ///         - Bit 56: Reserved for FGD cosmic
      ///         - Bit 57: unused
      ///     - Bits 58-63: Instruction
      ///         - Bit 58: Reserved for start-of-run
      ///         - Bit 59: Reserved for end-of-run
      ///         - Bit 60: 1 PPS / counter reset
      ///         - Bit 61: Go to spill mode
      ///         - Bit 62: Go to cosmic mode
      ///         - Bit 63: Decline CTM request/abort
      int eventNum = ((uint32_t*)pdata)[1];
      int spillNum = ((uint32_t*)pdata)[2] & 0xFFFF;
      int trigBits = (((uint32_t*)pdata)[2] >> 16) &  0xFFFF;
      const char* trigType = "";
      int trigBeam = 1;
      switch (trigBits) 
	{
	case 1: 
	  trigType = "Beam spill";
	  break;
	}
      if (trigBits == trigBeam) {
	//	printf("EventSerial#: %i, EventID: %i\n",eventSN,eventId);
	printf(" eventNum %d, spillNum %d, trigBits 0x%04x, %s\n",eventNum,spillNum,trigBits,trigType);
      }
    } else {
      printf("Can't find bank %s\n",fBank);
    }
  //printf("\n");
	  
}

#if 0
int SetOutputFile(const char*foutname)
{
  TMidasFile f;
  bool tryOpen = f.OutOpen(foutname);

  if (!tryOpen)
    {
      printf("Cannot open output file \"%s\"\n",foutname);
      printf("Last error is %d, %s\n",f.GetLastErrno(),f.GetLastError());
      printf("Exiting program\n");
      exit (1);
    }
  return 0;
}
#endif

int ProcessMidasFile(const char* fname, const char* foutname)
{
  TMidasFile f;
  TMidasFile fout;

  bool tryOpen = f.Open(fname);

  if (!tryOpen)
    {
      printf("Cannot open input file \"%s\"\n",fname);
      return -1;
    }

  bool tryOutOpen = fout.OutOpen(foutname);

  if (!tryOutOpen)
    {
      printf("Cannot open output file \"%s\"\n",foutname);
      printf("Last error is %d, %s\n",f.GetLastErrno(),f.GetLastError());
      printf("Exiting program\n");
      exit (1);
    }

  int i=0;
  while (1)
    {
      TMidasEvent event;
      if (!f.Read(&event))
	break;

      int eventId = event.GetEventId();
      printf("Have an event of type %d\n",eventId);

      if ((eventId & 0xFFFF) == 0x8000)
	{
	  // begin run
	  event.Print();
	}
      else if ((eventId & 0xFFFF) == 0x8001)
	{
	  // end run
	  event.Print();
	}
      else
	{
	  event.SetBankList();
	  //event.Print();
	  HandleMidasEvent(event);
	}	
      if((i%500)==0)
	{
	  printf("Processing event %d\n",i);
	}
      
      i++;
      if ((gEventCutoff!=0)&&(i>=gEventCutoff))
	{
	  printf("Reached event %d, exiting loop.\n", i);
	  break;
	}
      // Output all events
      if (!fout.Write(&event)) {
	printf("Error writing event to output\n");
	break;
      }
      printf("Event written to out\n");

    }
  
  f.Close();
  fout.OutClose();

  return 0;
}


void help()
{
  printf("\nUsage:\n");
  printf("\n./event_skim.exe [-h]  [-eMaxEvents] [file1 file2 ...]\n");
  printf("\n");
  printf("\t-h: print this help message\n");
  printf("\t-e: Number of events to read from input data files\n");
  printf("\t-o: Generate output file \n");
  printf("\n");
  printf("Example1: print events from file: ./event_skim.exe /data/alpha/current/run00500.mid.gz\n");
  printf("Example2: output selected events: ./event_skim.exe -o skim_00500.mid.gz /data/alpha/current/run00500.mid.gz\n");
  exit(1);
}

// Main function call

int main(int argc, char *argv[])
{
   setbuf(stdout,NULL);
   setbuf(stderr,NULL);
 
   signal(SIGILL,  SIG_DFL);
   signal(SIGBUS,  SIG_DFL);
   signal(SIGSEGV, SIG_DFL);
   signal(SIGPIPE, SIG_DFL);
 
   std::vector<std::string> args;
   for (int i=0; i<argc; i++)
     {
       if (strcmp(argv[i],"-h")==0)
	 help(); // does not return
       args.push_back(argv[i]);
     }

   const char* outname = NULL;

   for (unsigned int i=1; i<args.size(); i++) // loop over the commandline options
     {
       const char* arg = args[i].c_str();
       //printf("argv[%d] is %s\n",i,arg);
	   
       if (strncmp(arg,"-e",2)==0)  // Event cutoff flag (only applicable in offline mode)
	 gEventCutoff = atoi(arg+2);
       else if (strncmp(arg,"-o",2)==0) { // Skim to output file
	 outname = args[i+1].c_str();
	 i++;
       }
       else if (strcmp(arg,"-h")==0)
	 help(); // does not return
       else if (arg[0] == '-')
	 help(); // does not return
    }
    	 
   for (unsigned int i=1; i<args.size(); i++)
     {
       const char* arg = args[i].c_str();

       if (arg[0] != '-')  
	 {  
	   ProcessMidasFile(arg, outname);
	 }
     }
   
   return 0;
}

//end
