include ./Makefile.inc

SRCS = $(wildcard ./*.cpp)
OBJS = $(SRCS:%.cpp=%.o)

INCLUDES  =    -I ../

CXXFLAGS += -DNO_STORAGE

OBJLIB     = libdram.a
OBJLIB_DIR = ../lib/

all : dramsim

dramsim: $(DEPDIR) $(OBJS)
	$(AR) cr $(OBJLIB_DIR)$(OBJLIB) $(OBJS)
	$(RANLIB) $(OBJLIB_DIR)$(OBJLIB)
        
%.o : %.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -MMD -c -o $@ $<
	mv $*.d $(DEPDIR)

-include $(SRCS:%.cpp=$(DEPDIR)/%.d)

clean:
	-rm  -rf *.d $(OBJS) $(OBJLIB) $(DEPDIR) ../lib/*.a

$(DEPDIR):
	mkdir -p $(DEPDIR)
