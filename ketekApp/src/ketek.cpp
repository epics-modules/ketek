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
#include <epicsMessageQueue.h>
#include <epicsMutex.h>
#include <epicsExit.h>
#include <envDefs.h>
#include <iocsh.h>
#include <asynOctetSyncIO.h>

/* MCA includes */
#include <drvMca.h>

/* Area Detector includes */
#include <asynNDArrayDriver.h>

#include <epicsExport.h>
#include "ketek.h"

#define DRIVER_VERSION       "1.0.0"
#define ALL_BOARDS              -1
#define MAX_MESSAGE_DATA          20
#define MSG_QUEUE_SIZE            50
#define MESSAGE_TIMEOUT           20.
#define MIN_TRACE_TIME         0.016
#define TRACE_LEN_INC          16384
#define NUM_MAPPING_STATS          4
#define NUM_MAPPING_ADV_STATS     24

#define KETEK_FILTER_TIME_UNITS 12.5e-9
#define KETEK_USEC_TO_FILTER_UNITS 1.e-6/KETEK_FILTER_TIME_UNITS
#define KETEK_ELAPSED_TIME_UNITS 10e-6

/** Only used for debugging/error messages to identify where the message comes from*/
static const char *driverName = "Ketek";

static void c_shutdown(void* arg)
{
    Ketek *pKetek = (Ketek*)arg;
    pKetek->shutdown();
    //delete pKetek;
}

static void acquisitionTaskC(void *drvPvt)
{
    Ketek *pKetek = (Ketek *)drvPvt;
    pKetek->acquisitionTask();
}

extern "C" int KetekConfig(const char *portName, const char *ipPortName)
{
    new Ketek(portName, ipPortName);
    return 0;
}

Ketek::Ketek(const char *portName, const char *ipPortName)
    : asynNDArrayDriver(portName, 1, 0, 0,
            asynInt32Mask | asynFloat64Mask | asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask | asynOctetMask | asynDrvUserMask,
            asynInt32Mask | asynFloat64Mask | asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask | asynOctetMask,
            ASYN_MULTIDEVICE | ASYN_CANBLOCK, 1, 0, 0),
    uniqueId_(0), traceLength_(0), newTraceTime_(true), traceBuffer_(0), traceBufferInt32_(0), traceTimeBuffer_(0)

