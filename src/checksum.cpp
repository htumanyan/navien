/**
 * checksum.cpp
 * Hovhannes Tumanyan (htumanyan@gmail.com)
 * Aug, 2024
 *
 * Implementation of the algorithm and test suite for CRC-like, but quite unusual checksum algorithm,
 * that is used by Navien water heaters for communication with external devices (Navien WiFi lite and alike).
 * I was unable to find an industry accepted compatible CRC implementation. 
 * The algorithm was reverse engineered and validated with traces captured over RS485 communication lines.
 */

#include <stdio.h>

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned long  ulong;
typedef char           byte;

// This is the seed value that was observed
// in all collected traces
const byte CHECKSUM_SEED_4B = 0x4b;

// This value was found in the binary
// but not in traces.
const byte CHECKSUM_SEED_62 = 0x62;

/**
 * This structure defines test vectors (input) and expected results.
 * Notice that on the wire the checksum is transmitted as the very last byte
 * after the data vector, while here for simplicity we take off the last byte
 * and store in the "result".
 */
typedef struct{
  const char * vector; // test input - header + data
  const int    len;    // count of bytes in vector
  const char   result; // expected checksum result
} TEST_VEC;

// This is an example of short format packet that is typically 41 bytes long (7 bytes header + 34 bytes data)
// the actual length of this test vector is 40 bytes as the last byte is cut and placed in the "result" field.
const char TEST_VEC_1[] = {0xF7, 0x05, 0x50, 0x50, 0x90, 0x22, 0x42, 0x00, 0x00, 0x25, 0x14, 0x56, 0x49, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0xC2, 0x00, 0x20, 0x02, 0x00, 0x00, 0x00, 0x21, 0x03, 0x99, 0x08, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Same as above but for the longer packet that is usually 49 bytes long.
const char TEST_VEC_2[] = {
  0xF7, 0x05, 0x50, 0x0F, 0x90, 0x2A, 0x45, 0x00, 0x01, 0x01, 0x14, 0x03, 0x1F, 0x00, 0x56, 0x56, 0x48, 0x00, 0x00, 0x00, 0x14, 0x01, 0x74, 0x13, 0x0B, 0x44, 0x00, 0x00, 0x9D, 0x07, 0x60, 0x20, 0x4B, 0x3B, 0x20, 0x00, 0x21, 0x03, 0x00, 0x00, 0x00, 0x00, 0xA6, 0x49, 0x00, 0x00, 0x01, 0x00
};

// Same as above
const char TEST_VEC_3[] = {
  0xF7, 0x05, 0x50, 0x0F, 0x90, 0x2A, 0x45, 0x00, 0x01, 0x01, 0x14, 0x03, 0x1F, 0x00, 0x56, 0x49, 0x4B, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x0B, 0x44, 0x00, 0x00, 0x9D, 0x07, 0x60, 0x20, 0x4B, 0x3B, 0x20, 0x00, 0x21, 0x03, 0x00, 0x00, 0x00, 0x00, 0xA6, 0x49, 0x00, 0x00, 0x01, 0x00

};

const TEST_VEC TEST_VECTORS[] = {
  {TEST_VEC_1, sizeof(TEST_VEC_1), 0x65},
  {TEST_VEC_2, sizeof(TEST_VEC_2), 0x36},
  {TEST_VEC_3, sizeof(TEST_VEC_3), 0xE5}
};

  
byte likely_crc_calc(const byte * buffer,uint len,ushort seed){
  ushort result;

  if (len < 2) {
    result = 0x00;
  }
  else {
    result = 0xff;

    for (uint i = 0; i < len; i++){
      result = result << 1;

      if (result > 0xff){
	result = (result & 0xff) ^ seed;
      }

      // this is important!!
      // the checksum is calculated
      // based on the lower byte, i.e.
      // only the lower byte is XOR-ed
      result = ((byte)result) ^ (ushort)buffer[i];
    }

  }

  return result;
}

  
int main(int argc, char * argv[]){
  printf ("Starting\n");
  for (int i = 0; i < sizeof(TEST_VECTORS) / sizeof(TEST_VEC); i++){
    TEST_VEC v = TEST_VECTORS[i];
    printf ("Checking test vector 0x4b (%d bytes): expected result 0x%02hhx, actual: 0x%02hhx\n",
	    v.len,
	    v.result,

	    
	    likely_crc_calc(v.vector, v.len, CHECKSUM_SEED_4B)
	    );
  }
}










int __fastcall likely_crc_calc(unsigned __int8 *a1, unsigned int len, unsigned __int16 seed_val)

{

  if ( (int)len < 2 )

  {

  *(_WORD *)&byte_200117C4[576] = 0;

  }

  else

  {

  *(_WORD *)&byte_200117C4[576] = 0xFF;

    for ( *(_DWORD *)&byte_200117C4[580] = a1;

    *(_DWORD *)&byte_200117C4[580] < (unsigned int)&a1[(unsigned __int16)len - 1];

          ++*(_DWORD *)&byte_200117C4[580] )

    {

    *(_WORD *)&byte_200117C4[576] *= 2;

      if ( *(unsigned __int16 *)&byte_200117C4[576] > 0xFFu )

      *(_WORD *)&byte_200117C4[576] = byte_200117C4[576] ^ seed_val;

      *(_DWORD *)&byte_200117C4[584] = *(_DWORD *)&byte_200117C4[580];

      *(_WORD *)&byte_200117C4[576] ^= (unsigned __int8)**(_BYTE **)&byte_200117C4[580];

    }

  }

  return byte_200117C4[576];

}



*/

