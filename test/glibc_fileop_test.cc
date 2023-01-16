#include <gtest/gtest.h>
extern "C" {
#include <stdio.h>
#include <stdlib.h>
}

class GlibcFileOpTest: public ::testing::Test {
protected:
   void SetUp() override {
      remove("file.txt");
   }
   void TearDown() override {
      remove("file.txt");
   }
};

TEST_F(GlibcFileOpTest, basic) {
   char str1[10], str2[10], str3[10];
   int year;
   FILE * fp;

   fp = fopen ("file.txt", "w+");
   ASSERT_TRUE(fp != NULL);
   fprintf(fp,"We are in 2022\n");
   
   fseek(fp,0,SEEK_SET);
   ASSERT_EQ(fscanf(fp, "%s %s %s %d", str1, str2, str3, &year), 4);

   EXPECT_EQ(std::string(str1), "We");
   EXPECT_EQ(std::string(str2), "are");
   EXPECT_EQ(std::string(str3), "in");
   EXPECT_EQ(year, 2022);

   ASSERT_EQ(fclose(fp), 0);
}