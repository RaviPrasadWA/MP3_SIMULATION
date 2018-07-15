#include "sideband.h"


/**Description: Reads sideinfo from bitstream into buffer for Get_Side_Bits.
* Parameters: Stream handle,TBD
* Return value: TBD**/
static void Get_Sideinfo(pdmp3_handle *id,unsigned sideinfo_size){
  if(Get_Bytes(id,sideinfo_size,id->side_info_vec) != PDMP3_OK) {
    ERR("\nCouldn't read sideinfo %d bytes at pos %d\n",
   sideinfo_size,Get_Filepos(id));
    return;
  }

  id->side_info_ptr = &(id->side_info_vec[0]);
  id->side_info_idx = 0;

}



/**Description: reads 'number_of_bits' from buffer which contains side_info.
* Parameters: Stream handle,number_of_bits to read(max 16)
* Return value: The bits are returned in the LSB of the return value.**/
static unsigned Get_Side_Bits(pdmp3_handle *id,unsigned number_of_bits){
  unsigned tmp;

  /* Form a word of the next four bytes */                  
  tmp =(id->side_info_ptr[0] << 24) |(id->side_info_ptr[1] << 16) |
       (id->side_info_ptr[2] <<  8) |(id->side_info_ptr[3] <<  0);
  /* Remove bits already used */
  tmp = tmp << id->side_info_idx;
  /* Remove bits after the desired bits */
  tmp = tmp >>(32 - number_of_bits);
  /* Update pointers */
  id->side_info_ptr +=(id->side_info_idx + number_of_bits) >> 3;
  id->side_info_idx =(id->side_info_idx + number_of_bits) & 0x07;
  return(tmp);
}
