#include <gtest/gtest.h>
#include "bitmap64.h"
#include "bitmap8.h"

TEST(bitmap64, basic) {
  Bitmap64 *bitmap = NewBitmap64(56);
  
  for(int i=0; i<56; i++) {
    ASSERT_EQ(bitmap->Test(i), 0);
  }
  ASSERT_EQ(bitmap->Set(0), true);
  ASSERT_EQ(bitmap->Set(1), true);
  ASSERT_EQ(bitmap->Set(3), true);
  ASSERT_EQ(bitmap->FirstFreePos(), 2);
  FreeBitmap64(bitmap);
}

TEST(bitmap8, basic) {
  char *buf = (char*)calloc(56, 1);
  Bitmap8 *bitmap = new Bitmap8(56, buf);
  
  for(int i=0; i<56; i++) {
    ASSERT_EQ(bitmap->Test(i), 0);
  }
  ASSERT_EQ(bitmap->Set(0), true);
  ASSERT_EQ(bitmap->Set(1), true);
  ASSERT_EQ(bitmap->Set(3), true);
  ASSERT_EQ(bitmap->FirstFreePos(), 2);
  free(buf);
}