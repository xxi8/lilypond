#
# project  LilyPond -- the musical typesetter
# title	   generic variables
# file	   make/Variables.make
# abstract 
#
# do not change this file for site-wide extensions; please use 
# make/out/Site.make; 
# Any change in files in this directory (make/) would be distributed.
#
# Copyright (c) 1997 by    
#   	Jan Nieuwenhuizen <jan@digicash.com>
#	Han-Wen Nienhuys <hanwen@stack.nl>

# toplevel version info, might be handy?
#
include $(depth)/VERSION
#
ifeq (0,${MAKELEVEL})
MAKE:=$(MAKE) --no-builtin-rules
endif

# directory names:


ifdef buildprefix
top-directory := $(shell  cd $(depth); pwd)
abs-sub-directory := $(shell pwd)
relative-sub-directory := $(subst $(top-directory),,$(abs-sub-directory))
outdir=$(buildprefix)/$(relative-sub-directory)/out/
else
buildprefix=$(depth)
outdir=out
endif

# derived names
lily_bindir = $(depth)/bin
distdir = $(depth)/$(DIST_NAME)
module-distdir = $(depth)/$(MODULE_DIST_NAME)
depdir = $(outdir)

flowerout = $(buildprefix)/flower/out
libout = $(buildprefix)/lib/out
lilyout = $(buildprefix)/lily/out
mi2muout = $(buildprefix)/mi2mu/out
makeout = $(buildprefix)/make/out
docout = $(buildprefix)/Documentation/out
binout = $(buildprefix)/bin/out

doc-dir = $(depth)/Documentation
flower-dir = $(depth)/flower
lib-dir = $(depth)/lib
lily-dir = $(depth)/lily
mi2mu-dir = $(depth)/mi2mu
make-dir = $(depth)/make
include-lib = $(depth)/lib/include
include-flower = $(depth)/flower/include


rpm-sources = ${HOME}/rpms/SOURCES
#

include $(makeout)/Configure_variables.make

# user settings:
#
include $(depth)/make/User.make
#
#
# need to be defined in local Makefiles:
# build = ./$(depth)/lily/$(outdir)/.build ######## UGR!
BUILD = $(shell cat $(build))
INCREASE_BUILD = echo `expr \`cat $(build)\` + 1` > .b; mv .b $(build)
#

# the version:
#
VERSION=$(MAJOR_VERSION).$(MINOR_VERSION).$(PATCH_LEVEL)$(MY_PATCH_LEVEL)
TOPLEVEL_VERSION=$(TOPLEVEL_MAJOR_VERSION).$(TOPLEVEL_MINOR_VERSION).$(TOPLEVEL_PATCH_LEVEL)$(TOPLEVEL_MY_PATCH_LEVEL)
#


# module and top level dist:
#
# fix naming, use TOPLEVEL_ prefix _or_ MODULE?
MODULE_DIST_NAME = $(MODULE_NAME)-$(VERSION)
DIST_NAME = lilypond-$(TOPLEVEL_VERSION)
NO_DOOS_DIST = bin flower lib lily make mi2mu out
#

# list of object files:
#
SOURCE_FILES = $(CCFILES) $(EXTRA_SOURCE_FILES)
OFILEC = $(SOURCE_FILES:.c=.o)
OFILECC = $(OFILEC:.cc=.o)
OFILEL = $(OFILECC:.l=.o)
OFILEY = $(OFILEL:.y=.o)
OFILES = $(addprefix $(outdir)/,$(OFILEY))
#

# dummydeps
#
DUMMYDEPS=\
 $(flowerout)/dummy.dep\
 $(libout)/dummy.dep\
 $(lilyout)/dummy.dep\
 $(mi2muout)/dummy.dep\

#

# clean file lists:
#
ERROR_LOG = 2> /dev/null
SILENT_LOG = 2>&1 >  /dev/null
date = $(shell date +%x)

