TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================

DBD += ketekSupport.dbd
PROD_NAME = ketekApp

LIBRARY_IOC += ketek
PROD_IOC  += $(PROD_NAME)

LIB_SRCS += ketek.cpp

LIB_LIBS           += asyn 
LIB_LIBS           += ADBase
LIB_LIBS           += $(EPICS_BASE_IOC_LIBS)

DBD += $(PROD_NAME).dbd
$(PROD_NAME)_DBD += mcaSupport.dbd
$(PROD_NAME)_DBD += ketekSupport.dbd

## <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
$(PROD_NAME)_SRCS   += $(PROD_NAME)_registerRecordDeviceDriver.cpp
$(PROD_NAME)_SRCS   += $(PROD_NAME)Main.cpp
$(PROD_NAME)_LIBS   += mca
$(PROD_NAME)_LIBS   += ketek

include $(ADCORE)/ADApp/commonDriverMakefile
$(PROD_NAME)_DBD += drvAsynIPPort.dbd

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
