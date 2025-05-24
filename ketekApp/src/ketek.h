#ifndef KETEK_H
#define KETEK_H

#include <epicsEvent.h>

typedef struct ketekRequest {
    char paramID;
    char command;
    epicsUInt16 data;
} ketekRequest_t;

typedef struct ketekResponse {
    char paramID;
    char status;
    epicsUInt16 data;
} ketekResponse_t;

typedef enum ketekPresetModes {
    KetekPresetNone,
    KetekPresetLiveTime,
    KetekPresetRealTime,
    KetekPresetInputCts,
    KetekPresetOutputCts
} ketekPresetModes_t;

#define KETEK_COMMAND_READ     0
#define KETEK_COMMAND_WRITE    1
#define KETEK_MAX_MCA_BINS     8192
#define KETEK_MAX_SCOPE_POINTS 8192

typedef enum ketekParameters {
    pRunStart             = 0,
    pRunStop              = 1,
    pStopCondition        = 2,
    pStopValueLow         = 3,
    pStopValueHigh        = 4,
    pRunStatus            = 5,
    pRealtimeLow          = 6,
    pRealtimeHigh         = 7,
    pLivetimeLow          = 8,
    pLivetimeHigh         = 9,
    pOutputCountsLow      = 10,
    POutputCountsHigh     = 11,
    pInputCountsLow       = 12,
    pInputCountsHigh      = 13,
    pOutputCountRateLow   = 14,
    POutputCountRateHigh  = 15,
    pInputCountRateLow    = 16,
    pInputCountRateHigh   = 17,
    pRunStatistics        = 18,
    pMCARead              = 19,
    pMCANumBins           = 20,
    pMCABytesPerBin       = 21,
    pFastPeakingTime      = 32,
    pFastGapTime          = 33,
    pMediumPeakingTime    = 34,
    pMediumGapTime        = 35,
    pSlowPeakingTime      = 36,
    pSlowGapTime          = 37,
    pFastTrigThreshold    = 38,
    pMediumTrigThreshold  = 39,
    pMediumTriggerEnable  = 40,
    pFastMaxWidth         = 41,
    pMediumMaxWidth       = 42,
    pResetInhibitTime     = 43,
    pBaselineAverageLen   = 44,
    pBaselineTrim         = 45,
    pBaselineCorrEnable   = 46,
    pDigitalEnergyGain    = 47,
    pDigitalEnergyOffset  = 48,
    pDynResetEnable       = 49,
    pDynResetThreshold    = 50,
    pDynResetDuration     = 51,
    pParamSetLoad         = 64,
    pParamSetSave         = 65,
    pFirmwareMajor        = 66,
    pFirmwareMinor        = 67,
    pFirmwarePatch        = 68,
    pFirmwareBuild        = 69,
    pFirmwareVariant      = 70,
    pMCUPassthrough       = 71,
    pMCUStatus            = 72,
    pBoardTemperature     = 73,
    pAnalogPowerdown      = 74,
    pClockingSpeed        = 75,
    pReadAllParams        = 79,
    pEventTriggerSource   = 80,
    pEventTriggerValue    = 81,
    pEventSampInterval    = 82,
    pEventTriggerTimeout  = 83,
    pEventScopeGet        = 84,
    pEventRateCalc        = 85,
    pEventRateLow         = 86,
    pEventRateHigh        = 87,
    pKeyRevision          = 90,
    pDeleteFirmware       = 91,
    pWriteFirmareSection  = 92,
    pReadFirmwareSection  = 93,
    pServiceCodeLow       = 94,
    pServiceCodeHigh      = 95,
    pEthernetPowerdown    = 96,
    pEthernetProtocol     = 97,
    pEthernetSpeed        = 98,
    pIPAddr01             = 100,
    pIPAddr23             = 101,
    pSubnetMask01         = 102,
    pSubnetMask23         = 103,
    pGatewayIP01          = 104,
    pGatewayIP23          = 105,
    pEthernetPort         = 106,
    pMACAddr01            = 107,
    pMACAddr23            = 108,
    pMACAddr45            = 109,
    pEthernetReconfig     = 110,
    pUSBPowerdown         = 115,
    pSPIPowerdown         = 118
} ketekParameters_t;

/* General parameters */
#define KetekPortNameSelfString             "KetekPortNameSelf"
#define KetekDriverVersionString            "KetekDriverVersion"
#define KetekModelString                    "KetekModel"
#define KetekFirmwareVersionString          "KetekFirmwareVersion"

