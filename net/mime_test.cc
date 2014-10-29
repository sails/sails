#include <stdlib.h>
#include <sys/types.h>
#include <iostream>
#include <gtest/gtest.h>
#include "sails/net/mime.h"


using namespace sails::net;

TEST(MIME, test)
{
  std::vector<std::string> exts = {".js", ".jar", ".png", ".jpg", ".jpg2", ".wm", ".ez"};
  for (std::string& item : exts) {
    MimeType type;
    MimeTypeManager::instance()->GetMimeTypebyFileExtension(item, &type);
    printf("%s content-type:%s/%s\n",item.c_str(), type.Type().c_str(), type.SubType().c_str());
  }
  
}

int main(int argc, char *argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();  
}










