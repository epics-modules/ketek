/* ketek.cpp
 *
 * Driver for KETEK spectroscopy system
 *
 * Mark Rivers
 * University of Chicago
 *
 */

/* Standard includes... */
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <algorithm>
#include <math.h>
#include <osiSock.h>

/* EPICS includes */
#include <epicsString.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsExit.h>
#include <envDefs.h>
#include <iocsh.h>
#include <asynNDArrayDriver.h>
#include <drvAsynIPPort.h>
#include <asynOctetSyncIO.h>

/* MCA includes */
#include <drvMca.h>

#include <epicsExport.h>
#include "ketek.h"

#define DRIVER_VERSION       "1.0.0"
#define MIN_TRACE_TIME         0.016
#define TRACE_LEN_INC          16384

#define KETEK_FILTER_TIME_UNITS 12.5e-9
#define KETEK_USEC_TO_FILTER_UNITS (1.e-6/KETEK_FILTER_TIME_UNITS)
#define KETEK_ELAPSED_TIME_UNITS 10e-6
#define KETEK_EVENT_TIME_UNITS (1/80e6)
#define KETEK_USEC_TO_EVENT_UNITS (1e-6/KETEK_EVENT_TIME_UNITS)
#define KETEK_UDP_TIMEOUT 2
#define KETEK_UDP_PORT_NAME "KETEK_UDP_SYNC"

/** Only used for debugging/error messages to identify where the message comes from*/
static const char *driverName = "Ketek";

static void acquisitionTaskC(void *drvPvt)
{
    Ketek *pKetek = (Ketek *)drvPvt;
    pKetek->acquisitionTask();
}

static void c_shutdown(void* arg)
{
    Ketek *pKetek = (Ketek*)arg;
    pKetek->shutdown();
    //delete pKetek;
}

extern "C" int KetekConfig(const char *portName, const char *ipPortName, const char* hostIPAddress, int hostUDPPort)
{
    new Ketek(portName, ipPortName, hostIPAddress, hostUDPPort);
    return 0;
}

Ketek::Ketek(const char *portName, const char *ipPortName, const char* hostIPAddress, int hostUDPPort)
    : asynNDArrayDriver(portName, 1, 0, 0,
            asynInt32Mask | asynFloat64Mask | asynInt32ArrayMask | asynFloat64ArrayMask | asynOctetMask | asynDrvUserMask,
            asynInt32Mask | asynFloat64Mask | asynInt32ArrayMask | asynFloat64ArrayMask | asynOctetMask,
            ASYN_CANBLOCK, 1, 0, 0),
    uniqueId_(0),
    polling_(0)

