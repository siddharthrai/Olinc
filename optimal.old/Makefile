CXX      =g++
CXXFLAGS =-O0 -funroll-all-loops -funroll-loops -g

CC      = gcc
CCFLAGS = -O0 -funroll-all-loops -funroll-loops -std=c99 -g

DEPDIR      = .deps/
CXXSOURCES  = $(wildcard *.cpp ../StatisticsManager/*.cpp ../*.cpp)
C_INC_DIR     = . ../common 
CXX_INC_DIR   = . ../ ../common ../StatisticsManager 

PROGRAM_TARGET = optimal

CINCLUDES = $(C_INC_DIR:%=-I %)
CXXINCLUDES = $(CXX_INC_DIR:%=-I %)

CXXOBJECTS  = $(CXXSOURCES:%.cpp=%.o)
COBJECTS    = $(CSOURCES:%.c=%.o)

LIBS=-lz -lm

.PHONY: all clean

all : $(DEPDIR) $(COBJECTS) $(CXXOBJECTS) $(PROGRAM_TARGET)

%.o : %.cpp %.h
	@$(CXX) $(CXXFLAGS) $(CXXINCLUDES) -MMD -c -o $@ $< 
	mv $*.d $(DEPDIR)

%.i : %.cpp %.h
	$(CXX) $(CXXFLAGS) $(CXXINCLUDES) -E -o $@ $< 

%.i : %.c %.h
	$(CC) $(CCFLAGS) $(CINCLUDES) -E -o $@ $< 

%.o : %.c %.h
	$(CC) $(CCFLAGS) $(CINCLUDES) -MMD -c -o $@ $< 
	mv $*.d $(DEPDIR)

$(PROGRAM_TARGET): $(DEPS) $(CXXOBJECTS) $(COBJECTS) $(PROGRAM_DEPS)
	$(CXX) -o $@ $(COBJECTS) $(CXXOBJECTS) $(CXXFLAGS) $(LIBS)

-include $(SRCS:%.cpp=$(DEPDIR)/%.d)

$(DEPDIR):
	mkdir -p $(DEPDIR)

clean:
	rm -rf *.o cache-sim .dips/*.d
