#include <stdio.h>
#include <stdint.h>

#include "v1190unpack.h"

v1190event::v1190event() // ctor
{
   error = false;
   event_count = 0;
   geo = 0;
}

v1190event* UnpackV1190(const char** data8, int* datalen, bool verbose)
{
   const uint32_t *data = (const uint32_t*)(*data8);
   int count = (*datalen)/4;

   v1190event* e = new v1190event();

   bool done = false;

   for (int i=0; i<count; i++) {
      switch ((data[i] >> 27)  & 0x1F) {
      case 0x8:  // Global Header
         {
            e->event_count = (data[i] >> 5) & 0x3FFFFF;
            e->geo = data[i] & 0x1F;
            if (verbose)
               printf("%d 0x%08x: global header: event count %d, geo 0x%2x\n", i, data[i], e->event_count, e->geo);
            break;
         }
      case 0x1:  // TDC Header
         {
            e->tdc_header_tdc = (data[i] >> 24) & 0x3;
            e->tdc_header_event_id  = (data[i] >> 12) & 0xFFF;
            e->tdc_header_bunch_id = data[i] & 0xFFF;
            if (verbose)
               printf("%d 0x%08x: tdc header:  tdc %d, event_id %d, bunch_id %d\n", i, data[i], e->tdc_header_tdc, e->tdc_header_event_id, e->tdc_header_bunch_id);
            break;
         }
      case 0x0:  // TDC measurement
         {
            v1190hit h;
            h.trailing = (data[i] >> 26) & 1;
            h.channel  = (data[i] >> 19) & 0x7F;
            h.measurement = data[i] & 0x7FFFF;
            
            e->hits.push_back(h);
            
            if (verbose)
               printf("%d 0x%08x: tdc measurement: te %d, chan %d, meas %d\n", i, data[i], h.trailing, h.channel, h.measurement);
            break;
         }
      case 0x3:  // TDC Trailer
         {
            e->tdc_trailer_tdc = (data[i] >> 24) & 3;
            e->tdc_trailer_event_id = (data[i] >> 12) & 0xFFF;
            e->tdc_trailer_word_count = (data[i]) & 0xFFF;
            
            if (verbose)
               printf("%d 0x%08x: tdc trailer: tdc %d, event id %d, word count %d\n", i, data[i], e->tdc_trailer_tdc, e->tdc_trailer_event_id, e->tdc_trailer_word_count);
            if (e->tdc_trailer_event_id != e->tdc_header_event_id) {
               e->error = true;
               printf ("v1190unpack: event number mismatch: trailer: %d, header: %d\n", e->tdc_trailer_event_id, e->tdc_header_event_id);
            }
            break;
         }
      case 0x4:  // TDC Error
         {
            e->tdc_error_tdc = (data[i] >> 24) & 3;
            e->tdc_error_flags = (data[i]) & 0xFFF;
            if (e->tdc_error_flags)
               e->error = true;
            if (verbose)
               printf("%d 0x%08x: tdc error: tdc %d, error flags 0x%x\n", i, data[i], e->tdc_error_tdc, e->tdc_error_flags);
            break; 
         }
      case 0x11:  // Extended Trigger Time
         {
            e->ettt =  data[i] & 0x7FFFFFF;
            if (verbose)
               printf("%d 0x%08x: extended trigger time 0x%x\n", i, data[i], e->ettt);
            break;
         }
      case 0x10:  // Trailer
         {
            done = true;
            e->trailer_trigger_lost = (data[i] >> 26) & 1;
            e->trailer_output_buffer_overflow = (data[i] >> 25) & 1;
            e->trailer_tdc_error = (data[i] >> 24) & 1;
            e->trailer_word_count = (data[i] >> 5) & 0xFFFF;
            e->trailer_geo = (data[i]) & 0x1F;
            if (e->trailer_tdc_error)
               e->error = true;
            if (verbose)
               printf("%d 0x%08x: trailer: status TL %d, OBO %d, TDC_E %d, word count %d, geo 0x%x\n", i, data[i], e->trailer_trigger_lost, e->trailer_output_buffer_overflow, e->trailer_tdc_error, e->trailer_word_count, e->trailer_geo);
            if (e->trailer_geo != e->geo) {
               e->error = true;
               printf ("v1190unpack: geo mismatch\n");
            }
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
      }

      if (done) {
         i++;
         (*data8)   += i*4;
         (*datalen) -= i*4;
         break;
      }
   }
   return e;
}

void v1190event::Print() const
{
   printf("v1190event: error %d, ec %d, geo 0x%x, tl %d, obo %d, tdc_e %d, wc %d, %d hits\n", error, event_count, geo, trailer_trigger_lost, trailer_output_buffer_overflow, trailer_tdc_error, trailer_word_count, (int)hits.size());
}

/* emacs
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