{
    int status = asynSuccess;
    const char *functionName = "Ketek";
    
    status = pasynOctetSyncIO->connect(ipPortName, 0, &pasynUserRemote_, 0);

    epicsUInt16 fwMajor, fwMinor, fwPatch;
    status = readSingleParam(pFirmwareMajor, &fwMajor);
    status = readSingleParam(pFirmwareMinor, &fwMinor);
    status = readSingleParam(pFirmwarePatch, &fwPatch);
    char firmwareString[32];
    sprintf(firmwareString, "%d.%d.%d", fwMajor, fwMinor, fwPatch);
    printf("Firmware version=%s\n", firmwareString);
    setStringParam(ADFirmwareVersion, firmwareString);

    /* General parameters */
    createParam(KetekCollectModeString,            asynParamInt32,   &KetekCollectMode);
    createParam(KetekCurrentPixelString,           asynParamInt32,   &KetekCurrentPixel);
    createParam(KetekMaxEnergyString,              asynParamFloat64, &KetekMaxEnergy);
    createParam(KetekSpectrumXAxisString,          asynParamFloat64Array, &KetekSpectrumXAxis);

    /* Internal asyn driver parameters */
    createParam(KetekErasedString,                 asynParamInt32,   &KetekErased);
    createParam(KetekAcquiringString,              asynParamInt32,   &KetekAcquiring);
    createParam(KetekPollTimeString,               asynParamFloat64, &KetekPollTime);

    /* Diagnostic trace parameters */
    createParam(KetekTraceDataString,              asynParamInt32Array,   &KetekTraceData);
    createParam(KetekTraceTimeArrayString,         asynParamFloat64Array, &KetekTraceTimeArray);
    createParam(KetekTraceTimeString,              asynParamFloat64,      &KetekTraceTime);
    createParam(KetekTraceTriggerInstantString,    asynParamInt32,        &KetekTraceTriggerInstant);
    createParam(KetekTraceTriggerRisingString,     asynParamInt32,        &KetekTraceTriggerRising);
    createParam(KetekTraceTriggerFallingString,    asynParamInt32,        &KetekTraceTriggerFalling);
    createParam(KetekTraceTriggerLevelString,      asynParamInt32,        &KetekTraceTriggerLevel);
    createParam(KetekTraceTriggerWaitString,       asynParamFloat64,      &KetekTraceTriggerWait);
    createParam(KetekTraceLengthString,            asynParamInt32,        &KetekTraceLength);
    createParam(KetekReadTraceString,              asynParamInt32,        &KetekReadTrace);

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

    /* Other parameters */
    createParam(KetekInputModeString,               asynParamInt32,   &KetekInputMode);
    createParam(KetekAnalogOffsetString,            asynParamInt32,   &KetekAnalogOffset);
    createParam(KetekGatingModeString,              asynParamInt32,   &KetekGatingMode);
    createParam(KetekMappingPointsString,           asynParamInt32,   &KetekMappingPoints);

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

    /* Register the epics exit function to be called when the IOC exits... */
    epicsAtExit(c_shutdown, this);

    /* Set the parameters in param lib */
    status |= setIntegerParam(KetekCollectMode, 0);
    setIntegerParam(mcaAcquiring, 0);

    /* Create the start and stop events that will be used to signal our
     * acquisitionTask when to start/stop polling the HW     */
    cmdStartEvent_ = new epicsEvent();
    cmdStopEvent_ = new epicsEvent();
    stoppedEvent_ = new epicsEvent();

    /* Allocate an internal buffer long enough to hold all the energy values in a spectrum */
    spectrumXAxisBuffer_ = (epicsFloat64*)calloc(KETEK_MAX_MCA_BINS, sizeof(epicsFloat64));

    /* Start up acquisition thread */
    setDoubleParam(KetekPollTime, 0.01);
    polling_ = 1;
    status = (epicsThreadCreate("acquisitionTask",
              epicsThreadPriorityMedium,
              epicsThreadGetStackSize(epicsThreadStackMedium),
              (EPICSTHREADFUNC)acquisitionTaskC, this) == NULL);
    if (status) {
        printf("%s:%s epicsThreadCreate failure for image task\n",
                driverName, functionName);
        return;
    }

    setStringParam(NDDriverVersion, DRIVER_VERSION);
    setStringParam(ADManufacturer, "KETEK");
    setStringParam(ADModel, "AXAS-D 3.0");
    setStringParam(ADSerialNumber, "0");

/*
        char firmwareVersion[20];
        snprintf(firmwareVersion, sizeof(firmwareVersion)-1, "%d.%d.%d",
                 ketekReply_[0], ketekReply_[1], ketekReply_[2]);
        setStringParam (i, ADFirmwareVersion, firmwareVersion);
        setIntegerParam(i, KetekForceRead,                  0);
        setDoubleParam (i, mcaPresetCounts,               0.0);
        setDoubleParam (i, mcaElapsedCounts,              0.0);
        setDoubleParam (i, mcaPresetRealTime,             0.0);
        setDoubleParam (i, mcaPresetRealTime,             0.0);
        setIntegerParam(i, KetekCurrentPixel,             0);
        setIntegerParam(i, mcaNumChannels,                2048);
        setDoubleParam (i, KetekFastFilterThreshold,      0.1);
        setDoubleParam (i, KetekEnergyFilterThreshold,    0.0);
        setDoubleParam (i, KetekBaselineThreshold,        0.0);
        setDoubleParam (i, KetekMaxRiseTime,              0.25);
        setDoubleParam (i, KetekGain,                     1.0);
        setDoubleParam (i, KetekPeakingTime,              1.0);
        setDoubleParam (i, KetekMaxPeakingTime,           0.0);
        setDoubleParam (i, KetekFlatTop,                  0.05);
        setDoubleParam (i, KetekFastPeakingTime,          0.03);
        setDoubleParam (i, KetekFastFlatTop,              0.01);
        setDoubleParam (i, KetekResetRecoveryTime,        6.0);
        setDoubleParam (i, KetekZeroPeakFreq,             1000.);
        setIntegerParam(i, KetekBaselineSamples,          64);
        setIntegerParam(i, KetekInvertedInput,            0);
        setDoubleParam (i, KetekTimeConstant,             0.0);
        setIntegerParam(i, KetekBaseOffset,               0);
        setIntegerParam(i, KetekResetThreshold,           300);
*/
    /* Read the MCA and DXP parameters once */
    dataAcquiring();
    this->getAcquisitionStatistics();

    // Enable array callbacks by default
    setIntegerParam(NDArrayCallbacks, 1);

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
    status = writeSingleParam(pStopValueHigh, (value & 0xFFFF0000) >> 8);
    return status;
}

