//
// testODB.cxx --- test ODB access functions
//
// K.Olchanski
//
// $Id$
//

#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include <assert.h>
#include <signal.h>

#include "TMidasOnline.h"
#include "TMidasFile.h"
#include "TMidasEvent.h"
#ifdef HAVE_ROOT
#include "XmlOdb.h"
#endif

VirtualOdb* gOdb = NULL;

double GetTimeSec()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec + 0.000001*tv.tv_usec;
}

#ifdef HAVE_ROOT
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

	  //char buf[256];
	  //memset(buf,0,sizeof(buf));
	  //memcpy(buf,event.GetData(),255);
	  //printf("buf is [%s]\n",buf);

	  //
	  // Load ODB contents from the ODB XML file
	  //
	  if (gOdb)
	    delete gOdb;
	  gOdb = new XmlOdb(event.GetData(),event.GetDataSize());

	  //startRun(0,event.GetSerialNumber(),0);
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
	  //HandleMidasEvent(event);
	}
	
      if((i%500)==0)
	{
	  //resetClock2time();
	  printf("Processing event %d\n",i);
	  //SISperiodic();
	  //StepThroughSISBuffer();
	}
      
      i++;
      //if ((gEventCutoff!=0)&&(i>=gEventCutoff))
      //	{
      //  printf("Reached event %d, exiting loop.\n",i);
      //  break;
      //}
    }
  
  f.Close();

  //endRun(0,gRunNumber,0);

  // start the ROOT GUI event loop
  //  app->Run(kTRUE);

  return 0;
}
#endif

// Main function call

int main(int argc, char *argv[])
{
   setbuf(stdout,NULL);
   setbuf(stderr,NULL);
 
   signal(SIGILL,  SIG_DFL);
   signal(SIGBUS,  SIG_DFL);
   signal(SIGSEGV, SIG_DFL);
 
   const char* hostname = NULL;
   const char* exptname = NULL;
   bool online = true;

   if (online)
     {
       TMidasOnline *midas = TMidasOnline::instance();

       int err = midas->connect(hostname, exptname, "rootana");
       if (err != 0)
         {
           fprintf(stderr,"Cannot connect to MIDAS, error %d\n", err);
           return -1;
         }

       gOdb = midas;
     }
   else
     {
#ifdef HAVE_ROOT
       //ProcessMidasFile(app,arg);
#endif
     }

   printf("read run number (odbReadInt): %d\n", gOdb->odbReadInt("/runinfo/Run number"));
   printf("read array size: %d\n", gOdb->odbReadArraySize("/test/intarr"));
   printf("read array values:\n");
   int size = gOdb->odbReadArraySize("/test/intarr");
   for (int i=0; i<size; i++)
     printf("  intarr[%d] = %d\n", i, gOdb->odbReadInt("/test/intarr", i));
   printf("read double value: %f\n", gOdb->odbReadDouble("/test/dblvalue"));
   printf("read float value: %f\n", gOdb->odbReadDouble("/test/fltvalue"));
   printf("read uint32 value: %d\n", gOdb->odbReadUint32("/test/dwordvalue"));
   printf("read uint32 value: %d\n", gOdb->odbReadUint32("/test/wordvalue"));
   printf("read int value: %d\n", gOdb->odbReadInt("/test/wordvalue"));
   printf("read bool value: %d\n", gOdb->odbReadBool("/test/boolvalue"));
   
   return 0;
}

//end
