#****************************************************************************
# This is a GNU make (gmake) makefile
#****************************************************************************

# DEBUG can be set to YES to include debugging info, or NO otherwise
DEBUG          := YES

# PROFILE can be set to YES to include profiling info, or NO otherwise
PROFILE        := NO 

# USE_STL can be used to turn on STL support. NO, then STL
# will not be used. YES will include the STL files.
USE_STL := YES

# WIN32_ENV
WIN32_ENV := YES 
#****************************************************************************

USE_CHUNZHEN := YES

CC     := gcc
CXX    := g++
LD     := g++
AR     := ar rc
RANLIB := ranlib

# ifeq (YES, ${WIN32_ENV})
#   RM     := del
# else
#   RM     := rm -f
# endif

#DEBUG_CFLAGS     := -Wall -Wno-format -g -DDEBUG -Wno-deprecated
DEBUG_CFLAGS     := -Wall -g -DDEBUG -Wno-deprecated
RELEASE_CFLAGS   := -Wall -Wno-unknown-pragmas -Wno-format -Wno-deprecated

DEBUG_CXXFLAGS   := ${DEBUG_CFLAGS}
RELEASE_CXXFLAGS := ${RELEASE_CFLAGS}

DEBUG_LDFLAGS    := -g 
RELEASE_LDFLAGS  := -O3

ifeq (YES, ${DEBUG})
   CFLAGS       := ${DEBUG_CFLAGS}
   CXXFLAGS     := ${DEBUG_CXXFLAGS}
   LDFLAGS      := ${DEBUG_LDFLAGS}
else
   CFLAGS       := ${RELEASE_CFLAGS}
   CXXFLAGS     := ${RELEASE_CXXFLAGS}
   LDFLAGS      := ${RELEASE_LDFLAGS}
endif

ifeq (YES, ${PROFILE})
   CFLAGS   := ${CFLAGS} -pg
   CXXFLAGS := ${CXXFLAGS} -pg
   LDFLAGS  := ${LDFLAGS} -pg
endif

#****************************************************************************
# Preprocessor directives
#****************************************************************************

ifeq (YES, ${USE_STL})
  DEFS := -DUSE_STL
else
  DEFS :=
endif

ifeq (YES, ${USE_CHUNZHEN})
  DEFS := -DUSE_IPDB_CZ
endif

#****************************************************************************
# Include paths
#****************************************************************************

INC_PATH = $(shell pwd)
LIB_PATH = $(shell pwd)
LIBRARYS = 

INC_PATH += ../net-utility  ../net-utility
LIB_PATH += 
LIBRARYS += z rt
			 

#****************************************************************************
# Makefile code common to all platforms
#****************************************************************************

CFLAGS   := ${CFLAGS}   ${DEFS}
CXXFLAGS := ${CXXFLAGS} ${DEFS}
LIBS := -pthread

INC_DIR = $(patsubst %, -I%, $(INC_PATH))
LIB_DIR = $(patsubst %, -L%, $(LIB_PATH))
LIBS += $(patsubst %, -l%, $(LIBRARYS))

#****************************************************************************
# Targets of the build
#****************************************************************************

OUTPUT := lbs

#****************************************************************************
# Source files
#****************************************************************************

LOG_SRCS := 	\
	../net-utility/ulog/ulog.cpp

CONFIG_SRCS :=	\
	../net-utility/uconfig/uconfig.cpp	\
	../net-utility/uconfig/uconfig_default.cpp	\

FS_SRCS := \
	../net-utility/utools/ufs.cpp	\
	../net-utility/utools/ustring.cpp	\

NET_SRCS := \
		$(wildcard ../net-utility/unetwork/*.cpp) 	\
		$(wildcard ../net-utility/uthread/*.cpp)	\
		$(wildcard ../net-utility/uhttp/*.cpp)		\

DEP_3RD := \

SRCS := \
		./main.cpp 		\
		./area.cpp 		\
		./lbsdb.cpp		\
		./server.cpp	\
		./serverunit.cpp	\
		./connection.cpp	\
		./log.cpp			\
		./client.cpp		\

ifeq (YES, ${USE_CHUNZHEN})
	SRCS += ipdb_cz.cpp
else
	SRCS += ipdb.cpp
endif

#SRCS += $(LOG_SRCS) $(CONFIG_SRCS) $(FS_SRCS) $(NET_SRCS)  $(DEP_3RD)
SRCS +=  $(FS_SRCS) $(LOG_SRCS) $(NET_SRCS) $(CONFIG_SRCS)

DEP_SOURCES = $(notdir $(SRCS))
OUTPUT_DEP_OBJS = $(patsubst %.cpp,%.o, $(DEP_SOURCES))

#****************************************************************************
# Output
#****************************************************************************
all : $(OUTPUT)

$(OUTPUT) : $(OUTPUT_DEP_OBJS)
	${LD} -o $@ ${LDFLAGS} $^ $(LIBS) ${EXTRA_LIBS} $(LIB_DIR)

.PHONY : clean
clean:
	-${RM} $(OUTPUT)
	-${RM} *.o
	-${RM} *.d
	-${RM} *.d.

ifneq "$(MAKECMDGOALS)" "clean"
    include $(DEP_SOURCES:.cpp=.d)  
endif

%.d :
	@t1=$*; t2=$${t1##*/}; \
    for file in $(SRCS); \
    	do \
    		find=$${file%%$$t2.cpp};        \
			find=$$find$$t2.cpp;		\
			split=$${file##*/};	\
			cmpfile=$$t2.cpp;	\
			if [ $$split = $$cmpfile ] && [ $${find} = $${file} ]; then     \
            	rely_file=$$file;       \
    		fi;     \
    	done;   \
    set -e; rm -f $@; \
    $(CXX) -MM $(CXXFLAGS) $(INC_DIR) $$rely_file > $@.;    \
    cat $@. > $@;   \
    echo "	$(CXX) $(CXXFLAGS) $(SHARED) -c -o $$t2.o $(INC_DIR) $$rely_file" >> $@; \
    sed 's/\.o/\.d/g' < $@. >> $@; \
    rm -f $@.       ;
