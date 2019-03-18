##########################################################################
# File:             Mailefile 
# Version:          0.1 
# Author:           thienducee
# DATE              NAME                        DESCRIPTION
# 3-19-2017        Maobin                      Initial Version 1.0
##########################################################################
# started
include $(PROJ_HOME)/build/option.mak
OBJDIR=./
MODULE_NAME = $(PARAM)
LIB_RESULE=$(MODULE_NAME).a

#Two mothods#
#First mothod#
S1 := $(wildcard *.c)
SOURCE:=$(S1)

#Second mothod#
#SOURCE := 


LOCAL_INCLUDE := -I./

include $(PROJ_HOME)/build/app_build.mak

