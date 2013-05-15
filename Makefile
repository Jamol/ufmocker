mocktest : fnmock.cpp mocktest.cpp
  g++ -Wall -g -o $@ $^ -DLINUX distorm.a
