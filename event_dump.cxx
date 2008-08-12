//
// Example event dump program
//
// K.Olchanski
//
// $Id$
//

#include <stdio.h>
#include <sys/time.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>

#include "TMidasOnline.h"
#include "TMidasEvent.h"
#include "TMidasFile.h"
#include "XmlOdb.h"

#include <vector>

VirtualOdb* gOdb = NULL;

int  gEventCutoff = 0;
bool gSaveOdb = false;

void HandleMidasEvent(TMidasEvent& event)
{
  int eventId = event.GetEventId();
  event.Print();
}

int ProcessMidasFile(const char*fname)
{
  TMidasFile f;
  bool tryOpen = f.Open(fname);

  if (!tryOpen)
    {
      printf("Cannot open input file \"%s\"\n",fname);
      return -1;
    }

  int i=0;
  while (1)
    {
      TMidasEvent event;
      if (!f.Read(&event))
	break;

      int eventId = event.GetEventId();
      //printf("Have an event of type %d\n",eventId);

      if ((eventId & 0xFFFF) == 0x8000)
	{
	  // begin run
	  event.Print();

          if (gSaveOdb)
            {
              char fname[256];
              sprintf(fname,"odb%05d.xml", event.GetSerialNumber());

              FILE* fp = fopen(fname,"w");
              if (!fp)
                {
                  fprintf(stderr,"Error: Cannot write ODB to \'%s\', errno %d (%s)\n", fname, errno, strerror(errno));
                  exit(1);
                }

              fwrite(event.GetData(), event.GetDataSize(), 1, fp);
              fclose(fp);

              fprintf(stderr,"Wrote ODB to \'%s\'\n", fname);
              exit(0);
            }

	  //
	  // Load ODB contents from the ODB XML file
	  //
	  if (gOdb)
	    delete gOdb;
	  gOdb = new XmlOdb(event.GetData(),event.GetDataSize());
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
    }
  
  f.Close();

  return 0;
}

#ifdef HAVE_MIDAS

void startRun(int transition,int run,int time)
{
  printf("Begin run: %d\n", run);
}

void endRun(int transition,int run,int time)
{
  printf("End of run %d\n",run);
}

int ProcessMidasOnline(const char* hostname, const char* exptname)
{
   TMidasOnline *midas = TMidasOnline::instance();

   int err = midas->connect(hostname, exptname, "rootana");
   if (err != 0)
     {
       fprintf(stderr,"Cannot connect to MIDAS, error %d\n", err);
       return -1;
     }

   gOdb = midas;

   midas->setTransitionHandlers(startRun,endRun,NULL,NULL);
   midas->registerTransitions();

   /* reqister event requests */

   int req = midas->eventRequest("SYSTEM",-1,-1,(1<<1), true);

   int i=0;
   while (1)
     {
       char pevent[100*1024];
       int size = midas->receiveEvent(req, pevent, sizeof(pevent), true);

       if (size == 0)
         {
           if (!midas->poll(1000))
             break;
           continue;
         }

       if (size <= 0)
         break;

       TMidasEvent event;
       memcpy(event.GetEventHeader(), pevent, sizeof(EventHeader_t));
       event.SetData(size, pevent+sizeof(EventHeader_t));
       event.SetBankList();
       HandleMidasEvent(event);

       if ((i%500)==0)
         {
           printf("Processing event %d\n",i);
         }
       
       i++;
       if ((gEventCutoff!=0) && (i>=gEventCutoff))
         {
           printf("Reached event %d, exiting loop.\n", i);
           break;
         }
     }

   /* disconnect from experiment */
   midas->disconnect();

   return 0;
}

#endif

void help()
{
  printf("\nUsage:\n");
  printf("\n./event_dump.exe [-h] [-Hhostname] [-Eexptname] [-O] [-eMaxEvents] [file1 file2 ...]\n");
  printf("\n");
  printf("\t-h: print this help message\n");
  printf("\t-Hhostname: connect to MIDAS experiment on given host\n");
  printf("\t-Eexptname: connect to this MIDAS experiment\n");
  printf("\t-O: save ODB from midas data file into odbNNNN.xml\n");
  printf("\t-e: Number of events to read from input data files\n");
  printf("\n");
  printf("Example1: print online events: ./event_dump.exe\n");
  printf("Example2: print events from file: ./event_dump.exe /data/alpha/current/run00500.mid.gz\n");
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
 
   std::vector<std::string> args;
   for (int i=0; i<argc; i++)
     {
       if (strcmp(argv[i],"-h")==0)
	 help(); // does not return
       args.push_back(argv[i]);
     }

   const char* hostname = NULL;
   const char* exptname = NULL;

   for (unsigned int i=1; i<args.size(); i++) // loop over the commandline options
     {
       const char* arg = args[i].c_str();
       //printf("argv[%d] is %s\n",i,arg);
	   
       if (strncmp(arg,"-e",2)==0)  // Event cutoff flag (only applicable in offline mode)
	 gEventCutoff = atoi(arg+2);
       else if (strncmp(arg,"-H",2)==0)
	 hostname = strdup(arg+2);
       else if (strncmp(arg,"-E",2)==0)
	 exptname = strdup(arg+2);
       else if (strncmp(arg,"-O",2)==0)
	 gSaveOdb = true;
       else if (strcmp(arg,"-h")==0)
	 help(); // does not return
       else if (arg[0] == '-')
	 help(); // does not return
    }
    	 
   bool flag = false;

   for (unsigned int i=1; i<args.size(); i++)
     {
       const char* arg = args[i].c_str();

       if (arg[0] != '-')  
	 {  
           flag = true;
	   ProcessMidasFile(arg);
	 }
     }

   // if we processed some data files,
   // do not go into online mode.
   if (flag)
     return 0;
	   
#ifdef HAVE_MIDAS
   ProcessMidasOnline(hostname, exptname);
#endif
   
   return 0;
}

//end
