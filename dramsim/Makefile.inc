FLAGS           = -msse2 -Wall -O0 -g

ifeq ($(ENABLE_DEBUG),1)
        FLAGS = -Wall -Werror -Wno-unknown-pragmas -g -O0
endif

ifeq ($(DISABLE_ASSERT_CHECKS),1)
        FLAGS +=  -DNDEBUG
endif

CXX             = g++
CXXFLAGS        = $(FLAGS)
CC              = gcc-4.4
CFLAGS          = $(FLAGS) -std=gnu99
AR              = ar
ARFLAGS         = cru
RANLIB          = ranlib
DEPDIR          = .deps/
TGT_PATH			  = ../lib/
