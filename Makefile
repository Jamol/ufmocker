
SRCDIR = ./src
LIBDIR = ./libs/linux
OBJDIR = .
BINDIR = .

SRCS = $(SRCDIR)/fnmock.cpp \
	$(SRCDIR)/mocktest.cpp

OBJS = $(addsuffix .o,$(addprefix ,$(basename $(SRCS))))

TARGET = $(BINDIR)/mocktest

CC = g++
INCLUDE = -I./include -I$(SRCDIR)
LIBS = $(LIBDIR)/distorm64.a
CXXFLAGS = -g -Wall -DLINUX

$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $^ $(INCLUDE) $(LIBS)

$(OBJS): %.o: %.cpp
	$(CC) -c $(CXXFLAGS) $< -o $@ $(INCLUDE)
	
clean :
	rm -f $(TARGET) $(OBJS)
