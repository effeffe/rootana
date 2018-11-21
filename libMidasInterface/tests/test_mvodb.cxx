//
// test_mvodb.cxx --- test ODB access functions
//
// K.Olchanski
//

#include <stdio.h>
#include <sys/time.h>
#include <iostream>
#include <assert.h>
#include <signal.h>

#include "TMidasOnline.h"
#include "TMidasFile.h"
#include "TMidasEvent.h"
#include "mvodb.h"

// Main function call

int main(int argc, char *argv[])
{
   setbuf(stdout,NULL);
   setbuf(stderr,NULL);
 
   signal(SIGILL,  SIG_DFL);
   signal(SIGBUS,  SIG_DFL);
   signal(SIGSEGV, SIG_DFL);
   signal(SIGPIPE, SIG_DFL);
 
   const char* hostname = NULL;
   const char* exptname = NULL;
   const char* filename = argv[1];
   bool online  = false;
   bool xmlfile = false;
   bool jsonfile = false;
   bool httpfile = false;
   bool nullodb = false;

   if (!filename)
     online = true;
   else if (strstr(filename, ".xml")!=0)
     xmlfile = true;
   else if (strstr(filename, ".json")!=0)
     jsonfile = true;
   else if (strstr(filename, "null")!=0)
     nullodb = true;
   else
     httpfile = true;

   MVOdb* odb = NULL;

   if (nullodb)
     {
       printf("Using NullOdb\n");
       odb = MakeNullOdb();
     }
   else if (online)
     {
       printf("Using MidasOdb\n");
       TMidasOnline *midas = TMidasOnline::instance();

       int err = midas->connect(hostname, exptname, "test_mvodb");
       if (err != 0)
         {
           fprintf(stderr,"Cannot connect to MIDAS, error %d\n", err);
           return -1;
         }

       odb = MakeMidasOdb(midas->fDB);
     }
#if 0
   else if (xmlfile)
     {
#ifdef HAVE_ROOT_XML
       XmlOdb* odb = new XmlOdb(filename);
       //odb->DumpTree();
       gOdb = odb;
#else
       printf("This program is compiled without support for XML ODB access\n");
       return -1;
#endif
     }
   else if (jsonfile)
     {
#ifdef HAVE_ROOT_XML
       XmlOdb* odb = new XmlOdb(filename);
       //odb->DumpTree();
       gOdb = odb;
#else
       printf("This program is compiled without support for XML ODB access\n");
       return -1;
#endif
     }
   else if (httpfile)
     {
#ifdef HAVE_ROOT
       HttpOdb* odb = new HttpOdb(filename);
       //odb->DumpTree();
       gOdb = odb;
#else
       printf("This program is compiled without support for HTTP ODB access\n");
       return -1;
#endif
     }
   else
     {
#ifdef HAVE_ROOT
       TMidasFile f;
       bool tryOpen = f.Open(filename);

       if (!tryOpen)
         {
           printf("Cannot open input file \"%s\"\n",filename);
           return -1;
         }

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
               //event.Print();

               //
               // Load ODB contents from the ODB XML file
               //
               if (gOdb) {
                 delete gOdb;
                 gOdb = NULL;
               }
#ifdef HAVE_ROOT_XML
               gOdb = new XmlOdb(event.GetData(),event.GetDataSize());
#else
               printf("This program is compiled without support for XML ODB access\n");
#endif
               break;
             }
         }

       if (!gOdb)
         {
           printf("Failed to load ODB from input file \"%s\"\n",filename);
           return -1;
         }
#else
       printf("This program is compiled without support for XML ODB access\n");
       return -1;
#endif
     }
#endif

   int runno = 1234;

   odb->RI("runinfo/run number", 0, &runno, false);

   printf("read run number (RI): %d\n", runno);

#if 0
   printf("read array size of /test: %d\n", gOdb->odbReadArraySize("/test"));
   printf("read array size of /runinfo/run number: %d\n", gOdb->odbReadArraySize("/runinfo/Run number"));
   printf("read array size of /test/intarr: %d\n", gOdb->odbReadArraySize("/test/intarr"));
   printf("read array values:\n");
   int size = gOdb->odbReadArraySize("/test/intarr");
   for (int i=0; i<size; i++)
     printf("  intarr[%d] = %d\n", i, gOdb->odbReadInt("/test/intarr", i));
   printf("read double value: %f\n", gOdb->odbReadDouble("/test/dblvalue"));
   printf("read uint32 value: %d\n", gOdb->odbReadUint32("/test/dwordvalue"));
   printf("read bool value: %d\n", gOdb->odbReadBool("/test/boolvalue"));
   const char* s = gOdb->odbReadString("/test/stringvalue");
   int len=0;
   if (s)
     len = strlen(s);
   printf("read string value: [%s] length %d\n", s, len);

   printf("\nTry non-existent entries...\n\n");

   printf("read array size: %d\n", gOdb->odbReadArraySize("/test/doesnotexist"));
   printf("read uint32 value: %d\n", gOdb->odbReadUint32("/test/doesnotexist", 0, -9999));

   printf("\nTry wrong types...\n\n");

   printf("read float value: %f\n", gOdb->odbReadDouble("/test/fltvalue"));
   printf("read uint32 value: %d\n", gOdb->odbReadUint32("/test/wordvalue"));
   printf("read int value: %d\n", gOdb->odbReadInt("/test/wordvalue"));

   printf("\nTry wrong array indices...\n\n");

   printf("try to index a non-array: %f\n", gOdb->odbReadDouble("/test/dblvalue", 10, -9999));
   printf("try invalid index -1: %d\n", gOdb->odbReadInt("/test/intarr", -1, -9999));
   printf("try invalid index 999: %d\n", gOdb->odbReadInt("/test/intarr", 999, -9999));
   printf("try invalid index 1 for double value: %f\n", gOdb->odbReadDouble("/test/dblvalue", 1, -9999));
#endif
   
   return 0;
}

//end