{
    const char *functionName = "Ketek";
    asynStatus status;
    
    status = pasynOctetSyncIO->connect(ipPortName, 0, &pasynUserRemote_, 0);
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
          "%s::%s cannot connect to Ketek device, status=%d\n",
          driverName, functionName, status);
        return;
    }

    /* Internal asyn driver parameters */
    createParam(KetekErasedString,                 asynParamInt32,   &KetekErased);
    createParam(KetekReadStatusString,             asynParamInt32,   &KetekReadStatus);

    /* Diagnostic trace parameters */
    createParam(KetekEventTriggerSourceString,     asynParamInt32,        &KetekEventTriggerSource);
    createParam(KetekEventTriggerValueString,      asynParamInt32,        &KetekEventTriggerValue);
    createParam(KetekScopeDataSourceString,        asynParamInt32,        &KetekScopeDataSource);
    createParam(KetekScopeDataString,              asynParamInt32Array,   &KetekScopeData);
    createParam(KetekScopeTimeArrayString,         asynParamFloat64Array, &KetekScopeTimeArray);
    createParam(KetekScopeIntervalString,          asynParamFloat64,      &KetekScopeInterval);
    createParam(KetekScopeTriggerTimeoutString,    asynParamInt32,        &KetekScopeTriggerTimeout);
    createParam(KetekScopeReadString,              asynParamInt32,        &KetekScopeRead);

    /* Runtime statistics */
    createParam(KetekInputCountRateString,         asynParamFloat64, &KetekInputCountRate);
    createParam(KetekOutputCountRateString,        asynParamFloat64, &KetekOutputCountRate);
    createParam(KetekInputCountsString,            asynParamInt32,   &KetekInputCounts);
    createParam(KetekOutputCountsString,           asynParamInt32,   &KetekOutputCounts);
    createParam(KetekFastLiveTimeString,           asynParamFloat64, &KetekFastLiveTime);

    /* Preset parameters */
    createParam(KetekPresetInputCountsString,      asynParamInt32,   &KetekPresetInputCounts);
    createParam(KetekPresetOutputCountsString,     asynParamInt32,   &KetekPresetOutputCounts);
    createParam(KetekPresetModeString,             asynParamInt32,   &KetekPresetMode);

    /* Configuration parameters */
    createParam(KetekFastPeakingTimeString,             asynParamFloat64, &KetekFastPeakingTime);
    createParam(KetekFastGapTimeString,                 asynParamFloat64, &KetekFastGapTime);
    createParam(KetekMediumPeakingTimeString,           asynParamFloat64, &KetekMediumPeakingTime);
    createParam(KetekMediumGapTimeString,               asynParamFloat64, &KetekMediumGapTime);
    createParam(KetekSlowPeakingTimeString,             asynParamFloat64, &KetekSlowPeakingTime);
    createParam(KetekSlowGapTimeString,                 asynParamFloat64, &KetekSlowGapTime);
    createParam(KetekFastFilterThresholdString,         asynParamFloat64, &KetekFastFilterThreshold);
    createParam(KetekMediumFilterThresholdString,       asynParamFloat64, &KetekMediumFilterThreshold);
    createParam(KetekMediumFilterEnableString,          asynParamInt32,   &KetekMediumFilterEnable);
    createParam(KetekFastFilterMaxWidthString,          asynParamFloat64, &KetekFastFilterMaxWidth);
    createParam(KetekMediumFilterMaxWidthString,        asynParamFloat64, &KetekMediumFilterMaxWidth);
    createParam(KetekResetInhibitTimeString,            asynParamFloat64, &KetekResetInhibitTime);
    createParam(KetekBaselineAverageLenString,          asynParamInt32,   &KetekBaselineAverageLen);
    createParam(KetekBaselineTrimString,                asynParamInt32,   &KetekBaselineTrim);
    createParam(KetekBaselineCorrEnableString,          asynParamInt32,   &KetekBaselineCorrEnable);
    createParam(KetekEnergyGainString,                  asynParamFloat64, &KetekEnergyGain);
    createParam(KetekEnergyOffsetString,                asynParamFloat64, &KetekEnergyOffset);
    createParam(KetekDynResetEnableString,              asynParamInt32,   &KetekDynResetEnable);
    createParam(KetekDynResetThresholdString,           asynParamInt32,   &KetekDynResetThreshold);
    createParam(KetekDynResetDurationString,            asynParamFloat64, &KetekDynResetDuration);
    createParam(KetekBoardTemperatureString,            asynParamFloat64, &KetekBoardTemperature);
    createParam(KetekMCUReadyString,                    asynParamInt32,   &KetekMCUReady);
    createParam(KetekBytesPerBinString,                 asynParamInt32,   &KetekBytesPerBin);

    /* Sync parameters */
    createParam(KetekSyncAcquireString,                 asynParamInt32,   &KetekSyncAcquire);
    createParam(KetekSyncCycleTimeString,               asynParamFloat64, &KetekSyncCycleTime);
    createParam(KetekSyncPointsString,                  asynParamInt32,   &KetekSyncPoints);
    createParam(KetekSyncCurrentPointString,            asynParamInt32,   &KetekSyncCurrentPoint);
    createParam(KetekSyncEnabledString,                 asynParamInt32,   &KetekSyncEnabled);
    createParam(KetekSyncRunningString,                 asynParamInt32,   &KetekSyncRunning);

    /* Commands from MCA interface */
    createParam(mcaDataString,                     asynParamInt32Array, &mcaData);
    createParam(mcaStartAcquireString,             asynParamInt32,   &mcaStartAcquire);
    createParam(mcaStopAcquireString,              asynParamInt32,   &mcaStopAcquire);
    createParam(mcaEraseString,                    asynParamInt32,   &mcaErase);
    createParam(mcaReadStatusString,               asynParamInt32,   &mcaReadStatus);
    createParam(mcaChannelAdvanceSourceString,     asynParamInt32,   &mcaChannelAdvanceSource);
    createParam(mcaNumChannelsString,              asynParamInt32,   &mcaNumChannels);
    createParam(mcaAcquireModeString,              asynParamInt32,   &mcaAcquireMode);
    createParam(mcaSequenceString,                 asynParamInt32,   &mcaSequence);
    createParam(mcaPrescaleString,                 asynParamInt32,   &mcaPrescale);
    createParam(mcaPresetSweepsString,             asynParamInt32,   &mcaPresetSweeps);
    createParam(mcaPresetLowChannelString,         asynParamInt32,   &mcaPresetLowChannel);
    createParam(mcaPresetHighChannelString,        asynParamInt32,   &mcaPresetHighChannel);
    createParam(mcaDwellTimeString,                asynParamFloat64, &mcaDwellTime);
    createParam(mcaPresetLiveTimeString,           asynParamFloat64, &mcaPresetLiveTime);
    createParam(mcaPresetRealTimeString,           asynParamFloat64, &mcaPresetRealTime);
    createParam(mcaPresetCountsString,             asynParamFloat64, &mcaPresetCounts);
    createParam(mcaAcquiringString,                asynParamInt32,   &mcaAcquiring);
    createParam(mcaElapsedLiveTimeString,          asynParamFloat64, &mcaElapsedLiveTime);
    createParam(mcaElapsedRealTimeString,          asynParamFloat64, &mcaElapsedRealTime);
    createParam(mcaElapsedCountsString,            asynParamFloat64, &mcaElapsedCounts);

    int syncEnabled = 0;
    if (hostIPAddress && strlen(hostIPAddress) > 0) {
        int ip1, ip2, ip3, ip4;
        sscanf(hostIPAddress, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);
        writeSingleParam(pSyncEtherHigh, (ip1*256 + ip2));
        writeSingleParam(pSyncEtherLow,  (ip3*256 + ip4));
        writeSingleParam(pSyncEtherPort, hostUDPPort);
        char portString[256];
        sprintf(portString, "%s:%d:%d UDP", hostIPAddress, hostUDPPort, hostUDPPort);
        status = (asynStatus)drvAsynIPPortConfigure(KETEK_UDP_PORT_NAME, portString, 0, 0, 1);
        if (status) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s::%s cannot create local UDP port, status=%d\n",
              driverName, functionName, status);
            return;
        }
        status = pasynOctetSyncIO->connect(KETEK_UDP_PORT_NAME, 0, &pasynUserSync_, 0);
        if (status) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s::%s cannot connect to local UDP port, status=%d\n",
              driverName, functionName, status);
            return;
        }
        syncEnabled = 1;
        
    }
    setIntegerParam(KetekSyncEnabled, syncEnabled);
    setIntegerParam(KetekBaselineAverageLen, 8);
    setIntegerParam(KetekBytesPerBin, 3);
    setIntegerParam(NDArrayCallbacks, 1);

    setStringParam(ADManufacturer, "KETEK");
    setStringParam(ADModel, "DPP3 based system");
    setStringParam(NDDriverVersion, DRIVER_VERSION);
    epicsUInt16 fwMajor, fwMinor, fwPatch;
    status = readSingleParam(pFirmwareMajor, &fwMajor);
    status = readSingleParam(pFirmwareMinor, &fwMinor);
    status = readSingleParam(pFirmwarePatch, &fwPatch);
    char firmwareString[32];
    sprintf(firmwareString, "%d.%d.%d", fwMajor, fwMinor, fwPatch);
    setStringParam(ADFirmwareVersion, firmwareString);

    
    /* Register the epics exit function to be called when the IOC exits... */
    epicsAtExit(c_shutdown, this);

    /* Set the parameters in param lib */
    setIntegerParam(mcaAcquiring, 0);

    /* Create the syncStartEvent used to signal our acquisitionTask to read sync mode data */
    syncStartEvent_ = new epicsEvent();

    polling_ = true;
    status = (epicsThreadCreate("acquisitionTask",
              epicsThreadPriorityMedium,
              epicsThreadGetStackSize(epicsThreadStackMedium),
              (EPICSTHREADFUNC)acquisitionTaskC, this) == NULL) ? asynError:asynSuccess;
    if (status) {
        printf("%s:%s epicsThreadCreate failure\n",
                driverName, functionName);
        return;
    }

    // Stop acquisition in case it is running, since we can't change settings if it is
    this->stopAcquiring();
    /* Read the statistics and data once */
    this->getAcquisitionStatistics();
    getMcaData();
}