asynStatus Ketek::sendRcvMsg(ketekRequest_t *request, void *response, size_t responseSize)
{
    size_t nWrite, nRead;
    int eomReason;
    asynStatus status;

    status = pasynOctetSyncIO->writeRead(pasynUserRemote_, (const char *)request, sizeof(request), 
                                        (char *)response, responseSize, 
                                         1, &nWrite, &nRead, &eomReason);
    return status;
}
/* virtual methods to override from asynNDArrayDriver */
asynStatus Ketek::writeInt32( asynUser *pasynUser, epicsInt32 value)
{
    asynStatus status = asynSuccess;
    int function = pasynUser->reason;
    int acquiring, mode;
    const char* functionName = "writeInt32";

    /* Set the parameter and readback in the parameter library.  This may be overwritten later but that's OK */
    status = setIntegerParam(function, value);

    if (function == mcaStartAcquire)
    {
        status = this->startAcquiring();
    }
    else if (function == mcaStopAcquire)
    {
        status = this->stopAcquiring();
        /* Wait for the acquisition task to realize the run has stopped and do the callbacks */
        while (1) {
            getIntegerParam(mcaAcquiring, &acquiring);
            if (!acquiring) break;
            this->unlock();
            epicsThreadSleep(epicsThreadSleepQuantum());
            this->lock();
        }
    }
    else if (function == mcaReadStatus)
    {
        getIntegerParam(KetekCollectMode, &mode);
        asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
            "%s::%s mcaReadStatus [%d] mode=%d\n",
            driverName, functionName, function, mode);
        /* We let the polling task set the acquiring flag, so that we can be sure that
         * the statistics and data have been read when needed.  DON'T READ ACQUIRE STATUS HERE */
        getIntegerParam(mcaAcquiring, &acquiring);
        /* If we are acquiring then read the statistics, else we use the cached values */
        if (acquiring) status = this->getAcquisitionStatistics();
    }
    else if (function == KetekTraceLength) {
        // For length to be a multiple of 16K.
        uint16_t length = value / TRACE_LEN_INC;
        if (length < 1) length = 1;
        value = length * TRACE_LEN_INC;
        setIntegerParam(function, value);
        traceLength_ = value;
        /* Allocate a buffer for the trace data */
        if (traceBuffer_) free(traceBuffer_);
        traceBuffer_ = (uint16_t *)malloc(traceLength_ * sizeof(uint16_t));
        if (traceBufferInt32_) free(traceBufferInt32_);
        traceBufferInt32_ = (int32_t *)malloc(traceLength_ * sizeof(int32_t));
        /* Allocate a buffer for the trace time array */
        if (traceTimeBuffer_) free(traceTimeBuffer_);
        traceTimeBuffer_ = (epicsFloat64 *)malloc(traceLength_ * sizeof(epicsFloat64));
        newTraceTime_ = true;
    }
    else if (function == KetekReadTrace) {
        status = this->getTraces();
    }
    else if ((function == KetekEnableConfigure) && (value == 1)) {
    }
    else if (function == KetekEnableBoard) {
    }
    else if (function == KetekKeepAlive) {
        // This just periodically reads the firmware version of the first board to keep the socket alive
    }
    else if ((function == KetekMediumFilterEnable) ||
             (function == KetekBaselineAverageLen) ||
             (function == KetekBaselineTrim)       ||
             (function == KetekBaselineCorrEnable))
    {
        this->setKetekConfiguration();
    }
            
    /* Call the callback */
    callParamCallbacks();
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: exit\n",
        driverName, functionName);
    return status;
}

