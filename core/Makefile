#***********************************************************
# Copyright (c) 2010-present Peng Fei.  All rights reserved.
#***********************************************************/

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:

# Redistribution and use in source and binary forms must authorized by
# Peng Fei.

# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.

# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.

OBJ_PRELOAD = preload.o os.o config.o
SOURCE_PRELOAD = preload.c os.c config.c

OBJ_DENTRE = utils.o dentre.o options.o instrument.o perfctr.o	\
			 heap.o proc.o vmareas.o
SOURCE_DENTRE = utils.c dentre.c options.c instrument.c perfctr.c	\
				heap.c proc.c vmareas.c

CC = gcc
C_FLAG = -g -O0 -fPIC -shared -DO32  
#C_FLAG = -g -O0 -fPIC -shared -DO32  -DDEBUG -DINTERNAL
C_FLAG_PRELOAD = -DNOT_DENTRE_CORE_PROPER 
C_FLAG_DENTRE  = 
LINK_FLAG = -ldl 

#D_FLAG = 

OBJDIR = ../build/
LIBDIR = ../build/

#$(OBJ) : $(SOURCE)
#	$(CC) ${C_FLAG}  $(SOURCE) -c

ALL:
	cd ${OBJDIR};			\
	rm *.so *.o;			\
	cd -;					\
							\
	$(CC) ${C_FLAG} ${C_FLAG_PRELOAD} ${D_FLAG} ${TOOL_FLAG} linux/preload.c          -c -o  ${OBJDIR}preload.o;		\
	$(CC) ${C_FLAG} ${C_FLAG_PRELOAD} ${D_FLAG} ${TOOL_FLAG} linux/os.c               -c -o  ${OBJDIR}os.o;				\
	$(CC) ${C_FLAG} ${C_FLAG_PRELOAD} ${D_FLAG} ${TOOL_FLAG} config.c                 -c -o  ${OBJDIR}config.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} utils.c                  -c -o  ${OBJDIR}utils.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} dentre.c                 -c -o  ${OBJDIR}dentre.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} options.c                -c -o  ${OBJDIR}options.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} mips/instrument.c        -c -o  ${OBJDIR}instrument.o;		\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} perfctr.c                -c -o  ${OBJDIR}perfctr.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} heap.c                   -c -o  ${OBJDIR}heap.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} mips/proc.c              -c -o  ${OBJDIR}proc.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} vmareas.c                -c -o  ${OBJDIR}vmareas.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} fcache.c                 -c -o  ${OBJDIR}fcache.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} moduledb.c               -c -o  ${OBJDIR}moduledb.o;		\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} module_list.c            -c -o  ${OBJDIR}module_list.o;		\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} linux/module.c           -c -o  ${OBJDIR}module.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} linux/signal.c           -c -o  ${OBJDIR}signal.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} mips/arch.c              -c -o  ${OBJDIR}arch.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} synch.c                  -c -o  ${OBJDIR}synch.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} stats.c                  -c -o  ${OBJDIR}stats.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} monitor.c                -c -o  ${OBJDIR}monitor.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} link.c                   -c -o  ${OBJDIR}link.o;			\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} fragment.c               -c -o  ${OBJDIR}fragment.o;		\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} perscache.c              -c -o  ${OBJDIR}perscache.o;		\
	$(CC) ${C_FLAG} ${C_FLAG_DENTRE} ${D_FLAG} ${TOOL_FLAG} hotpatch.c               -c -o  ${OBJDIR}hotpatch.o;		\
															\
	cd ${OBJDIR};											 \
	${CC} ${C_FLAG} ${LINK_FLAG} -nostartfiles  -o  libpreload.so $(OBJ_PRELOAD);		\
	cd -;													

	
clean :
	-rm -f ${OBJDIR}*.o ${LIBDIR}*.so

distclean:
	-rm -rf *