asynStatus Ketek::readSingleParam(int paramID, unsigned short *value)
{
    asynStatus status;
    size_t nWrite, nRead;
    int eomReason;
    ketekRequest_t req = {0};
    ketekResponse_t resp = {0};
    static const char *functionName = "readSingleParam";
    
    req.paramID = paramID;
    req.command = KETEK_COMMAND_READ;
        status = pasynOctetSyncIO->writeRead(pasynUserRemote_, (const char *)&req, sizeof(req), 
                                             (char *)&resp, sizeof(resp), 
                                             KETEK_UDP_TIMEOUT, &nWrite, &nRead, &eomReason);
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s paramID=%d, error in writeRead status=%d\n",
                  driverName, functionName, paramID, status);
        return status;
    }
    if (resp.status != 0) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s paramID=%d, device returned error status=%d\n",
                  driverName, functionName, paramID, status);
        return status;
    }
    *value = ntohs(resp.data);
    return asynSuccess;
}

asynStatus Ketek::writeSingleParam(int paramID, int value)
{
    asynStatus status;
    size_t nWrite, nRead;
    epicsUInt16 sval;
    int eomReason;
    ketekRequest_t req;
    ketekResponse_t resp;
    static const char *functionName = "writeSingleParam";
 
    req.paramID = paramID;
    req.command = KETEK_COMMAND_WRITE;
    sval = (epicsUInt16)value;
    req.data = htons(sval);
    status = pasynOctetSyncIO->writeRead(pasynUserRemote_, (const char *)&req, sizeof(req), 
                                         (char *)&resp, sizeof(resp), 
                                         KETEK_UDP_TIMEOUT, &nWrite, &nRead, &eomReason);
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s paramID=%d, error in writeRead, status=%d\n",
                  driverName, functionName, paramID, status);
        return status;
    }
    if (resp.status != 0) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s paramID=%d, value=%d, device returned error status=%d\n",
                  driverName, functionName, paramID, value, resp.status);
        return status;
     }
     return asynSuccess;
}

asynStatus Ketek::writeStopValue(epicsUInt32 value)
{
    asynStatus status;
    //static const char *functionName = "writeStopValue";
    
    status = writeSingleParam(pStopValueLow, value & 0x0000FFFF);
    status = writeSingleParam(pStopValueHigh, (value & 0xFFFF0000) >> 16);
    return status;
}

asynStatus Ketek::sendRcvMsg(ketekRequest_t *request, void *response, size_t responseSize, double timeout)
{
    size_t nWrite, nRead;
    int eomReason;
    asynStatus status;
    static const char *functionName = "sendRcvMsg";

    status = pasynOctetSyncIO->writeRead(pasynUserRemote_, (const char *)request, sizeof(*request), 
                                        (char *)response, responseSize, 
                                        timeout, &nWrite, &nRead, &eomReason);
    if (status || (nRead != responseSize)) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s error in writeRead, status=%d, nWrite=%d, nRequested=%d, nRead=%d, eomReason=%d, timeout=%f\n",
                  driverName, functionName, status, (int)nWrite, (int)responseSize, (int)nRead, eomReason, timeout);
        ketekResponse_t *resp = (ketekResponse_t *)response;
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s paramID=%d, status=%d, data=%d\n",
                  driverName, functionName, resp->paramID, resp->status, ntohs(resp->data));
        
    }
    return status;
}
/* virtual methods to override from asynNDArrayDriver */
asynStatus Ketek::writeInt32( asynUser *pasynUser, epicsInt32 value)
{
    asynStatus status = asynSuccess;
    int function = pasynUser->reason;
    int acquiring;
    //const char* functionName = "writeInt32";

    /* Set the parameter and readback in the parameter library.  This may be overwritten later but that's OK */
    status = setIntegerParam(function, value);

    if (function == mcaStartAcquire) {
        status = this->startAcquiring();
    }
    else if (function == mcaStopAcquire) {
        status = this->stopAcquiring();
    }
    else if (function == mcaErase) {
        setIntegerParam(KetekErased, 1);
    }
    else if (function == mcaReadStatus) {
        getIntegerParam(mcaAcquiring, &acquiring);
        /* If we are acquiring then read the statistics, else we use the cached values */
        if (acquiring) status = this->getAcquisitionStatistics();
    }
    else if (function == KetekReadStatus) {
        epicsUInt16 iValue;
        readSingleParam(pBoardTemperature, &iValue);
        setDoubleParam(KetekBoardTemperature, iValue/16.0 - 273.15);
        readSingleParam(pMCUStatus, &iValue);
        setIntegerParam(KetekMCUReady, (iValue & 2) ? 1 : 0);
    }
    else if (function == KetekSyncAcquire) {
        if (value) {
            this->startSyncAcquire();
        } else {
            this->stopSyncAcquire();
        }
    }
    else if (function == KetekScopeRead) {
        status = this->getScopeTrace();
    }
    else if ((function == KetekMediumFilterEnable) ||
             (function == KetekBaselineAverageLen) ||
             (function == KetekBaselineTrim)       ||
             (function == KetekBaselineCorrEnable) ||
             (function == KetekDynResetEnable)     ||
             (function == mcaNumChannels)          ||
             (function == KetekDynResetThreshold) ||
             (function == KetekBytesPerBin)) {
        this->setConfiguration();
    }
    else if ((function == KetekPresetMode) ||
             (function == KetekPresetInputCounts) ||
             (function == KetekPresetOutputCounts)) {
        this->setPresets();
    }
    else if ((function == KetekEventTriggerSource) ||
             (function == KetekEventTriggerValue)  ||
             (function == KetekScopeTriggerTimeout)) {
        this->setEventScope();
    }

    /* Call the callbacks */
    callParamCallbacks();
    return status;
}

asynStatus Ketek::writeFloat64( asynUser *pasynUser, epicsFloat64 value)
{
    asynStatus status = asynSuccess;
    int function = pasynUser->reason;
    //const char *functionName = "writeFloat64";

    /* Set the parameter and readback in the parameter library.  This may be overwritten later but that's OK */
    status = setDoubleParam(function, value);

    if ((function == KetekFastPeakingTime) ||
        (function == KetekSlowPeakingTime) ||
        (function == KetekFastFilterThreshold) ||
        (function == KetekMediumFilterThreshold) ||
        (function == KetekFastFilterMaxWidth) ||
        (function == KetekMediumFilterMaxWidth) ||
        (function == KetekResetInhibitTime) ||
        (function == KetekEnergyGain) ||
        (function == KetekEnergyOffset) ||
        (function == KetekDynResetDuration))
    {
        this->setConfiguration();
    }
    else if ((function == mcaPresetRealTime) ||
             (function == mcaPresetLiveTime)) {
        this->setPresets();
    }
    else if (function == KetekScopeInterval) {
        this->setEventScope();
    }

    /* Call the callback */
    callParamCallbacks();

    return status;
}

