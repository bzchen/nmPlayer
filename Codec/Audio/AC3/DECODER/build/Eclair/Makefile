#################################################################
#  	Visualon Makefile Template  		 	#
#       (Please Keep Below Options Order Unchanged)	#
#################################################################

# target type
# available: pc, v4(armv4), v5(armv5), v5x(armv5 xscale), v6(armv6), v7(cortex-a8 neon)
VOTT:= v6
 

#top root path
VOTOP:=../../../../../..


# module type
# please specify the type of your module: lib or exe
VOMT:= lib


# module macros
# please append the additional macro definitions here for your module if necessary. 
# e.g. -DVISUALON, macro VISUALON defined for your module 
ifeq ($(VOTT), v4)
VOMM:=-DARM -DLITEND -DHALFRATE #-DLCHECK -DKCAPABLE 
endif				

ifeq ($(VOTT), v5)
VOMM:=-DARM -DLITEND -DKCAPABLE -DHALFRATE -DLCHECK
endif

ifeq ($(VOTT), v6)
VOMM:=-DARM -DARMV6 -DLITEND -DKCAPABLE -DAC3D_ARM_OPT -DHALFRATE -DLCHECK #-DMS_CHANNEL_CONFIG #If Support MS Multichannel configure, enable the "MS_CHANNEL_CONFIG" MACRO
endif				

ifeq ($(VOTT), v7)
VOMM:=-DARM -DARMV6 -DARMV7 -DLITEND -DKCAPABLE -DAC3D_ARM_OPT -DHALFRATE -DLCHECK #-DMS_CHANNEL_CONFIG #If Support MS Multichannel configure, enable the "MS_CHANNEL_CONFIG" MACRO
endif	


# please specify the name of your module
VOTARGET:= libvoAC3Dec
VOMODVER?=3.0.0
VOSRCNO:=11711

# please modify here to be sure to see the g1.mk
include $(VOTOP)/build/eclair.mk 

# dependent libraries.
VODEPLIBS:=-ldl -lstdc++ -lcutils
ifeq ($(VOTT), v4)
VOSTCLIBS:=$(VOPREBUILTPATH)/libvoCheck.a
endif
ifeq ($(VOTT), v5)
VOSTCLIBS:=$(VOPREBUILTPATH)/libvoCheck.a
endif
ifeq ($(VOTT), v6)
VOSTCLIBS:=$(VOPREBUILTPATH)/libvoCheck.a
endif
ifeq ($(VOTT), v7)
VOSTCLIBS:=$(VOPREBUILTPATH)/libvoCheck.a
endif

# module source
# please modify here to be sure to see the ms.mk which specifies all source info of your module
include ../ms.mk


# please specify where is the$(VOTOP)/../voRelease on your PC, relative path is suggested
VORELDIR:=$(VOTOP)/../voRelease


# please modify here to be sure to see the doit.mk
include $(VOTOP)/build/doit.mk 

