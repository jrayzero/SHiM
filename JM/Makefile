###
###     Makefile for JM H.264/AVC encoder/decoder
###
###             generated for UNIX/LINUX environments
###             by Limin Wang(lance.lmwang@gmail.com) 
###

SUBDIRS := lencod rtpdump rtp_loss

### include debug information: 1=yes, 0=no
DBG?= 0
### enforce 32-bit build : 1=yes, 0=no
M32?= 0
### static link : 1=yes, 0=no
STC?= 0
### OPENMP support : 1=yes, 0=no
OPENMP?= 0

USE_SHIM?=0
USE_CPP?=0
VERBOSE?=0
CFLAGS+=-DUSE_SHIM=$(USE_SHIM) -DUSE_CPP=$(USE_CPP)
CXX?= $(shell which clang++)
CC?= $(shell which clang)
ifeq ($(VERBOSE),1)
CC+="-v"
CXX+="-v"
endif

#check for LLVM and silence warnings accordingly
ifeq ($(USE_CPP),1)
LLVM = $(shell $(CXX) --version | grep clang)
else	
LLVM = $(shell $(CC) --version | grep clang)
endif
ifneq ($(LLVM),)
	CFLAGS+=-Qunused-arguments
else
	CFLAGS+=-Wno-unused-but-set-variable
endif
ifeq ($(USE_CPP),1)
	CFLAGS+=-Wno-deprecated -Wno-writable-strings
endif

export DBG
export STC 
export M32
export OPENMP
export CFLAGS

.PHONY: default all distclean clean tags depend $(SUBDIRS)

default: all

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean depend:
	@echo "Cleaning dependencies"
	@for i in $(SUBDIRS); do make -C $$i $@; done

tags:
	@echo "update tag table at top directory"
	@ctags -R .
	@for i in $(SUBDIRS); do make -C $$i $@; done

distclean: clean
	@echo "Cleaning all"
	@rm -f tags
	@for i in $(SUBDIRS); do make -C $$i $@; done

