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
#include <asynPortDriver.h>
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

/** Only used for debugging/error messages to identify where the message comes from*/
static const char *driverName = "Ketek";

static void c_shutdown(void* arg)
{
    Ketek *pKetek = (Ketek*)arg;
    pKetek->shutdown();
    //delete pKetek;
}

extern "C" int KetekConfig(const char *portName, const char *ipPortName)
{
    new Ketek(portName, ipPortName);
    return 0;
}

Ketek::Ketek(const char *portName, const char *ipPortName)
    : asynPortDriver(portName, 1,
            asynInt32Mask | asynFloat64Mask | asynInt32ArrayMask | asynFloat64ArrayMask | asynOctetMask | asynDrvUserMask,
            asynInt32Mask | asynFloat64Mask | asynInt32ArrayMask | asynFloat64ArrayMask | asynOctetMask,
            ASYN_CANBLOCK, 1, 0, 0)

{
    const char *functionName = "Ketek";
    asynStatus status;
    
    status = pasynOctetSyncIO->connect(ipPortName, 0, &pasynUserRemote_, 0);
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
          "%s::%s cannot connect to Ketek devices, status=%d\n",
          driverName, functionName, status);
        return;
    }

    /* Internal asyn driver parameters */
    createParam(KetekPortNameSelfString,           asynParamOctet,   &KetekPortNameSelf);
    createParam(KetekDriverVersionString,          asynParamOctet,   &KetekDriverVersion);
    createParam(KetekModelString,                  asynParamOctet,   &KetekModel);
    createParam(KetekFirmwareVersionString,        asynParamOctet,   &KetekFirmwareVersion);
    createParam(KetekErasedString,                 asynParamInt32,   &KetekErased);

    /* Diagnostic trace parameters */
    createParam(KetekEventTriggerSourceString,     asynParamInt32,       &KetekEventTriggerSource);
    createParam(KetekEventTriggerValueString,      asynParamInt32,        &KetekEventTriggerValue);
    createParam(KetekEventRateCalculateString,     asynParamInt32,        &KetekEventRateCalculate);
    createParam(KetekEventRateString,              asynParamInt32,        &KetekEventRate);
    createParam(KetekScopeDataSourceString,        asynParamInt32,        &KetekScopeDataSource);
    createParam(KetekScopeTimeArrayString,         asynParamFloat64Array, &KetekScopeTimeArray);
    createParam(KetekScopeIntervalString,          asynParamFloat64,      &KetekScopeInterval);
    createParam(KetekScopeTriggerTimeoutString,    asynParamInt32,        &KetekScopeTriggerTimeout);
    createParam(KetekScopeReadString,              asynParamInt32,        &KetekScopeRead);

    /* Runtime statistics */
    createParam(KetekInputCountRateString,         asynParamFloat64, &KetekInputCountRate);
    createParam(KetekOutputCountRateString,        asynParamFloat64, &KetekOutputCountRate);
    createParam(KetekInputCountsString,            asynParamInt32,   &KetekInputCounts);
    createParam(KetekOutputCountsString,           asynParamInt32,   &KetekOutputCounts);

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
    createParam(KetekBoardTemperatureString,            asynParamFloat64, &KetekBoardTemperature);

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

    setStringParam(KetekPortNameSelf, this->portName);
    setStringParam(KetekModel, "AXAS-D 3.0");
    setStringParam(KetekDriverVersion, DRIVER_VERSION);
    epicsUInt16 fwMajor, fwMinor, fwPatch;
    status = readSingleParam(pFirmwareMajor, &fwMajor);
    status = readSingleParam(pFirmwareMinor, &fwMinor);
    status = readSingleParam(pFirmwarePatch, &fwPatch);
    char firmwareString[32];
    sprintf(firmwareString, "%d.%d.%d", fwMajor, fwMinor, fwPatch);
    setStringParam(KetekFirmwareVersion, firmwareString);

    /* Register the epics exit function to be called when the IOC exits... */
    epicsAtExit(c_shutdown, this);

    /* Set the parameters in param lib */
    setIntegerParam(mcaAcquiring, 0);

    /* Create the start and stop events that will be used to signal our
     * acquisitionTask when to start/stop polling the HW     */
    cmdStartEvent_ = new epicsEvent();
    cmdStopEvent_ = new epicsEvent();
    stoppedEvent_ = new epicsEvent();

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
    ketekRequest_t req;
    ketekResponse_t resp;
    static const char *functionName = "readSingleParam";
    
    req.paramID = paramID;
    req.command = KETEK_COMMAND_READ;
    status = pasynOctetSyncIO->writeRead(pasynUserRemote_, (const char *)&req, sizeof(req), 
                                        (char *)&resp, sizeof(resp), 
                                         1, &nWrite, &nRead, &eomReason);
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s error in writeRead, status=%d\n",
                  driverName, functionName, status);
        return status;
    }
    if (resp.status != 0) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s device returned error status=%d\n",
                  driverName, functionName, status);
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
                                         1, &nWrite, &nRead, &eomReason);
    if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s error in writeRead, status=%d\n",
                  driverName, functionName, status);
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