asynStatus Ketek::writeFloat64( asynUser *pasynUser, epicsFloat64 value)
{
    asynStatus status = asynSuccess;
    int function = pasynUser->reason;
    const char *functionName = "writeFloat64";

    /* Set the parameter and readback in the parameter library.  This may be overwritten later but that's OK */
    status = setDoubleParam(function, value);

    if (function == KetekMaxEnergy) {
        int numMcaChannels;
        getIntegerParam(mcaNumChannels, &numMcaChannels);
        if (numMcaChannels <= 0.)
            numMcaChannels = 2048;
        /* Create a waveform which contain the value (in keV) of all the energy bins in the spectrum */
        double binEnergyEV = value / numMcaChannels;
        for(int bin=0; bin<numMcaChannels; bin++)
        {
            *(spectrumXAxisBuffer_ + bin) = (bin + 1) * binEnergyEV;
        }
        doCallbacksFloat64Array(spectrumXAxisBuffer_, numMcaChannels, KetekSpectrumXAxis, 0);
        // Must call setKetekConfiguration() because the energy parameters need to be recalculated and reloaded
        this->setKetekConfiguration();
    }

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
        this->setKetekConfiguration();
    }

    if (function == KetekTraceTime) {
        // Convert from microseconds to decimation of 16 ns.
        uint16_t decRatio = (uint16_t)(value/MIN_TRACE_TIME);
        if (decRatio < 1) decRatio = 1;
        if (decRatio > 32) decRatio = 32;
        value = decRatio * MIN_TRACE_TIME;
        setDoubleParam(function, value);
        newTraceTime_ = true;
    }
    /* Call the callback */
    callParamCallbacks();

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
        "%s:%s: exit\n",
        driverName, functionName);
    return status;
}

asynStatus Ketek::readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn)
{
    asynStatus status = asynSuccess;
    int function = pasynUser->reason;
    int nBins, acquiring,mode;
    int i;
    const char *functionName = "readInt32Array";

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
        "%s::%s function=%d\n",
        driverName, functionName, function);
    if (function == mcaData)
    {
        getIntegerParam(mcaNumChannels, &nBins);
        if (nBins > (int)nElements) nBins = (int)nElements;
        getIntegerParam(mcaAcquiring, &acquiring);
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
            "%s::%s getting mcaData, mcaNumChannels=%d mcaAcquiring=%d\n",
            driverName, functionName, nBins, acquiring);
        *nIn = nBins;
        getIntegerParam(KetekCollectMode, &mode);

        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
            "%s::%s mode=%d acquiring=%d\n",
            driverName, functionName, mode, acquiring);
        if (acquiring) {
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

    asynPrint(pasynUser, ASYN_TRACE_FLOW,
        "%s:%s: exit\n",
        driverName, functionName);
    return(status);
}

asynStatus Ketek::setKetekConfiguration()
{
    double dValue;
    int iValue;
    //static const char *functionName = "setKetekParam";

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
    iValue = round(dValue * KETEK_USEC_TO_FILTER_UNITS);
    if (iValue < 0) iValue = 0;
    if (iValue > 16384) iValue = 16384;
    writeSingleParam(pFastTrigThreshold, iValue);

    getDoubleParam(KetekMediumFilterThreshold, &dValue);
    iValue = round(dValue * KETEK_USEC_TO_FILTER_UNITS);
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

    return asynSuccess;
}

bool Ketek::waveformAcquiring()
{
    bool acquiring=false;
    int boardAcquiring=0;
    static const char *functionName = "waveformAcquiring";

    /* Note: we use the internal parameter KetekAcquiring rather than mcaAcquiring here
     * because we need to do callbacks in acquisitionTask() on all other parameters before
     * we do callbacks on mcaAcquiring, and callParamCallbacks does not allow control over the order. */
    setIntegerParam(KetekAcquiring, boardAcquiring);
    if (boardAcquiring) acquiring = true;
    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
              "%s::%s boardAcquiring=%d\n",
              driverName, functionName, boardAcquiring);
    setIntegerParam(KetekAcquiring, acquiring?1:0);
    return acquiring;
}