asynStatus Ketek::readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn)
{
    asynStatus status = asynSuccess;
    int function = pasynUser->reason;
    int nBins;
    int i;
    int syncRunning;
    const char *functionName = "readInt32Array";

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
        "%s::%s function=%d\n",
        driverName, functionName, function);
    if (function == mcaData) {
        getIntegerParam(mcaNumChannels, &nBins);
        if (nBins > (int)nElements) nBins = (int)nElements;
        *nIn = nBins;
        getIntegerParam(KetekSyncRunning, &syncRunning);
        // If a sync mode run is active then mcaData_ will be updated in acquisitionTask.
        if (!syncRunning) {
            this->getMcaData();
        }
        for (i=0; i<nBins; i++) {
            value[i] = mcaData_[i];
        }
    }
    else {
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
            "%s::%s Function not implemented: [%d]\n",
            driverName, functionName, function);
        status = asynError;
    }
    return status;
}

asynStatus Ketek::setPresets()
{
    double presetReal, presetLive;
    int presetInputCounts, presetOutputCounts;
    int presetMode;
    //static const char *functionName = "SetPresets";

    getIntegerParam(KetekPresetMode,         &presetMode);
    getDoubleParam( mcaPresetRealTime,       &presetReal);
    getDoubleParam( mcaPresetLiveTime,       &presetLive);
    getIntegerParam(KetekPresetInputCounts,  &presetInputCounts);
    getIntegerParam(KetekPresetOutputCounts, &presetOutputCounts);
    getIntegerParam(KetekPresetMode,         &presetMode);

    switch(presetMode) {
        case KetekPresetRealTime:
            writeStopValue(presetReal / KETEK_ELAPSED_TIME_UNITS);
            break;
        case KetekPresetLiveTime:
            writeStopValue(presetLive / KETEK_ELAPSED_TIME_UNITS);
            break;
        case KetekPresetInputCts:
            writeStopValue(presetInputCounts);
            break;
        case KetekPresetOutputCts:
            writeStopValue(presetOutputCounts);
            break;
    }
    writeSingleParam(pStopCondition, presetMode);
    return asynSuccess;
}

asynStatus Ketek::setConfiguration()
{
    double dValue;
    int iValue;
    int i;
    //static const char *functionName = "setConfiguration";

    getDoubleParam(KetekFastPeakingTime, &dValue);
    iValue = round(dValue * KETEK_USEC_TO_FILTER_UNITS);
    if (iValue < 2) iValue = 2;
    if (iValue > 40) iValue = 40;
    writeSingleParam(pFastPeakingTime, iValue);

    getDoubleParam(KetekSlowPeakingTime, &dValue);
    iValue = round(dValue * KETEK_USEC_TO_FILTER_UNITS);
    if (iValue <= 3) iValue = 2;
    else if (iValue <= 5) iValue = 4;
    else if (iValue <= 7) iValue = 6;
    else if (iValue <= 10) iValue = 8;
    else if (iValue <= 14) iValue = 12;
    else if (iValue <= 16) iValue = 16;
    else iValue = (iValue + 8)/16 * 16;
    if (iValue > 1008) iValue = 1008;
    writeSingleParam(pSlowPeakingTime, iValue);

    getDoubleParam(KetekSlowGapTime, &dValue);
    iValue = round(dValue * KETEK_USEC_TO_FILTER_UNITS);
    if (iValue < 2) iValue = 2;
    if (iValue > 127) iValue = 127;
    writeSingleParam(pSlowGapTime, iValue);

    getDoubleParam(KetekFastFilterThreshold, &dValue);
    iValue = round(dValue);
    if (iValue < 0) iValue = 0;
    if (iValue > 16384) iValue = 16384;
    writeSingleParam(pFastTrigThreshold, iValue);

    getDoubleParam(KetekMediumFilterThreshold, &dValue);
    iValue = round(dValue);
    if (iValue < 0) iValue = 0;
    if (iValue > 16384) iValue = 16384;
    writeSingleParam(pMediumTrigThreshold, iValue);

    getIntegerParam(KetekMediumFilterEnable, &iValue);
    writeSingleParam(pMediumTriggerEnable, iValue);

    getDoubleParam(KetekFastFilterMaxWidth, &dValue);
    iValue = round(dValue * KETEK_USEC_TO_FILTER_UNITS);
    writeSingleParam(pFastMaxWidth, iValue);

    getDoubleParam(KetekMediumFilterMaxWidth, &dValue);
    iValue = round(dValue * KETEK_USEC_TO_FILTER_UNITS);
    writeSingleParam(pMediumMaxWidth, iValue);

    getDoubleParam(KetekResetInhibitTime, &dValue);
    iValue = round(dValue * KETEK_USEC_TO_FILTER_UNITS);
    writeSingleParam(pResetInhibitTime, iValue);

    getIntegerParam(KetekBaselineAverageLen, &iValue);
    writeSingleParam(pBaselineAverageLen, iValue);

    getIntegerParam(KetekBaselineTrim, &iValue);
    writeSingleParam(pBaselineTrim, iValue);

    getIntegerParam(KetekBaselineCorrEnable, &iValue);
    writeSingleParam(pBaselineCorrEnable, iValue);

    getDoubleParam(KetekEnergyGain, &dValue);
    iValue = round(dValue * 16383/100.);  // Convert from % to value
    writeSingleParam(pDigitalEnergyGain, iValue);

    getDoubleParam(KetekEnergyOffset, &dValue);
    iValue = round(dValue/256*32768 + 32768);
    writeSingleParam(pDigitalEnergyOffset, iValue);

    getIntegerParam(mcaNumChannels, &iValue);
    for (i=9; i<13; i++) {
        if (1<<i == iValue) break;
    }
    writeSingleParam(pMCANumBins, i);
    
    getIntegerParam(KetekBytesPerBin, &iValue);
    writeSingleParam(pMCABytesPerBin, iValue);
    
    getIntegerParam(KetekDynResetEnable, &iValue);
    writeSingleParam(pDynResetEnable, iValue);

    getIntegerParam(KetekDynResetThreshold, &iValue);
    writeSingleParam(pDynResetThreshold, iValue);

    getDoubleParam(KetekDynResetDuration, &dValue);
    iValue = round(dValue * KETEK_USEC_TO_FILTER_UNITS);
    writeSingleParam(pDynResetDuration, iValue);

    getConfiguration();

    return asynSuccess;
}

