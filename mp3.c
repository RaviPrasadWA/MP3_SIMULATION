#include "mp3.h"


/**Description: Convert MP3 data to PCM data
* Parameters: Stream handle,a pointer to a buffer for the PCM data,the size of
              the PCM buffer in bytes,a pointer to return the number of
              converted bytes.
* Return value: PDMP3_OK or an error.**/
int pdmp3_read(pdmp3_handle *id,unsigned char *outmemory,size_t outsize,size_t *done){
  if(id && outmemory && outsize && done) {
    *done = 0;
    if(outsize) {
      int res = PDMP3_ERR;

      if (id->ostart) {
        Convert_Frame_S16(id,outmemory,outsize,done);
        outmemory += *done;
        outsize -= *done;
        res = PDMP3_OK;
      }

      while(outsize) {
        if (Get_Inbuf_Filled(id) >= (2*576)) {
          size_t pos = id->processed;
          unsigned mark = id->istart;

          res = Read_Frame(id);
          if(res == PDMP3_OK || res == PDMP3_NEW_FORMAT) {
            size_t batch;

            Decode_L3(id);
            Convert_Frame_S16(id,outmemory,outsize,&batch);
            outmemory += batch;
            outsize -= batch;
            *done += batch;
          }
          else {
            id->processed = pos;
            id->istart = mark;
            break;
          }
        }
        else {
          res = PDMP3_NEED_MORE;
          break;
        }
      } /* outsize */
      if(id->new_header == 1 && res == PDMP3_OK) {
        res = PDMP3_NEW_FORMAT;
      }
      return(res);
    }
    else if(outsize < (2*576)) {
      return(PDMP3_NO_SPACE);
    }
    return(PDMP3_NEED_MORE);
  }
  return(PDMP3_ERR);
}