bool Ketek::dataAcquiring()
{
    bool acquiring=false;
    int boardAcquiring=0;
    static const char *functionName = "dataAcquiring";

    /* Note: we use the internal parameter KetekAcquiring rather than mcaAcquiring here
     * because we need to do callbacks in acquisitionTask() on all other parameters before
     * we do callbacks on mcaAcquiring, and callParamCallbacks does not allow control over the order. */
    setIntegerParam(KetekAcquiring, boardAcquiring);
    if (boardAcquiring) acquiring = true;
    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
        "%s::%s boardAcquiring=%d\n",
        driverName, functionName, boardAcquiring);
    setIntegerParam(KetekAcquiring, acquiring ? 1 : 0);
    return acquiring;
}

asynStatus Ketek::getAcquisitionStatistics()
{
    double realTime, liveTime, icr, ocr;
    int inputCounts, outputCounts;
    int erased;
    asynStatus status=asynSuccess;
    struct statsItem {
       epicsUInt8 paramId;
       epicsUInt8 status;
       epicsUInt16 data;
    };
    ketekRequest_t req;
    statsItem allStats[13];
    const char *functionName = "getAcquisitionStatistics";

    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
        "%s::%s start\n",
        driverName, functionName);
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
        status = sendRcvMsg(&req, (char *)allStats, sizeof(allStats));
        realTime     = (ntohs(allStats[0].data)  + ntohs(allStats[1].data)*65536)/KETEK_ELAPSED_TIME_UNITS;
        liveTime     = (ntohs(allStats[2].data)  + ntohs(allStats[3].data)*65536)/KETEK_ELAPSED_TIME_UNITS;
        outputCounts = (ntohs(allStats[4].data)  + ntohs(allStats[5].data)*65536);
        inputCounts  = (ntohs(allStats[6].data)  + ntohs(allStats[7].data)*65536);
        ocr          = (ntohs(allStats[9].data)  + ntohs(allStats[9].data)*65536);
        icr          = (ntohs(allStats[10].data) + ntohs(allStats[11].data)*65536);

        setDoubleParam( mcaElapsedRealTime,   realTime); 
        setDoubleParam( mcaElapsedLiveTime,   liveTime);
        setIntegerParam(KetekOutputCounts,    outputCounts);
        setIntegerParam(KetekInputCounts,     inputCounts);
        setDoubleParam( KetekOutputCountRate, ocr);
        setDoubleParam( KetekInputCountRate,  icr);

        asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
            "%s::%s\n"
            "           real time=%f\n"
            "           live time=%f\n"
            "       output counts=%d\n"
            "        input counts=%d\n"
            "    input count rate=%f\n"
            "   output count rate=%f\n",
            driverName, functionName,
            realTime,
            liveTime,
            outputCounts,
            inputCounts,
            ocr,
            icr);

    }
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: status=%d\n",
        driverName, functionName, status);
    return(asynSuccess);
}

asynStatus Ketek::getMcaData()
{
    asynStatus status = asynSuccess;
    int arrayCallbacks;
    epicsUInt16 numBins;
    epicsUInt16 bytesPerBin;
    ketekRequest_t req;
    //NDArray *pArray;
    NDDataType_t dataType;
    epicsTimeStamp now;
    const char* functionName = "getMcaData";

    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: enter\n",
        driverName, functionName);

    getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
    getIntegerParam(NDDataType, (int *)&dataType);

    epicsTimeGetCurrent(&now);

    /* Read the MCA spectrum */

    readSingleParam(pMCANumBins, &numBins);
    readSingleParam(pMCABytesPerBin, &bytesPerBin);
    epicsUInt8 *mcaRaw = (epicsUInt8 *) malloc(numBins*bytesPerBin);
    req.paramID = pMCARead;
    req.command = KETEK_COMMAND_READ;
    status = sendRcvMsg(&req, (char *)mcaRaw, numBins*bytesPerBin);
    for(int i=0; i<numBins; i++) {
       mcaData_[i] = mcaRaw[i*3] + mcaRaw[i*3+1]*256 + mcaRaw[i*3+2]*65536;
    }

