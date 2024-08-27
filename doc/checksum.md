#  Checksum Algorithm

## Overview

The checksum algorithm initially appeared to be similar to CRC. After trying all known CRC values and implementations and failing to find a match to observed network traces I've pivoted to the hard way of reversing the assembly code and coding the algorithm in C/C++ based on inferred knowledge.

## Data Input
Notice that the checksum is generated across the whol packet, which includes the header and the data payload. Essentially, the validation code removes the very last byte that carries the checksum, computes the checksum on the remaining buffer and then compares the computation result to the removed byte. Similarly, the sender computes the checksum over the whole message that includes the header and data and then appends the computed checksum byte at the end.

## Seed Values

Just like CRC, the Navien checksum protocol employs a seed value. There are two known seed values that are selected in the code based on the value at offset 2 in the header (likely the protocol version). If the version value is 0x50 the seed is 0x4b. Otherwise it is 0x62. Notice that I never observed the 0x4b value in real world traces, so for all practical purposes the value needs to be 0x4b

## Algorithm

```
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
```

### Working Code
There is a [working C/C++ code](/src/checksum.cpp) with test vectors derived from line captures. It can be compiled with GCC as follows:

```
gcc -g crc.cpp
```

It will product the default a.out file in the current working directory that will run the tests if executed.

[Link to the working code with test cases](/src/checksum.cpp)


