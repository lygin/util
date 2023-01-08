
#ifndef _BITMAP8_H_
#define _BITMAP8_H_

#include <stdint.h>                     /* Standard integers. E.g. uint8_t */
#include <stdlib.h>                     /* Standard library for memory allocation and exit function. */
#include <stdio.h>                      /* Standard I/O operations. E.g. fprintf() */            
#include <string.h>                     /* Header for memory operations. E.g. memset() */
#include <assert.h>

class Bitmap8
{
private:
    uint8_t *bytes;                     /* Byte array to hold bitmap. */
    uint64_t varCountFree;              /* Count variable of free bits. */
    uint64_t varCountTotal;             /* Count variable of total bits. */

public:
    bool Test(uint64_t pos)
    {
      assert(pos < varCountTotal);
      uint64_t idx = pos / 8;
      uint64_t off = pos % 8;
      return bytes[idx] & (1 << (7-off));
    }
    bool Set(uint64_t pos)             /* Set a bit. */
    {
      if(pos < varCountTotal) {
        uint64_t idx = pos / 8;
        uint64_t off = pos % 8;
        if((bytes[idx] & (1 << (7 - off))) == 0) {
          bytes[idx] |= 1<<(7-off);
          varCountFree--;
        }
        return true;
      } else {
        return false;
      }
    }
    bool Reset(uint64_t pos)           /* Clear a bit. */
    {
      if(pos < varCountTotal) {
        uint64_t idx = pos / 8;
        uint64_t off = pos % 8;
        if((bytes[idx] & (1 << (7 - off))) != 0) {
          bytes[idx] &= ~(1<<(7-off));
          varCountFree++;
        }
        return true;
      } else {
        return false;
      }
    }
    uint64_t FirstFreePos()       /* Find first free bit. */
    {
      if (varCountFree > 0) {
        for (unsigned int index = 0; index < varCountTotal / 8; index++) {
          for (int offset = 0; offset <= 7; offset++) {
            if((bytes[index] & (1 << (7 - offset))) == 0) {
              return index * 8 + offset;
            }
          }
        }
      }
      return -1;
    }
    uint64_t countFree()               /* Count of free bits. */
    {
      return varCountFree;
    }
    uint64_t countTotal()              /* Count of total bits. */
    {
      return varCountTotal;
    }
    Bitmap8(uint64_t count, char *buffer) /* Constructor of bitmap. Read buffer to initialize current status. */
    {
      assert(count % 8 == 0);
      assert(buffer != NULL);
      bytes = (uint8_t *)buffer;
      varCountTotal = count;
      varCountFree = 0;
      for (unsigned int index = 0; index < varCountTotal / 8; index++) {
          for (int offset = 0; offset <= 7; offset++) {
              if ((bytes[index] & (1 << (7 - offset))) == 0) {
                  varCountFree++;
              }
          }
      }
    }
    ~Bitmap8() {} /* Destructor of bitmap. Do not free buffer. */
};

#endif