# version stuff:
#
lily-version = $(lilyout)/version.hh
flower-version = $(flowerout)/version.hh
mi2mu-version = $(mi2muout)/version.hh
#

# custom libraries:
#
LIBFLOWER = $(depth)/flower/$(outdir)/$(LIB_PREFIX)flower$(LIB_SUFFIX)
LIBLILY = $(depth)/lib/$(outdir)/$(LIB_PREFIX)lily$(LIB_SUFFIX)
#

# compile and link options:
#
ARFLAGS = ru
CFLAGS = $(ICFLAGS) $(DEFINES) $(INCLUDES) $(USER_CFLAGS) $(EXTRA_CFLAGS)

# added two warnings that are treated by cygwin32's gcc 2.7.2 as errors.
# huh, but still, no warnings even provoced with linux's gcc 2.7.2.1?

# -pipe makes it go faster, but is not supported on all platforms. 
EXTRA_CXXFLAGS= -Wall -W -Wmissing-prototypes -Wmissing-declarations -Wconversion


CXXFLAGS = $(CFLAGS) $(USER_CXXFLAGS) $(EXTRA_CXXFLAGS) $(MODULE_CXXFLAGS)
INCLUDES = -Iinclude -I$(outdir) -I$(include-lib) -I$(libout) -I$(include-flower) -I$(flowerout) 
CXX_OUTPUT_OPTION = $< -o $@
LDFLAGS = $(ILDFLAGS) $(USER_LDFLAGS) $(EXTRA_LDFLAGS) $(MODULE_LDFLAGS) -L$(depth)/lib/out -L$(depth)/flower/out
LOADLIBES = $(EXTRA_LIBES) $(MODULE_LIBES) -lg++ # need lg++ for win32, really!
#

# librarian:
#
AR = ar
AR_COMMAND = $(AR) $(ARFLAGS) $@
#
RANLIB_COMMAND=$(RANLIB) $@
# compiler:
#

DO_CXX_COMPILE=$(DODEP)\
	$(CXX) -c $(CXXFLAGS) $(CXX_OUTPUT_OPTION) 
# "CC = $(CC)"
# "CXX = $(CXX)"
#

# linker:
#
LD = $(CXX)
LD_COMMAND = $(LD) $(LDFLAGS) -o $@
#

# dependencies:
#
depfile = $(depdir)/$(subst .o,.dep,$(notdir $@)) 
DODEP=rm -f $(depfile); DEPENDENCIES_OUTPUT="$(depfile) $(outdir)/$(notdir $@)"
#

# utils:
#
#FLEX = flex
#BISON = bison
#

# generic target names:
#
EXECUTABLE = $(outdir)/$(NAME)$(EXE)
EXECUTABLES = $(EXECUTABLE)
LIB_PREFIX = lib

ifndef LIB_SUFFIX
LIB_SUFFIX = .a
endif

LIBRARY = $(LIB_PREFIX)$(NAME)$(LIB_SUFFIX)
#

#replace to do stripping of certain objects
STRIPDEBUG=true 

DISTFILES=$(EXTRA_DISTFILES) Makefile $(ALL_SOURCES)
DOCDIR=$(depth)/$(outdir)

# .hh should be first. Don't know why
# take some trouble to auto ignore sources and obsolete stuff.
progdocs=$(shell $(FIND) ./ -name '*.hh' |egrep -v 'out/') $(shell $(FIND) ./ -name '*.cc'|egrep -v 'out/')


pod2html=pod2html
pod2groff=pod2man --center="LilyPond documentation" --section="0"\
	--release="LilyPond $(TOPLEVEL_MAJOR_VERSION).$(TOPLEVEL_MINOR_VERSION).$(TOPLEVEL_PATCH_LEVEL)" $< > $@


STRIP=strip --strip-debug
ifdef stablecc
 STABLEOBS=$(addprefix $(outdir)/,$(stablecc:.cc=.o))
endif

# substitute $(STRIP) in Site.make if you want stripping
DO_STRIP=true