asynStatus Ketek::getConfiguration()
{
    double dValue;
    epicsUInt16 iValue;
    //static const char *functionName = "getConfiguration";

    readSingleParam(pFastPeakingTime, &iValue);
    dValue = iValue / KETEK_USEC_TO_FILTER_UNITS;
    setDoubleParam(KetekFastPeakingTime, dValue);

    readSingleParam(pFastGapTime, &iValue);
    dValue = iValue / KETEK_USEC_TO_FILTER_UNITS;
    setDoubleParam(KetekFastGapTime, dValue);

    readSingleParam(pFastTrigThreshold, &iValue);
    setDoubleParam(KetekFastFilterThreshold, (double)iValue);

    readSingleParam(pFastMaxWidth, &iValue);
    dValue = iValue / KETEK_USEC_TO_FILTER_UNITS;
    setDoubleParam(KetekFastFilterMaxWidth, dValue);

    readSingleParam(pMediumPeakingTime, &iValue);
    dValue = iValue / KETEK_USEC_TO_FILTER_UNITS;
    setDoubleParam(KetekMediumPeakingTime, dValue);

    readSingleParam(pMediumGapTime, &iValue);
    dValue = iValue / KETEK_USEC_TO_FILTER_UNITS;
    setDoubleParam(KetekMediumGapTime, dValue);

    readSingleParam(pMediumTrigThreshold, &iValue);
    setDoubleParam(KetekMediumFilterThreshold, (double)iValue);

    readSingleParam(pMediumMaxWidth, &iValue);
    dValue = iValue / KETEK_USEC_TO_FILTER_UNITS;
    setDoubleParam(KetekMediumFilterMaxWidth, dValue);

    readSingleParam(pMediumTriggerEnable, &iValue);
    setIntegerParam(KetekMediumFilterEnable, iValue);

    readSingleParam(pSlowPeakingTime, &iValue);
    dValue = iValue / KETEK_USEC_TO_FILTER_UNITS;
    setDoubleParam(KetekSlowPeakingTime, dValue);

    readSingleParam(pSlowGapTime, &iValue);
    dValue = iValue / KETEK_USEC_TO_FILTER_UNITS;
    setDoubleParam(KetekSlowGapTime, dValue);

    readSingleParam(pResetInhibitTime, &iValue);
    dValue = iValue / KETEK_USEC_TO_FILTER_UNITS;
    setDoubleParam(KetekResetInhibitTime, dValue);

    readSingleParam(pBaselineAverageLen, &iValue);
    setIntegerParam(KetekBaselineAverageLen, iValue);

    readSingleParam(pBaselineTrim, &iValue);
    setIntegerParam(KetekBaselineTrim, iValue);

    readSingleParam(pBaselineCorrEnable, &iValue);
    setIntegerParam(KetekBaselineCorrEnable, iValue);

    readSingleParam(pDigitalEnergyGain, &iValue);
    dValue = iValue * 100. / 16383.;  // Convert from value to %
    setDoubleParam(KetekEnergyGain, dValue);

    readSingleParam(pDigitalEnergyOffset, &iValue);
    dValue = (iValue - 32768) * 256. / 32768.;  // Convert from value to %
    setDoubleParam(KetekEnergyOffset, dValue);

    readSingleParam(pMCANumBins, &iValue);
    setIntegerParam(mcaNumChannels, 1<<iValue);

    readSingleParam(pMCABytesPerBin, &iValue);
    setIntegerParam(KetekBytesPerBin, iValue);
    
    readSingleParam(pDynResetEnable, &iValue);
    setIntegerParam(KetekDynResetEnable, iValue);

    readSingleParam(pDynResetThreshold, &iValue);
    setIntegerParam(KetekDynResetThreshold, iValue);

    readSingleParam(pDynResetDuration, &iValue);
    dValue = iValue / KETEK_USEC_TO_FILTER_UNITS;
    setDoubleParam(KetekDynResetDuration, dValue);

    return asynSuccess;
}

asynStatus Ketek::setEventScope()
{
    int triggerSource, triggerValue, scopeTriggerTimeout;
    double scopeInterval;
    int iValue;
    //static const char *functionName = "setEventScope";

    getIntegerParam(KetekEventTriggerSource,  &triggerSource);
    getIntegerParam(KetekEventTriggerValue,   &triggerValue);
    getIntegerParam(KetekScopeTriggerTimeout, &scopeTriggerTimeout);
    getDoubleParam(KetekScopeInterval,        &scopeInterval);

    writeSingleParam(pEventTriggerSource,  triggerSource);
    writeSingleParam(pEventTriggerValue,   triggerValue);
    writeSingleParam(pScopeTriggerTimeout, scopeTriggerTimeout);
    iValue = round(scopeInterval*KETEK_USEC_TO_EVENT_UNITS);
    if (iValue < 1) iValue = 1;
    if (iValue > 65535) iValue = 65535;
    scopeInterval = iValue / KETEK_USEC_TO_EVENT_UNITS;
    writeSingleParam(pScopeSampInterval, iValue);
    setDoubleParam(KetekScopeInterval, scopeInterval);
    for (int i=0; i<KETEK_MAX_SCOPE_POINTS; i++) {
        scopeTimeBuffer_[i] = i*scopeInterval;
    }
    doCallbacksFloat64Array(scopeTimeBuffer_, KETEK_MAX_SCOPE_POINTS, KetekScopeTimeArray, 0);

    return asynSuccess;
}

