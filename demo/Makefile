CXX      =g++
CXXFLAGS =-O0 -funroll-all-loops -funroll-loops -Wall -g

CC      = gcc
CCFLAGS = -O0 -funroll-all-loops -funroll-loops -std=c99 -Wall -g

DEPDIR = .deps
BINDIR = .
PUBCXXSOURCES  = $(wildcard ../support/*.cpp ../common/*.cpp)
ALLCXXSOURCES  = $(wildcard *.cpp ../support/*.cpp ../common/*.cpp)
CXXSOURCES  = $(filter-out access-phase.cpp bitvec-demo.cpp address-map.cpp address-reuse.cpp, $(ALLCXXSOURCES))
CXX_INC_DIR   = . ../support ../cache .. ../policy

PROGRAM_TARGET = access-phase address-reuse

CINCLUDES = $(C_INC_DIR:%=-I %)
CXXINCLUDES = $(CXX_INC_DIR:%=-I %)

CXXOBJECTS  = $(CXXSOURCES:%.cpp=%.o)
PUBCXXOBJECTS  = $(PUBCXXSOURCES:%.cpp=%.o)
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

%.o : %.c
	$(CC) $(CCFLAGS) $(CINCLUDES) -MMD -c -o $@ $< 
	mv $*.d $(DEPDIR)

address-reuse: $(DEPS) address-reuse.o $(CXXOBJECTS) $(COBJECTS) $(PROGRAM_DEPS)
	$(CXX) -o $(BINDIR)/$@ $(COBJECTS) $(CXXOBJECTS) address-reuse.o $(CXXFLAGS) $(LIBS)

access-phase: $(DEPS) access-phase.o $(CXXOBJECTS) $(COBJECTS) $(PROGRAM_DEPS)
	$(CXX) -o $(BINDIR)/$@ $(COBJECTS) $(CXXOBJECTS) access-phase.o $(CXXFLAGS) $(LIBS)

queue-unit-test: queue-unit-test.o $(PUBCXXOBJECTS)
	$(CXX) -o $(BINDIR)/$@ queue-unit-test.o $(CXXFLAGS) $(PUBCXXOBJECTS) $(LIBS)

-include $(SRCS:%.cpp=$(DEPDIR)/%.d)

$(DEPDIR):
	mkdir -p $(DEPDIR)

clean:
	rm -rf *.o cache-sim optimal .dips/*.d $(CXXOBJECTS) $(COBJECTS) optimal.o cache-sim.o
