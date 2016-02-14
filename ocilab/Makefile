#/bin/make

.SUFFIXES: .o .c 

CC=/usr/bin/gcc
cc=/usr/bin/gcc

#set ORACLE_HOME 
ifndef ORACLE_HOME
      ORACLE_HOME=/scratch/mbastawa/view_storage/mbastawa_linuxs/oracle
endif

THREADLIBS=-lpthread
CCLIB=-L$(ORACLE_HOME)/lib -lclntsh $(THREADLIBS)

CCINCLUDES = -I$(ORACLE_HOME)/rdbms/public

CCFLAGS=$(CCINCLUDES) -DLINUX -D_GNU_SOURCE -D_REENTRANT -g
LDFLAGS=-g

SO_EXT=.so
RDBMS_VERSION=11
RDBMS_MINOR_VERSION=1

REMOVE=rm -rf
MKLINK=ln
MAKE=make
MAKEFILE=Makefile
SH=sh

all_exes=$(stage1_exe) $(stage2_exe) $(stage3_exe) $(stage4_exe) $(stage5_exe) $(stage6_exe) $(stage7_exe)


stage1_obj=stage1.o helper.o
stage2_obj=stage2.o helper.o
stage3_obj=stage3.o helper.o
stage4_obj=stage4.o helper.o
stage5_obj=stage5.o helper.o
stage6_obj=stage6.o helper.o
stage7_obj=stage7.o helper.o

stage1_exe=stage1
stage2_exe=stage2
stage3_exe=stage3
stage4_exe=stage4
stage5_exe=stage5
stage6_exe=stage6
stage7_exe=stage7


.c.o:
	$(cc) -c $(CCFLAGS) $<

all: clean stage1 stage2 stage3 stage4 stage5 stage6 stage7 

###################################
# Targets for building each stage #
###################################
stage1: $(stage1_obj)
	$(CC) -o $(stage1_exe) $(LDFLAGS) $(stage1_obj) $(CCLIB)

stage2: $(stage2_obj)
	$(CC) -o $(stage2_exe) $(LDFLAGS) $(stage2_obj) $(CCLIB)

stage3: $(stage3_obj)
	$(CC) -o $(stage3_exe) $(LDFLAGS) $(stage3_obj) $(CCLIB)

stage4: $(stage4_obj)
	$(CC) -o $(stage4_exe) $(LDFLAGS) $(stage4_obj) $(CCLIB)

stage5: $(stage5_obj)
	$(CC) -o $(stage5_exe) $(LDFLAGS) $(stage5_obj) $(CCLIB)

stage6: $(stage6_obj)
	$(CC) -o $(stage6_exe) $(LDFLAGS) $(stage6_obj) $(CCLIB)

stage7: $(stage7_obj)
	$(CC) -o $(stage7_exe) $(LDFLAGS) $(stage7_obj) $(CCLIB)

clean: 
	$(REMOVE) $(all_exes) *.o
