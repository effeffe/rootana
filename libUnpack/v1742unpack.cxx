#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "v1742unpack.h"

#define MEMZERO(array) memset((array), 0, sizeof(array))

v1742event::v1742event() // ctor
{
   error = false;
   //event_count = 0;
   //geo = 0;
   MEMZERO(adc);
   MEMZERO(adc_tr);
   MEMZERO(adc_overflow);
   MEMZERO(adc_tr_overflow);
}

v1742event* UnpackV1742(const char** data8, int* datalen, bool verbose)
{
   const uint32_t *data = (const uint32_t*)(*data8);
   //int count = (*datalen)/4;

   v1742event* e = new v1742event();

   if (verbose) {
      printf("Header:\n");
      printf("  total event size: 0x%08x (%d)\n", data[0], data[0]&0x0FFFFFFF);
      printf("  board id, pattern, gr mask: 0x%08x\n", data[1]);
      printf("  event counter: 0x%08x (%d)\n", data[2], data[2]);
      printf("  event time tag: 0x%08x (%d)\n", data[3], data[3]);
   }

   // header word 0
   e->total_event_size = data[0]&0x0FFFFFFF;

   // header word 1
   e->board_id = (data[1]>>27)&0x1F;
   e->pattern  = (data[1]>>8)&0x3FFF;
   e->group_mask =  data[1] & 0xF;

   // header word 2
   e->event_counter = data[2] & 0x3FFFFF;

   // header word 3
   e->event_time_tag = data[3];

   const uint32_t *g = data + 4;
   for (int i=0; i<4; i++) {
      
      if (((1<<i)&e->group_mask)==0)
         continue;

      e->len[i]  = g[0] & 0xfff;
      e->tr[i]   = (g[0]>>12)&1;
      e->freq[i] = (g[0]>>16)&3;
      e->cell[i] = (g[0]>>20)&0x3ff;

      g += 1;
      // g points to the data
      
      //for (int k=0; k<10; k++)
      //	printf("  adc data[k]: 0x%08x\n", g[k]);
      
      int k=0;
      
      const uint8_t* p = (const uint8_t*)g;
      int x = 0;
      for (int s=0; s<1024; s++)
         for (int a=0; a<8; a++) {
            int v = 0;
            if (x==0) {
               v = (p[0]) | ((p[1]&0xF)<<8);
               p += 1;
               x = 1;
            } else {
               v = ((p[0]&0xF0)>>4) | ((p[1]&0xFF)<<4);
               p += 2;
               x = 0;
            }
            
            //printf("group %d, channel %d, sample %d: value %6d (0x%03x)\n", i, a, s, v, v);
            
            e->adc[i*8+a][s] = v;
            
            if (v == 0)
               e->adc_overflow[i*8+a] = true;
            
            k++;
            //if (k > 10)
            //abort();
         }
      
      g += e->len[i];
      
      if (e->tr[i]) {
         int trlen = e->len[i]/8;
         
         const uint8_t* p = (const uint8_t*)g;
         int x = 0;
         for (int s=0; s<1024; s++) {
            int v = 0;
            if (x==0) {
               v = (p[0]) | ((p[1]&0xF)<<8);
               p += 1;
               x = 1;
            } else {
               v = ((p[0]&0xF0)>>4) | ((p[1]&0xFF)<<4);
               p += 2;
               x = 0;
            }
	    
            //printf("group %d, channel %d, sample %d: value %6d (0x%03x)\n", i, a, s, v, v);
	    
            e->adc_tr[i][s] = v;
            
            if (v == 0)
               e->adc_tr_overflow[i] = true;
	    
            k++;
            //if (k > 10)
            //abort();
         }
         
         g += trlen;
      }
      
      // g points to the time tag
      if (verbose) {
         printf("  group trigger time tag: 0x%08x\n", g[0]);
      }

      e->trigger_time_tag[i] = g[0];

      g += 1;
      // g point s to the next group
   }


#if 0
            if (e->trailer_word_count != i+1) {
               e->error = true;
               printf ("v1190unpack: word count mismatch: trailer wc %d, but data has %d words\n", e->trailer_word_count, i+1);
            }
            break;
         }
      default:
         {
            if (verbose)
               printf("%d 0x%08x: unexpected data\n", i, data[i]);
            e->error = true;
            printf("v1190unpack: unexpected data word 0x%08x\n", data[i]);
   }
#endif

   return e;
}

void v1742event::Print() const
{
#if 0
   printf("v1742event: error %d, ec %d, geo 0x%x, tl %d, obo %d, tdc_e %d, wc %d, %d hits\n", error, event_count, geo, trailer_trigger_lost, trailer_output_buffer_overflow, trailer_tdc_error, trailer_word_count, (int)hits.size());
#endif
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