/* Internal asyn driver parameters */
#define KetekErasedString                   "KetekErased"
#define KetekAcquiringString                "KetekAcquiring"  /* Internal use only !!! */
#define KetekPollTimeString                 "KetekPollTime"

/* Event and scope parameters */
#define KetekEventTriggerSourceString       "KetekEventTriggerSource"
#define KetekEventTriggerValueString        "KetekEventTriggerValue"
#define KetekEventRateCalculateString       "KetekEventRateCalculate"
#define KetekEventRateString                "KetekEventRate"
#define KetekScopeDataSourceString          "KetekScopeDataSource"
#define KetekScopeTimeArrayString           "KetekScopeTimeArray"
#define KetekScopeIntervalString            "KetekScopeInterval"
#define KetekScopeTriggerTimeoutString      "KetekScopeTriggerTimeout"
#define KetekScopeReadString                "KetekScopeRead"

/* Runtime statistics */
#define KetekInputCountRateString           "KetekInputCountRate"
#define KetekOutputCountRateString          "KetekOutputCountRate"
#define KetekInputCountsString              "KetekInputCounts"
#define KetekOutputCountsString             "KetekOutputCounts"

/* Preset parameters */
#define KetekPresetInputCountsString        "KetekPresetInputCounts"
#define KetekPresetOutputCountsString       "KetekPresetOutputCounts"
#define KetekPresetModeString               "KetekPresetMode"

/* Configuration parameters */
#define KetekFastPeakingTimeString          "KetekFastPeakingTime"
#define KetekFastGapTimeString              "KetekFastGapTime"
#define KetekMediumPeakingTimeString        "KetekMediumPeakingTime"
#define KetekMediumGapTimeString            "KetekMediumGapTime"
#define KetekSlowPeakingTimeString          "KetekSlowPeakingTime"
#define KetekSlowGapTimeString              "KetekSlowGapTime"
#define KetekFastFilterThresholdString      "KetekFastFilterThreshold"
#define KetekMediumFilterThresholdString    "KetekMediumFilterThreshold"
#define KetekMediumFilterEnableString       "KetekMediumFilterEnable"
#define KetekFastFilterMaxWidthString       "KetekFastFilterMaxWidth"
#define KetekMediumFilterMaxWidthString     "KetekMediumFilterMaxWidth"
#define KetekResetInhibitTimeString         "KetekResetInhibitTime"
#define KetekBaselineAverageLenString       "KetekBaselineAverageLen"
#define KetekBaselineTrimString             "KetekBaselineTrim"
#define KetekBaselineCorrEnableString       "KetekBaselineCorrEnable"
#define KetekEnergyGainString               "KetekEnergyGain"
#define KetekEnergyOffsetString             "KetekEnergyOffset"
#define KetekBoardTemperatureString         "KetekBoardTemperature"

/* Other parameters */
#define KetekInputModeString                "KetekInputMode"
#define KetekAnalogOffsetString             "KetekAnalogOffset"
#define KetekGatingModeString               "KetekGatingMode"
#define KetekMappingPointsString            "KetekMappingPoints"

class Ketek : public asynPortDriver
{
public:
    Ketek(const char *portName, const char *ipAddress);

    /* virtual methods to override from asynNDArrayDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn);
    void report(FILE *fp, int details);

    /* Local methods to this class */
    void shutdown();
    void acquisitionTask();
    void ketekCallback(uint16_t type, uint32_t call_id, uint32_t length, uint32_t* data);

protected:

    /* General parameters */
    #define FIRST_KETEK_PARAM KetekPortNameSelf
    /* Internal asyn driver parameters */
    int KetekPortNameSelf;
    int KetekDriverVersion;
    int KetekModel;
    int KetekFirmwareVersion;
    int KetekErased;               /** < Erased flag. (0=not erased; 1=erased) */

    /* Event scope and rate parameters */
    int KetekEventTriggerSource;   /** < Source of the event or scope data int32 */
    int KetekEventTriggerValue;    /** < Event trigger value int32 */
    int KetekEventRateCalculate;   /** < Time period to calculate rate int32 */
    int KetekEventRate;            /** < Event rate int32 */
    int KetekScopeDataSource;      /** < Source of scope data int32 array */
    int KetekScopeData;            /** < Scope array data (read) int32 array */
    int KetekScopeTimeArray;       /** < Scope timebase array (read) float64 array */
    int KetekScopeInterval;        /** < Scope time per point, float64 */
    int KetekScopeTriggerTimeout;  /** < Scope trigger timeout int32 */
    int KetekScopeRead;            /** < Read the scope data int32 */