asynStatus Ketek::getAcquisitionStatistics()
{
    double realTime, fastLiveTime, slowLiveTime, icr, ocr;
    int inputCounts, outputCounts;
    int erased;
    int active;
    struct statsItem {
       epicsUInt8 paramId;
       epicsUInt8 status;
       epicsUInt16 data;
    };
    ketekRequest_t req;
    statsItem allStats[13];
    const char *functionName = "getAcquisitionStatistics";

    getIntegerParam(KetekErased, &erased);
    if (erased) {
        setDoubleParam(mcaElapsedRealTime,    0);
        setDoubleParam(mcaElapsedLiveTime,    0);
        setDoubleParam(KetekInputCountRate,   0);
        setDoubleParam(KetekOutputCountRate,  0);
        setIntegerParam(KetekInputCounts,     0);
        setIntegerParam(KetekOutputCounts,    0);
    } else {
        req.paramID = pRunStatistics;
        req.command = KETEK_COMMAND_READ;
        sendRcvMsg(&req, (char *)allStats, sizeof(allStats), 1.0);  // 1 second timeout should be fine
        active       = ntohs(allStats[0].data);
        realTime     = (ntohs(allStats[1].data)  + ntohs(allStats[2].data)*65536.)*KETEK_ELAPSED_TIME_UNITS;
        fastLiveTime = (ntohs(allStats[3].data)  + ntohs(allStats[4].data)*65536.)*KETEK_ELAPSED_TIME_UNITS;
        outputCounts = (ntohs(allStats[5].data)  + ntohs(allStats[6].data)*65536.);
        inputCounts  = (ntohs(allStats[7].data)  + ntohs(allStats[8].data)*65536.);
        ocr          = (ntohs(allStats[9].data)  + ntohs(allStats[10].data)*65536.);
        icr          = (ntohs(allStats[11].data) + ntohs(allStats[12].data)*65536.);

        slowLiveTime = (realTime * outputCounts)/inputCounts;
        setIntegerParam(mcaAcquiring,         active);
        setDoubleParam( mcaElapsedRealTime,   realTime); 
        setDoubleParam( mcaElapsedLiveTime,   slowLiveTime);
        setIntegerParam(KetekOutputCounts,    outputCounts);
        setIntegerParam(KetekInputCounts,     inputCounts);
        setDoubleParam( KetekOutputCountRate, ocr);
        setDoubleParam( KetekInputCountRate,  icr);
        setDoubleParam( KetekFastLiveTime,    fastLiveTime);


        asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
            "%s::%s\n"
            "              active=%d\n"
            "           real time=%f\n"
            "      fast live time=%f\n"
            "      slow live time=%f\n"
            "       output counts=%d\n"
            "        input counts=%d\n"
            "    input count rate=%f\n"
            "   output count rate=%f\n",
            driverName, functionName,
            active,
            realTime,
            fastLiveTime,
            slowLiveTime,
            outputCounts,
            inputCounts,
            ocr,
            icr);

    }
    return(asynSuccess);
}

asynStatus Ketek::getMcaData()
{
    asynStatus status = asynSuccess;
    epicsUInt16 iTemp;
    epicsUInt16 bytesPerBin;
    int numBins;
    ketekRequest_t req;
    epicsTimeStamp now;
    //const char* functionName = "getMcaData";

    epicsTimeGetCurrent(&now);

    /* Read the MCA spectrum */

    readSingleParam(pMCANumBins, &iTemp);
    numBins = 1 << iTemp;
    readSingleParam(pMCABytesPerBin, &bytesPerBin);
    req.paramID = pMCARead;
    req.command = KETEK_COMMAND_READ;
    req.data = 0;
    status = sendRcvMsg(&req, (char *)mcaRaw_, numBins*bytesPerBin, 1.0); // 1 second timeout is enough
    for (int i=0; i<numBins; i++) {
        switch (bytesPerBin) {
            case 1:
                mcaData_[i] = mcaRaw_[i];
                break;
            case 2:
                mcaData_[i] = mcaRaw_[i*2] + mcaRaw_[i*2+1]*256L;
                break;
            case 3:
                mcaData_[i] = mcaRaw_[i*3] + mcaRaw_[i*3+1]*256L + mcaRaw_[i*3+2]*65536L;
                break;
        }
    }
    return status;
}


/* Get trace data */
asynStatus Ketek::getScopeTrace()
{
    int scopeDataSource;
    ketekRequest_t req;
    
    getIntegerParam(KetekScopeDataSource, &scopeDataSource);
    req.paramID = pEventScopeGet;
    req.command = KETEK_COMMAND_WRITE;
    req.data = htons(scopeDataSource);
    sendRcvMsg(&req, (char *)scopeDataRaw_, sizeof(scopeDataRaw_), 10.);  // Need 10 second timeout for long interval times
    for (int i=0; i<KETEK_MAX_SCOPE_POINTS; i++) {
       scopeData_[i] = scopeDataRaw_[i*3] + scopeDataRaw_[i*3+1]*256L + scopeDataRaw_[i*3+2]*65536L;
    }
    doCallbacksInt32Array(scopeData_, KETEK_MAX_SCOPE_POINTS, KetekScopeData, 0);
    return asynSuccess;
}

asynStatus Ketek::startAcquiring()
{
    asynStatus status = asynSuccess;
    int acquiring, erased;
    const char *functionName = "startAcquiring";

    getIntegerParam(mcaAcquiring, &acquiring);
    getIntegerParam(KetekErased, &erased);
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
        "%s::%s acquiring=%d, erased=%d\n",
        driverName, functionName, acquiring, erased);
    /* if already acquiring we just ignore and return */
    if (acquiring) return status;

    writeSingleParam(pRunStart, erased ? 0 : 1);   
    setIntegerParam(KetekErased, 0); /* reset the erased flag */
    setIntegerParam(mcaAcquiring, 1); /* Set the acquiring flag */

    callParamCallbacks();
    return status;
}


asynStatus Ketek::stopAcquiring() {
    asynStatus status;
    status = writeSingleParam(pRunStop, 0);
    return status;
}

asynStatus Ketek::startSyncAcquire()
{
    asynStatus status = asynSuccess;
    double syncCycleTime;
    //int syncPoints;
    //const char *functionName = "startSyncAcquire";

    // Sync cycle time
    getDoubleParam(KetekSyncCycleTime, &syncCycleTime);
    int itemp = round(syncCycleTime / KETEK_ELAPSED_TIME_UNITS);
    status = writeSingleParam(pSyncCycleTimeLow, itemp & 0x0000FFFF);
    status = writeSingleParam(pSyncCycleTimeHigh, (itemp & 0xFFFF0000) >> 16);

    // Number of sync points
/* I can't get this to work, acquisition stops but reads then timeout and I cannot restart sync acquisition.
    getIntegerParam(KetekSyncPoints, &syncPoints);
    status = writeSingleParam(pSyncStopConditionLow, syncPoints & 0x0000FFFF);
    status = writeSingleParam(pSyncStopConditionHigh, (syncPoints & 0xFFFF0000) >> 16);
*/

    // Sync status data to be sent.  Enable all for now.
    status = writeSingleParam(pSyncStatusData, 0x3F);

    // Sync runtime data to be sent.  Enable all for now.
    status = writeSingleParam(pSyncRuntimeData, 0x7FF);
    
    // Sync spectrum data to be sent.  Enable.
    status = writeSingleParam(pSyncSpectrumData, 1);

    // Sync mode.  Only Mapping Mode supported for now
    status = writeSingleParam(pSyncMode, 4);
    
    // Flush any stale UDP data
    pasynOctetSyncIO->flush(pasynUserSync_);
    
    // Set the current point
    setIntegerParam(KetekSyncCurrentPoint, 0);

    // Reset counters and errors
    status = writeSingleParam(pSyncReset, 1);
    // Start sync run
    status = writeSingleParam(pSyncStart, 1);

    callParamCallbacks();

    // signal cmdStartEvent to start the polling thread
    syncStartEvent_->signal();
    return status;
}