asynStatus Ketek::sendRcvMsg(ketekRequest_t *request, void *response, size_t responseSize)
{
    size_t nWrite, nRead;
    int eomReason;
    asynStatus status;
    static const char *functionName = "sendRcvMsg";

    status = pasynOctetSyncIO->writeRead(pasynUserRemote_, (const char *)request, sizeof(*request), 
                                        (char *)response, responseSize, 
                                        1.0, &nWrite, &nRead, &eomReason);
    if (status || (nRead != responseSize)) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s error in writeRead, status=%d, nWrite=%d, nRequested=%d, nRead=%d, eomReason=%d\n",
                  driverName, functionName, status, (int)nWrite, (int)responseSize, (int)nRead, eomReason);
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
    else if (function == KetekScopeRead) {
        status = this->getTraces();
    }
    else if ((function == KetekMediumFilterEnable) ||
             (function == KetekBaselineAverageLen) ||
             (function == KetekBaselineTrim)       ||
             (function == KetekBaselineCorrEnable) ||
             (function == mcaNumChannels)) {
        this->setConfiguration();
    }
    else if ((function == KetekPresetMode) ||
             (function == KetekPresetInputCounts) ||
             (function == KetekPresetOutputCounts)) {
        this->setPresets();
    }
            
    /* Call the callback */
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
        (function == KetekEnergyOffset))
    {
        this->setConfiguration();
    }
    else if ((function == mcaPresetRealTime) ||
             (function == mcaPresetLiveTime)) {
        this->setPresets();
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
    const char *functionName = "readInt32Array";

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
        "%s::%s function=%d\n",
        driverName, functionName, function);
    if (function == mcaData) {
        getIntegerParam(mcaNumChannels, &nBins);
        if (nBins > (int)nElements) nBins = (int)nElements;
        *nIn = nBins;
        this->getMcaData();
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
    if (iValue < 2) iValue = 2;
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

    readSingleParam(pBoardTemperature, &iValue);
    setDoubleParam(KetekBoardTemperature, iValue/16.0 - 273.15);

    return asynSuccess;
}

asynStatus Ketek::getAcquisitionStatistics()
{
    double realTime, liveTime, icr, ocr;
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
        sendRcvMsg(&req, (char *)allStats, sizeof(allStats));
        active       = ntohs(allStats[0].data);
        realTime     = (ntohs(allStats[1].data)  + ntohs(allStats[2].data)*65536.)*KETEK_ELAPSED_TIME_UNITS;
        liveTime     = (ntohs(allStats[3].data)  + ntohs(allStats[4].data)*65536.)*KETEK_ELAPSED_TIME_UNITS;
        outputCounts = (ntohs(allStats[5].data)  + ntohs(allStats[6].data)*65536.);
        inputCounts  = (ntohs(allStats[7].data)  + ntohs(allStats[8].data)*65536.);
        ocr          = (ntohs(allStats[9].data)  + ntohs(allStats[10].data)*65536.);
        icr          = (ntohs(allStats[11].data) + ntohs(allStats[12].data)*65536.);

        setIntegerParam(mcaAcquiring,         active);
        setDoubleParam( mcaElapsedRealTime,   realTime); 
        setDoubleParam( mcaElapsedLiveTime,   liveTime);
        setIntegerParam(KetekOutputCounts,    outputCounts);
        setIntegerParam(KetekInputCounts,     inputCounts);
        setDoubleParam( KetekOutputCountRate, ocr);
        setDoubleParam( KetekInputCountRate,  icr);

        asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
            "%s::%s\n"
            "              active=%d\n"
            "           real time=%f\n"
            "           live time=%f\n"
            "       output counts=%d\n"
            "        input counts=%d\n"
            "    input count rate=%f\n"
            "   output count rate=%f\n",
            driverName, functionName,
            active,
            realTime,
            liveTime,
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
    status = sendRcvMsg(&req, (char *)mcaRaw_, numBins*bytesPerBin);
    for (int i=0; i<numBins; i++) {
       mcaData_[i] = mcaRaw_[i*3] + mcaRaw_[i*3+1]*256L + mcaRaw_[i*3+2]*65536L;
    }
    return status;
}


/* Get trace data */
asynStatus Ketek::getTraces()
{
    //int iValue;
    //uint16_t mode=0;
    //uint32_t i;
    //double traceTime;
    //const char *functionName = "getTraces";

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

    // signal cmdStartEvent to start the polling thread
    cmdStartEvent_->signal();
    return status;
}


asynStatus Ketek::stopAcquiring() {
    asynStatus status;
    status = writeSingleParam(pRunStop, 0);
    return status;
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
}


static const iocshArg KetekConfigArg0 = {"Asyn port name", iocshArgString};
static const iocshArg KetekConfigArg1 = {"IP port name", iocshArgString};
static const iocshArg * const KetekConfigArgs[] =  {&KetekConfigArg0,
                                                    &KetekConfigArg1};
static const iocshFuncDef configKetek = {"KetekConfig", 2, KetekConfigArgs};
static void configKetekCallFunc(const iocshArgBuf *args)
{
    KetekConfig(args[0].sval, args[1].sval);
}

static void ketekRegister(void)
{
    iocshRegister(&configKetek, configKetekCallFunc);
}

extern "C" {
epicsExportRegistrar(ketekRegister);
}
