< envPaths

# Tell EPICS all about the record types, device-support modules, drivers,
# etc. in this build from dxpApp
dbLoadDatabase("$(KETEK)/dbd/ketekApp.dbd")
ketekApp_registerRecordDeviceDriver(pdbbase)

# Prefix for all records
epicsEnvSet("PREFIX", "KETEK1:")
# The port name for the detector
epicsEnvSet("PORT", "KETEK1")
# The port name for the underlying TCP port
epicsEnvSet("IP_PORT", "KETEK_IP")
# Ketek IP address
epicsEnvSet("KETEK_IP_ADDRESS", "gse-ketek1")
# Ketek IP port
epicsEnvSet("KETEK_IP_PORT", "3141")
# Ketek IP protocol (TCP or UDP)
epicsEnvSet("KETEK_IP_PROTOCOL", "UDP")
# The host IP address, used for sync mode UDP connection from Ketek
epicsEnvSet("HOST_IP_ADDRESS", "10.54.160.174")
#epicsEnvSet("HOST_IP_ADDRESS", "164.54.160.82")
# The host UDP port, used for sync mode UDP connection from Ketek
epicsEnvSet("HOST_UDP_PORT", "3142")

# The queue size for all plugins
epicsEnvSet("QSIZE",  "20")
# The maximum image width; used to set the maximum size for row profiles in the NDPluginStats plugin
epicsEnvSet("XSIZE",  "1024")
# The maximum image height; used to set the maximum size for column profiles in the NDPluginStats plugin
epicsEnvSet("YSIZE",  "1024")
# The maximum number of time series points in the NDPluginStats plugin
epicsEnvSet("NCHANS", "2048")
# The maximum number of frames buffered in the NDPluginCircularBuff plugin
epicsEnvSet("CBUFFS", "500")
# The maximum number of threads for plugins which can run in multiple threads
epicsEnvSet("MAX_THREADS", "8")
# The maximum number of channels in the MCA records
epicsEnvSet("MCA_CHANS", "4096")
# The maximum number of points in the ADC trace waveform records
epicsEnvSet("TRACE_LEN", "16384")
# The search path for database files
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")

drvAsynIPPortConfigure("$(IP_PORT)", "$(KETEK_IP_ADDRESS):$(KETEK_IP_PORT) $(KETEK_IP_PROTOCOL)", 0, 0, 0)
asynSetTraceIOMask($(IP_PORT), 0, HEX)
#asynSetTraceMask($(IP_PORT), 0, ERROR|DRIVER)

# KetekConfig(portName, ipPortName)
KetekConfig("$(PORT)", $(IP_PORT), $(HOST_IP_ADDRESS), $(HOST_UDP_PORT))

dbLoadRecords("$(KETEK)/db/ketek.template", "P=$(PREFIX),R=Ketek1:,M=mca1,PORT=$(PORT),TRACE_LEN=$(TRACE_LEN)")
dbLoadRecords("$(MCA)/db/mca.db", "P=$(PREFIX),M=mca1,DTYP=asynMCA,INP=@asyn($(PORT) 0),NCHAN=$(MCA_CHANS)")
dbLoadRecords("$(ASYN)/db/asynRecord.db", "P=$(PREFIX),R=Ketek1:AsynIO,PORT=$(PORT),ADDR=0,IMAX=256,OMAX=256")

save_restoreSet_IncompleteSetsOk(1)
save_restoreSet_DatedBackupFiles(1)
save_restoreSet_NumSeqFiles(3)
save_restoreSet_SeqPeriodInSeconds(300)
set_savefile_path(".", "autosave")
set_pass0_restoreFile("auto_settings.sav")
set_pass1_restoreFile("auto_settings.sav")
set_requestfile_path(".")
set_requestfile_path("$(KETEK)/db")
set_requestfile_path("$(MCA)/db")

asynSetTraceIOMask($(PORT), 0, ESCAPE)
#asynSetTraceMask($(PORT), 0, ERROR|DRIVER)


iocInit

### Start up the autosave task and tell it what to do.
# Save settings every thirty seconds
create_monitor_set("auto_settings.req", 30, "P=$(PREFIX), R=Ketek1:, M=mca1")

# Wait 1 seconds for all records with PINI=YES to process
epicsThreadSleep(1)
