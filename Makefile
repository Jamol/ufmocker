TARGET = mocktest
$(TARGET) : src/fnmock.cpp src/mocktest.cpp
	g++ -Wall -g -o $@ $^ -DLINUX -Iinclude -Isrc lib/distorm.a

clear :
	rm -f $(TARGET)