						#################################################################
						#  									Visualon Makefile Template  							 	#
						#																																#
						#						(Please Keep Below Options Order Unchanged)					#
						#################################################################

# target type
# available: pc, v4(armv4), v5(armv5), v5x(armv5 xscale), v6(armv6), v7(cortex-a8 neon)
VOTT:=pc
 

#top root path
VOTOP:=../../../../../..


VODBG=yes
# module type
# please specify the type of your module: lib or exe
VOMT:=lib


# module macros
# please append the additional macro definitions here for your module if necessary. 
# e.g. -DVISUALON, macro VISUALON defined for your module 
VOMM:= -DPD -D_X86_VER `pkg-config sdl --cflags` -D_RGB_OUTPUT -DPDX86 


# please specify the name of your module
VOTARGET:=voMMPlay


# please modify here to be sure to see the g1.mk
include $(VOTOP)/build/g1.mk 


# dependent libraries.
VODEPLIBS:=-lpthread `pkg-config sdl --libs`


# module source
# please modify here to be sure to see the ms.mk which specifies all source info of your module
include ../ms.mk


# please specify where is the$(VOTOP)/../voRelease on your PC, relative path is suggested
VORELDIR:=$(VOTOP)/../voRelease


# please modify here to be sure to see the doit.mk
include $(VOTOP)/build/doit.mk

