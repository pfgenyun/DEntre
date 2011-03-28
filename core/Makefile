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

OBJ_DYNAMORIO = utils.o
SOURCE_DYNAMORIO = utils.c

CC = gcc
C_FLAG = -g -O0 -fPIC -shared 
LINK_FLAG = -ldl 

#D_FLAG = 

OBJDIR = ../build/
LIBDIR = ../build/

#$(OBJ) : $(SOURCE)
#	$(CC) ${C_FLAG}  $(SOURCE) -c

ALL:
	$(CC) ${C_FLAG} ${D_FLAG} ${TOOL_FLAG} linux/preload.c          -c -o  ${OBJDIR}preload.o;			\
	#$(CC) ${C_FLAG} ${D_FLAG} ${TOOL_FLAG} linux/os.c               -c -o  ${OBJDIR}os.o;				\
	#$(CC) ${C_FLAG} ${D_FLAG} ${TOOL_FLAG} utils.c                  -c -o  ${OBJDIR}utils.o;			\
	#$(CC) ${C_FLAG} ${D_FLAG} ${TOOL_FLAG} config.c                 -c -o  ${OBJDIR}config.o;			\
																										\
	cd ${OBJDIR};											 \
	#${CC} ${C_FLAG} ${LINK_FLAG} -nostartfiles  -o  libpreload.so $(OBJ_PRELOAD);		\
	cd -;													

	
clean :
	-rm -f ${OBJDIR}*.o

distclean:
	-rm -f ${LIBDIR}*.so ${OBJDIR}*.o


