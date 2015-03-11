CXX      =g++
CXXFLAGS =-O0 -funroll-all-loops -funroll-loops -Wall -g

CC      = gcc
CCFLAGS = -O0 -funroll-all-loops -funroll-loops -std=c99 -Wall -g

DEPDIR = .deps
BINDIR = bin
ALLCXXSOURCES  = $(wildcard *.cpp statisticsmanager/*.cpp configloader/*.cpp support/*.cpp common/*.cpp)
CXXSOURCES  = $(filter-out cache-sim.cpp optimal.cpp, $(ALLCXXSOURCES))
CSOURCES = $(wildcard *.c libmhandle/*.c cache/*.c policy/*.c)
C_INC_DIR     = . common libmhandle policy cache
CXX_INC_DIR   = . common statisticsmanager configloader support

PROGRAM_TARGET = cache-sim

CINCLUDES = $(C_INC_DIR:%=-I %)
CXXINCLUDES = $(CXX_INC_DIR:%=-I %)

CXXOBJECTS  = $(CXXSOURCES:%.cpp=%.o)
COBJECTS    = $(CSOURCES:%.c=%.o)

LIBS=-lz -lm

.PHONY: all clean

all : $(DEPDIR) $(COBJECTS) $(CXXOBJECTS) $(PROGRAM_TARGET)

%.o : %.cpp %.h
	echo $(CXXINCLUDES)
	@$(CXX) $(CXXFLAGS) $(CXXINCLUDES) $(CINCLUDES) -MMD -c -o $@ $< 
	mv $*.d $(DEPDIR)

%.i : %.cpp %.h
	$(CXX) $(CXXFLAGS) $(CXXINCLUDES) $(CINCLUDES) -E -o $@ $< 

%.i : %.c %.h
	$(CC) $(CCFLAGS) $(CINCLUDES) -E -o $@ $< 

%.o : %.c %.h
	$(CC) $(CCFLAGS) $(CINCLUDES) -MMD -c -o $@ $< 
	mv $*.d $(DEPDIR)

optimal: $(DEPS) optimal.o $(CXXOBJECTS) $(COBJECTS) $(PROGRAM_DEPS)
	$(CXX) -o $(BINDIR)/$@ $(COBJECTS) $(CXXOBJECTS) optimal.o $(CXXFLAGS) $(LIBS)

cache-sim: $(DEPS) cache-sim.o $(CXXOBJECTS) $(COBJECTS) $(PROGRAM_DEPS)
	$(CXX) -o $(BINDIR)/$@ $(COBJECTS) $(CXXOBJECTS) cache-sim.o $(CXXFLAGS) $(LIBS)

-include $(SRCS:%.cpp=$(DEPDIR)/%.d)

$(DEPDIR):
	mkdir -p $(DEPDIR)

clean:
	rm -rf *.o cache-sim optimal .dips/*.d $(CXXOBJECTS) $(COBJECTS) optimal.o cache-sim.o