asynStatus Ketek::stopSyncAcquire() {
    asynStatus status;
    status = writeSingleParam(pSyncStop, 0);
    return status;
}

/** Thread used to poll the hardware for status and data.
 *
 */
void Ketek::acquisitionTask()
{
    asynStatus status;
    int currentPoint;
    int syncPoints;
    int mcaRawOffset;
    int requestedSize;
    epicsUInt16 syncRunActive;
    epicsUInt16 syncSegmentNumber;
    //epicsUInt16 syncChannelIndex;
    //epicsUInt16 syncCycleCounterInternal;
    //epicsUInt16 syncCycleCounterMaster;
    //epicsUInt16 syncMode;
    epicsUInt16 syncCycleErrorCounter;
    epicsUInt16 syncSegmentQuantity;
    epicsUInt16 numBins;
    epicsUInt16 bytesPerBin;
    epicsUInt16 usTemp;
    int finalSegmentLen;
    int mcaBytesInBuffer;
    epicsUInt8 *pRawIn;
    epicsUInt8 *pRawOut;
    epicsFloat64 realTime, fastLiveTime, slowLiveTime, outputCounts, inputCounts, ocr, icr;
    epicsFloat64 boardTemp;
    //epicsFloat64 pollTime, sleepTime;
    size_t nRead;
    int eomReason;
    int arrayCallbacks;
    epicsTimeStamp start;
    epicsTimeStamp now;
    const char* functionName = "acquisitionTask";

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s acquisition task started!\n",
        driverName, functionName);

    lock();

    while (polling_) /* ... round and round and round we go until the IOC is shut down */
    {

        status = readSingleParam(pSyncRunActive, &syncRunActive);
        setIntegerParam(KetekSyncRunning, syncRunActive);
        if (!syncRunActive)
        {
            /* Release the lock while we wait for an event that says acquire has started, then lock again */
            unlock();
            /* Wait for syncStartEvent */
            syncStartEvent_->wait();
            lock();
        }
        epicsTimeGetCurrent(&start);
        requestedSize = KETEK_MAX_UDP_LEN;
        unlock();
        status = pasynOctetSyncIO->read(pasynUserSync_, (char *)syncMsgBuffer_, requestedSize, 
                                        KETEK_UDP_TIMEOUT, &nRead, &eomReason);
        lock();
        if (status) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s error reading UDP packet, status=%d, requestedSize=%d\n",
                      driverName, functionName, status, requestedSize);
            goto skip;
        } else {
            asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s received UDP packet, requestedSize=%d, nRead=%d, eomReason=%d\n",
                      driverName, functionName, requestedSize, (int)nRead, eomReason);
        }
        // syncChannelIndex should be 1.  If not there is an error, skip
        syncSegmentNumber = extractSyncParam(20, 139);
        if (syncSegmentNumber != 1) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s syncSegmentNumber should be 1 but is %d\n",
                      driverName, functionName, syncSegmentNumber);
            goto skip;
        }
        //syncChannelIndex         = extractSyncParam(0,  143);
        //syncCycleCounterInternal = extractSyncParam(4,  149);
        //syncCycleCounterMaster   = extractSyncParam(8,  150);
        syncCycleErrorCounter    = extractSyncParam(12, 147);
        if (syncCycleErrorCounter != 0) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s syncCycleErrorCounter=%d\n",
                      driverName, functionName, syncCycleErrorCounter);
        }
        syncSegmentQuantity = extractSyncParam(16, 138);
        //syncMode            = extractSyncParam(48, 144);
        usTemp              = extractSyncParam(64, 73);
        boardTemp = usTemp/16.0 - 273.15;
        numBins             = extractSyncParam(136, 20);
        numBins = 1<<numBins;
        bytesPerBin         = extractSyncParam(140, 21);
        realTime     = extractDoubleSyncParam(72, 6) * KETEK_ELAPSED_TIME_UNITS;
        fastLiveTime = extractDoubleSyncParam(80, 8) * KETEK_ELAPSED_TIME_UNITS;
        outputCounts = extractDoubleSyncParam(88, 10);
        inputCounts  = extractDoubleSyncParam(96, 12);
        ocr          = extractDoubleSyncParam(104, 14);
        icr          = extractDoubleSyncParam(112, 16);
        slowLiveTime = realTime * outputCounts / inputCounts;

        mcaRawOffset = 144;
        mcaBytesInBuffer = nRead - mcaRawOffset;
        pRawIn = syncMsgBuffer_ + mcaRawOffset;
        pRawOut = syncRawMCABuffer_;
        memcpy(pRawOut, pRawIn, mcaBytesInBuffer);
        pRawOut += mcaBytesInBuffer;
        finalSegmentLen = (numBins*bytesPerBin) - (KETEK_MAX_UDP_LEN-144) - (syncSegmentQuantity-2)*(KETEK_MAX_UDP_LEN-24) + 24;
        for (int segment=2; segment<=syncSegmentQuantity; segment++) {
            requestedSize = KETEK_MAX_UDP_LEN;
            if (segment == syncSegmentQuantity) requestedSize = finalSegmentLen;
            unlock();
            status = pasynOctetSyncIO->read(pasynUserSync_, (char *)syncMsgBuffer_, requestedSize, 
                                            KETEK_UDP_TIMEOUT, &nRead, &eomReason);
            lock();
            if (status) {
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s error reading UDP packet, status=%d, requestedSize=%d\n",
                          driverName, functionName, status, requestedSize);
                goto skip;
            } else {
                asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, "%s::%s received UDP packet, requestedSize=%d, nRead=%d, eomReason=%d\n",
                          driverName, functionName, requestedSize, (int)nRead, eomReason);
            }
            // syncChannelIndex should be segment.  If not there is an error, skip
            syncSegmentNumber = extractSyncParam(20, 139);
            if (syncSegmentNumber != segment) {
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s syncSegmentNumber should be %d but is %d\n",
                          driverName, functionName, segment, syncSegmentNumber);
                goto skip;
            }
            mcaRawOffset = 24;
            mcaBytesInBuffer = nRead - mcaRawOffset;
            pRawIn = syncMsgBuffer_ + mcaRawOffset;
            memcpy(pRawOut, pRawIn, mcaBytesInBuffer);
            pRawOut += mcaBytesInBuffer;
        }
        getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
        if (arrayCallbacks) {
            size_t dims[1];
            dims[0] = numBins;
            NDArray *pArray;
            NDDataType_t dataType = NDUInt32;
            switch (bytesPerBin) {
                case 1: dataType = NDUInt8; break;
                case 2: dataType = NDUInt16; break;
                case 3: dataType = NDUInt32; break;
                default:
                    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s unexpected bytesPerBin=%d\n",
                              driverName, functionName, bytesPerBin);
                    break;
            }
            pArray = this->pNDArrayPool->alloc(1, dims, dataType, 0, NULL );
            pRawIn = syncRawMCABuffer_;
            switch (bytesPerBin) {
                case 1:
                    memcpy(pArray->pData, pRawIn, numBins);
                    break;
                case 2:
                    memcpy(pArray->pData, pRawIn, numBins*2);
                    break;
                case 3:
                    epicsUInt32 *pOut = (epicsUInt32 *)pArray->pData;
                    for (int i=0; i<numBins; i++) {
                        pOut[i] = pRawIn[i*3] + pRawIn[i*3+1]*256L + pRawIn[i*3+2]*65536L;
                    }
                    break;
            }
            pArray->pAttributeList->add("RealTime",     "Real time",         NDAttrFloat64, &realTime);
            pArray->pAttributeList->add("FastLiveTime", "Live time (fast)",  NDAttrFloat64, &fastLiveTime);
            pArray->pAttributeList->add("SlowLiveTime", "Live time (slow)",  NDAttrFloat64, &slowLiveTime);
            pArray->pAttributeList->add("InputCounts",  "Input counts",      NDAttrFloat64, &inputCounts);
            pArray->pAttributeList->add("OutputCounts", "Output counts",     NDAttrFloat64, &outputCounts);
            pArray->pAttributeList->add("ICR",          "Input count rate",  NDAttrFloat64, &icr);
            pArray->pAttributeList->add("OCR",          "Output count rate", NDAttrFloat64, &ocr);

            epicsTimeGetCurrent(&now);
            pArray->timeStamp = now.secPastEpoch + now.nsec / 1.e9;
            updateTimeStamp(&pArray->epicsTS);
            pArray->uniqueId = uniqueId_;
            uniqueId_++;
            int arrayCounter;
            getIntegerParam(NDArrayCounter, &arrayCounter);
            arrayCounter++;
            setIntegerParam(NDArrayCounter, arrayCounter);
            getAttributes(pArray->pAttributeList);
            doCallbacksGenericPointer(pArray, NDArrayData, 0);
            pArray->release();
        }
    
        // Copy the spectral data for the first pixel in this buffer to the mcaRaw buffers.
        // This provides an update of the spectra and statistics while mapping is in progress
        // if the user sets the MCA spectra to periodically read.
        for (int i=0; i<numBins; i++) {
            switch (bytesPerBin) {
                case 1:
                    mcaData_[i] = syncRawMCABuffer_[i];
                    break;
                case 2:
                    mcaData_[i] = syncRawMCABuffer_[i*2] + syncRawMCABuffer_[i*2+1]*256L;
                    break;
                case 3:
                    mcaData_[i] = syncRawMCABuffer_[i*3] + syncRawMCABuffer_[i*3+1]*256L + syncRawMCABuffer_[i*3+2]*65536L;
                    break;
            }
        }
        setDoubleParam(mcaElapsedRealTime,   realTime);
        setDoubleParam(mcaElapsedLiveTime,   slowLiveTime);
        setDoubleParam(KetekFastLiveTime,    fastLiveTime);
        setIntegerParam(KetekInputCounts,    (int)inputCounts);
        setIntegerParam(KetekOutputCounts,   (int)outputCounts);
        setDoubleParam(KetekInputCountRate,  icr);
        setDoubleParam(KetekOutputCountRate, ocr);
        setDoubleParam(KetekBoardTemperature,boardTemp);
        getIntegerParam(KetekSyncCurrentPoint, &currentPoint);
        currentPoint++;
        setIntegerParam(KetekSyncCurrentPoint, currentPoint);

        getIntegerParam(KetekSyncPoints, &syncPoints);
        if (currentPoint >= syncPoints) {
            this->stopSyncAcquire();
        }

        skip:
        callParamCallbacks();
    }
}

