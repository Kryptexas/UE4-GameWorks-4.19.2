# Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#
# Makefile for building hlslcc on Linux.
#
# We have to build this library with -fvisibility=hidden to avoid symbol
# conflicts with any version of mesa that might get loaded by the GL
# drivers.

ROOT_DIR = hlslcc/src
TEST_SRCS = $(ROOT_DIR)/hlslcc_test/main.cpp
MAIN_SRCS = $(ROOT_DIR)/hlslcc_exe/main.cpp
LIB_SRCS = $(wildcard $(ROOT_DIR)/hlslcc_lib/*.cpp) \
	$(wildcard $(ROOT_DIR)/hlslcc_lib/mesa/*.c) \
	$(wildcard $(ROOT_DIR)/hlslcc_lib/mesa/*.cpp) \
	$(wildcard $(ROOT_DIR)/hlslcc_lib/glsl/*.cpp)

LIB_OBJS := $(LIB_SRCS:.cpp=.o)
LIB_OBJS := $(LIB_OBJS:.c=.o)
MAIN_OBJS = $(MAIN_SRCS:.cpp=.o)

LIB = hlslcc/lib/Linux/libhlslcc.a
MAIN = hlslcc/bin/Linux/hlslcc_64

all: main
lib: $(LIB)
main: $(MAIN)

DEBUG = 1
CC = clang
CXX = clang++
CPPFLAGS = -Wno-switch -fPIC -Wno-unused-value -I$(ROOT_DIR)/hlslcc_lib -fvisibility=hidden
CXXFLAGS = -std=c++11
ifeq ($(DEBUG),1)
CPPFLAGS += -g -O0
else
CPPFLAGS += -O2
endif

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $^

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $^

$(LIB): $(LIB_OBJS)
	mkdir -p $(dir $@)
	ar cr $@ $^

$(MAIN): $(MAIN_OBJS) $(LIB)
	mkdir -p $(dir $@)
	$(CXX) -o $@ $(MAIN_OBJS) -Lhlslcc/lib/Linux -lhlslcc

clean:
	rm -f $(LIB_OBJS) $(MAIN_OBJS) $(LIB) $(MAIN)

.PHONY: all lib main clean