// In the future we may want to do array callbacks with the MCA data.  For now we are not doing this.
//        if (arrayCallbacks)
//       {
//            /* Allocate a buffer for callback */
//            pArray = this->pNDArrayPool->alloc(1, &nChannels, dataType, 0, NULL);
//            pArray->timeStamp = now.secPastEpoch + now.nsec / 1.e9;
//            pArray->uniqueId = spectrumCounter;
//            /* TODO: Need to copy the data here */
//            doCallbacksGenericPointer(pArray, NDArrayData, addr);
//            pArray->release();
//       }

    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: exit\n",
        driverName, functionName);
    return status;
}


/* Get trace data */
asynStatus Ketek::getTraces()
{
    int iValue;
    //uint16_t mode=0;
    //uint32_t i;
    double traceTime;
    const char *functionName = "getTraces";

    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: enter\n",
        driverName, functionName);

    getDoubleParam(KetekTraceTime, &traceTime);
    // Convert from microseconds to decimation of 16 ns.
    uint16_t decRatio = (uint16_t)(traceTime/MIN_TRACE_TIME);
    if (decRatio < 1) decRatio = 1;
    if (decRatio > 32) decRatio = 32;
    setDoubleParam(KetekTraceTime, decRatio * MIN_TRACE_TIME);
    uint32_t triggerMask = 0;
    getIntegerParam(KetekTraceTriggerInstant, &iValue);
    if (iValue) triggerMask |= 1;
    getIntegerParam(KetekTraceTriggerRising, &iValue);
    if (iValue) triggerMask |= 2;
    getIntegerParam(KetekTraceTriggerFalling, &iValue);
    if (iValue) triggerMask |= 4;
    getIntegerParam(KetekTraceTriggerLevel, &iValue);
    //uint32_t triggerLevel = iValue;
    double triggerWaitTime;
    getDoubleParam(KetekTraceTriggerWait, &triggerWaitTime);
    getIntegerParam(KetekTraceLength, &iValue);
    uint16_t length = iValue / TRACE_LEN_INC;
    if (length < 1) length = 1;
    setIntegerParam(KetekTraceLength, length * TRACE_LEN_INC);
    callParamCallbacks();

/*
    while(1) {
        if (waveformAcquiring()) {
            epicsThreadSleep(0.001);
        } else {
            break;
        }
    }
    for (const auto& board: activeBoards_) {
        if (!getWaveData(ketekIdentifier_, board, traceBuffer_, traceLength_)) {
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                      "%s::%s error calling getWaveData\n", driverName, functionName);
            return asynError;
        }
        // There is a bug in their firmware, the last waveform entry is always 0.
        // This messes up auto-scaling displays.  Replace it with the next to last value for now
        traceBuffer_[traceLength_-1] = traceBuffer_[traceLength_-2];
        for (i=0; i<traceLength_; i++) {
            traceBufferInt32_[i] = traceBuffer_[i];
        }
        doCallbacksInt32Array(traceBufferInt32_, traceLength_, KetekTraceData, board);
    }
    if (newTraceTime_) {
        double traceTime;
        getDoubleParam(0, KetekTraceTime, &traceTime);
        newTraceTime_ = false;
        for (i=0; i<traceLength_; i++) {
            traceTimeBuffer_[i] = i * traceTime;
        }
        doCallbacksFloat64Array(traceTimeBuffer_, traceLength_, KetekTraceTimeArray, 0);
    }
*/
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: exit\n",
        driverName, functionName);
    return asynSuccess;
}