epicsUInt16 Ketek::extractSyncParam(int offset, int paramID)
{
    static const char *functionName = "extractSyncParam";
    if (syncMsgBuffer_[offset] != paramID) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                  "%s::%s unexpected paramID, should be %d actual=%d\n",
                  driverName, functionName, paramID, syncMsgBuffer_[offset]);
        return 65535;
    }
    if (syncMsgBuffer_[offset+1] != 0) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                  "%s::%s paramID=%d bad status=%d\n",
                  driverName, functionName, paramID, syncMsgBuffer_[offset+1]);
        return 65535;
    }
    return syncMsgBuffer_[offset+2]*256 + syncMsgBuffer_[offset+3];
}

epicsFloat64 Ketek::extractDoubleSyncParam(int offset, int paramID)
{
    //static const char *functionName = "extractDoubleSyncParam";
    epicsUInt16 t1, t2;
    t1 = extractSyncParam(offset,   paramID);
    t2 = extractSyncParam(offset+4, paramID+1);
    return t1 + t2*65536;
}


void Ketek::report(FILE *fp, int details)
{
    if (details > 0) {
    }
    asynPortDriver::report(fp, details);
}


void Ketek::shutdown()
{
    //static const char *functionName = "shutdown";
    polling_ = false;
}


static const iocshArg KetekConfigArg0 = {"Asyn port name", iocshArgString};
static const iocshArg KetekConfigArg1 = {"IP port name", iocshArgString};
static const iocshArg KetekConfigArg2 = {"Host IP address", iocshArgString};
static const iocshArg KetekConfigArg3 = {"Host UDP port", iocshArgInt};
static const iocshArg * const KetekConfigArgs[] =  {&KetekConfigArg0,
                                                    &KetekConfigArg1,
                                                    &KetekConfigArg2,
                                                    &KetekConfigArg3};
static const iocshFuncDef configKetek = {"KetekConfig", 4, KetekConfigArgs};
static void configKetekCallFunc(const iocshArgBuf *args)
{
    KetekConfig(args[0].sval, args[1].sval, args[2].sval, args[3].ival);
}

static void ketekRegister(void)
{
    iocshRegister(&configKetek, configKetekCallFunc);
}

extern "C" {
epicsExportRegistrar(ketekRegister);
}