    /* Runtime statistics */
    int KetekInputCountRate;      /* float64 */
    int KetekOutputCountRate;     /* float64 */
    int KetekInputCounts;         /* uint32 */
    int KetekOutputCounts;        /* uint32 */

    /* Preset parameters */
    int KetekPresetInputCounts;   /** < Preset input counts int32 */
    int KetekPresetOutputCounts;  /** < Preset output counts int32 */
    int KetekPresetMode;          /** < Preset mode int32 */

    /* Configuration parameters */
    int KetekFastPeakingTime;             /* float64, usec */
    int KetekFastGapTime;                 /* float64, usec */
    int KetekMediumPeakingTime;           /* float64, usec */
    int KetekMediumGapTime;               /* float64, usec */
    int KetekSlowPeakingTime;             /* float64, usec */
    int KetekSlowGapTime;                 /* float64, usec */
    int KetekFastFilterThreshold;         /* float64, keV */
    int KetekMediumFilterThreshold;       /* float64, keV */
    int KetekMediumFilterEnable;          /* uint32 */
    int KetekFastFilterMaxWidth;          /* float64, keV */
    int KetekMediumFilterMaxWidth;        /* float64, keV */
    int KetekResetInhibitTime;            /* float64, usec */
    int KetekBaselineAverageLen;          /* uint32 */
    int KetekBaselineTrim;                /* uint32 */
    int KetekBaselineCorrEnable;          /* uint32 */
    int KetekEnergyGain;                  /* float64, % */
    int KetekEnergyOffset;                /* float64, bins */
    int KetekBoardTemperature;            /* float64, C */

    /* Commands from MCA interface */
    int mcaData;                   /* int32Array, write/read */
    int mcaStartAcquire;           /* int32, write */
    int mcaStopAcquire;            /* int32, write */
    int mcaErase;                  /* int32, write */
    int mcaReadStatus;             /* int32, write */
    int mcaChannelAdvanceSource;   /* int32, write */
    int mcaNumChannels;            /* int32, write */
    int mcaAcquireMode;            /* int32, write */
    int mcaSequence;               /* int32, write */
    int mcaPrescale;               /* int32, write */
    int mcaPresetSweeps;           /* int32, write */
    int mcaPresetLowChannel;       /* int32, write */
    int mcaPresetHighChannel;      /* int32, write */
    int mcaDwellTime;              /* float64, write/read */
    int mcaPresetLiveTime;         /* float64, write */
    int mcaPresetRealTime;         /* float64, write */
    int mcaPresetCounts;           /* float64, write */
    int mcaAcquiring;              /* int32, read */
    int mcaElapsedLiveTime;        /* float64, read */
    int mcaElapsedRealTime;        /* float64, read */
    int mcaElapsedCounts;          /* float64, read */


private:
    asynStatus setPresets();
    asynStatus getPresets();
    asynStatus setConfiguration();
    asynStatus getConfiguration();
    asynStatus getAcquisitionStatistics();
    asynStatus getMcaData();
    asynStatus getTraces();
    asynStatus startAcquiring();
    asynStatus stopAcquiring();
    asynStatus readSingleParam(int paramID, unsigned short *value);
    asynStatus writeSingleParam(int paramID, int value);
    asynStatus writeStopValue(epicsUInt32 value);
    asynStatus sendRcvMsg(ketekRequest_t *request, void *response, size_t responseSize);

    /* Data */
    epicsInt32 mcaData_[KETEK_MAX_MCA_BINS];
    epicsUInt8 mcaRaw_[KETEK_MAX_MCA_BINS*sizeof(epicsUInt32)];
    epicsInt32 scopeData_[KETEK_MAX_SCOPE_POINTS];
    epicsUInt8 scopeDataRaw_[KETEK_MAX_SCOPE_POINTS*sizeof(epicsUInt32)];
    epicsFloat64 scopeTimeBuffer_[KETEK_MAX_SCOPE_POINTS];

    asynUser *pasynUserRemote_;

    epicsEvent *cmdStartEvent_;
    epicsEvent *cmdStopEvent_;
    epicsEvent *stoppedEvent_;


};
#ifdef __cplusplus
extern "C"
{
#endif

int ketekConfig(const char *portName, const char *ipPortName);

#ifdef __cplusplus
}
#endif

#endif /* KETEK_H */