asynStatus Ketek::startAcquiring()
{
    asynStatus status = asynSuccess;
    int acquiring, erased;
    double presetReal, presetLive;
    int presetInputCounts, presetOutputCounts;
    int presetMode;
    int collectMode;
    int mappingPoints;
    const char *functionName = "startAcquiring";

    getIntegerParam(mcaAcquiring, &acquiring);
    getIntegerParam(KetekErased, &erased);
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
        "%s::%s acquiring=%d, erased=%d\n",
        driverName, functionName, acquiring, erased);
    /* if already acquiring we just ignore and return */
    if (acquiring) return status;

    getIntegerParam(KetekPresetMode,         &presetMode);
    getDoubleParam( mcaPresetRealTime,       &presetReal);
    getDoubleParam( mcaPresetLiveTime,       &presetLive);
    getIntegerParam(KetekPresetInputCounts,  &presetInputCounts);
    getIntegerParam(KetekPresetOutputCounts, &presetOutputCounts);
    getIntegerParam(KetekPresetMode,         &presetMode);
    getIntegerParam(KetekCollectMode,        &collectMode);
    getIntegerParam(KetekMappingPoints,      &mappingPoints);

    switch(presetMode) {
        case KetekPresetRealTime:
            writeStopValue(presetReal * KETEK_ELAPSED_TIME_UNITS);
            break;
        case KetekPresetLiveTime:
            writeStopValue(presetLive * KETEK_ELAPSED_TIME_UNITS);
            break;
        case KetekPresetInputCts:
            writeStopValue(presetInputCounts);
            break;
        case KetekPresetOutputCts:
            writeStopValue(presetOutputCounts);
            break;
    }
    writeSingleParam(pStopCondition, presetMode);
    writeSingleParam(pRunStart, erased ? 0 : 1);   

    setIntegerParam(KetekErased, 0); /* reset the erased flag */
    setIntegerParam(mcaAcquiring, 1); /* Set the acquiring flag */


    callParamCallbacks();

    // signal cmdStartEvent to start the polling thread
    cmdStartEvent_->signal();
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: exit\n",
        driverName, functionName);
    return status;
}

/** Thread used to poll the hardware for status and data.
 *
 */
void Ketek::acquisitionTask()
{
    int paramStatus;
    int mode;
    int acquiring = 0;
    epicsFloat64 pollTime, sleepTime, dtmp;
    epicsTimeStamp now, start;
    const char* functionName = "acquisitionTask";

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s acquisition task started!\n",
        driverName, functionName);

    lock();

    while (polling_) /* ... round and round and round we go until the IOC is shut down */
    {

        getIntegerParam(KetekAcquiring, &acquiring);
        if (!acquiring)
        {
            /* Release the lock while we wait for an event that says acquire has started, then lock again */
            unlock();
            /* Wait for someone to signal the cmdStartEvent */
            asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s Waiting for acquisition to start!\n",
                driverName, functionName);
            cmdStartEvent_->wait();
            lock();
            getIntegerParam(KetekCollectMode, &mode);
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                "%s::%s [%s]: started! (mode=%d)\n",
                driverName, functionName, this->portName, mode);
        }
        epicsTimeGetCurrent(&start);

        /* In this loop we only read the acquisition status to minimise overhead.
         * If a transition from acquiring to done is detected then we read the statistics
         * and the data. */
        acquiring = dataAcquiring();
        if (!acquiring)
        {
            /* There must have just been a transition from acquiring to not acquiring */
            /* We force a read of the data */
            asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                "%s::%s Detected acquisition stop! Now reading statistics\n",
                driverName, functionName);
            getMcaData();
            getAcquisitionStatistics();
            asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
                "%s::%s Detected acquisition stop! Now reading data\n",
                driverName, functionName);

        }

        /* Do callbacks for all boards for everything except mcaAcquiring*/
        callParamCallbacks();
        /* Copy internal acquiring flag to mcaAcquiring */
        getIntegerParam(KetekAcquiring, &acquiring);
        setIntegerParam(mcaAcquiring, acquiring);
        callParamCallbacks();

        setIntegerParam(ADAcquire, acquiring);

        paramStatus |= getDoubleParam(KetekPollTime, &pollTime);
        epicsTimeGetCurrent(&now);
        dtmp = epicsTimeDiffInSeconds(&now, &start);
        sleepTime = pollTime - dtmp;
        if (sleepTime < 0) sleepTime = 0.001;
        this->unlock();
        epicsThreadSleep(sleepTime);
        this->lock();
    }
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
    //asynNDArrayDriver::report(fp, details);
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
