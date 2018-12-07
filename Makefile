
VPATH := src:test:examples
#CFLAGS += -g
CFLAGS += -Wall -Iinc $(EXCFLAGS)

#Set the following for cross compilation
ifndef RANLIB
CXX=g++
CC=gcc
AR=ar
RANLIB=ranlib
endif

ifndef ARFLAGS
ARFLAGS=rv
endif

ifndef ODIR
ODIR = ./
endif

$(ODIR)/%.o : %.c
	$(CC) -c $(CFLAGS) -o $@ $<
$(ODIR)/%.o : %.cpp
	$(CXX) -c $(CFLAGS) -o $@ $<

# Three libs: One with all source code (large), one with
# JParserValFact (medium), and one with JDecoder.c (small)

CORESRC = BaAtoi.c  BufPrint.c  JEncoder.c  JParser.c
JSONSRC = AllocatorIntf.c JVal.c JDecoder.c $(CORESRC)
VALFACTSRC = AllocatorIntf.c JVal.c $(CORESRC)
DECODERSRC = JDecoder.c $(CORESRC)


.PHONY: all libs examples c_example clean

#All examples: C and C++
all: libs
	$(MAKE) examples

#Example using C only (no C++)
c_example: libs
	$(MAKE) m2mled$(EXT)

libs:	obj
	$(MAKE) ODIR=obj/jd EXCFLAGS=-DNO_JVAL_DEPENDENCY libJDecoder.a
	$(MAKE) ODIR=obj libJSON.a libValFact.a

obj:
	mkdir obj
	cd obj&&mkdir jd

libJSON.a: $(JSONSRC:%.c=$(ODIR)/%.o)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

libValFact.a: $(VALFACTSRC:%.c=$(ODIR)/%.o)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@ 

libJDecoder.a: $(DECODERSRC:%.c=$(ODIR)/%.o)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@ 

examples: test1$(EXT) staticalloc$(EXT) m2mled$(EXT)

test1$(EXT): test1.o
	$(CXX) -o $@ $^ -L. -lJSON $(EXTRALIBS)

staticalloc$(EXT): StaticAllocatorEx.o
	$(CXX) -o $@ $^ -L. -lValFact $(EXTRALIBS)

m2mled$(EXT): m2m-led.o solib.o
	$(CC) -o $@ $^ -L. -lJDecoder $(EXTRALIBS)

clean:
	rm -rf obj *.a *.o
