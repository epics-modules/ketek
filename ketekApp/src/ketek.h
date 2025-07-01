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

typedef enum ketekSyncModes {
    KetekSyncNone,
    KetekSyncMXMaster,
    KetekSyncMXSlave,
    KetekSyncSortFreeSlave,
    KetekSyncMappingMode
} ketekSyncModes_t;

#define KETEK_COMMAND_READ     0
#define KETEK_COMMAND_WRITE    1
#define KETEK_MAX_MCA_BINS     8192
#define KETEK_MAX_SCOPE_POINTS 8192
#define KETEK_MAX_UDP_LEN      1460
#define KETEK_MAX_BYTES_PER_BIN 3

#define STATUS_DATA_CHANNEL_INDEX   0x01
#define STATUS_DATA_FIRMWARE_INFO   0x02
#define STATUS_DATA_SYNC_MODE       0x04
#define STATUS_DATA_SYNC_TIME       0x08
#define STATUS_DATA_MCU_STATUS      0x10
#define STATUS_DATA_BOARD_TEMP      0x20

#define RUN_DATA_SYNC_RUN_ACTIVE    0x01
#define RUN_DATA_REAL_TIME          0x02
#define RUN_DATA_LIVE_TIME          0x04
#define RUN_DATA_OUTPUT_COUNTS      0x08
#define RUN_DATA_INPUT_COUNTS       0x10
#define RUN_DATA_OCR                0x20
#define RUN_DATA_ICR                0x40
#define RUN_DATA_COUNTER_INTERNAL   0x80
#define RUN_DATA_COUNTER_MASTER     0x100
#define RUN_DATA_CYCLE_ERROR_COUNT  0x200
#define RUN_DATA_SYNC_COM_ERROR     0x400


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
    pScopeSampInterval    = 82,
    pScopeTriggerTimeout  = 83,
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
    pSPIPowerdown         = 118,
    pSyncStart            = 128,
    pSyncStop             = 129,
    pSyncStopConditionLow = 130,
    pSyncStopConditionHigh  = 131,
    pSyncRunActive        = 132,
    pSyncEtherHigh        = 133,
    pSyncEtherLow         = 134,
    pSyncEtherPort        = 135,
    pSyncSegSize          = 138,
    pSyncSegNumber        = 139,
    pSyncStatusData       = 140,
    pSyncRuntimeData      = 141,
    pSyncSpectrumData     = 142,
    pSyncChannelIndex     = 143,
    pSyncMode             = 144,
    pSyncCycleTimeLow     = 145,
    pSyncCycleTimeHigh    = 146,
    pSyncCycleErrorCount  = 147,
    pSyncComError         = 148,
    pSyncCycleInternal    = 149,
    pSyncCycleMaster      = 150,
    pSyncReset            = 151,
    pSyncAutoStart        = 152,
    pSyncTestPattern      = 153
} ketekParameters_t;

/* Internal asyn driver parameters */
#define KetekErasedString                   "KetekErased"
#define KetekAcquiringString                "KetekAcquiring"  /* Internal use only !!! */
#define KetekPollTimeString                 "KetekPollTime"

/* Event and scope parameters */
#define KetekEventTriggerSourceString       "KetekEventTriggerSource"
#define KetekEventTriggerValueString        "KetekEventTriggerValue"
#define KetekEventRatePeriodString          "KetekEventRatePeriod"
#define KetekEventRateMeasureString         "KetekEventRateMeasure"
#define KetekEventRateString                "KetekEventRate"
#define KetekScopeDataSourceString          "KetekScopeDataSource"
#define KetekScopeDataString                "KetekScopeData"
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
#define KetekCollectModeString              "KetekCollectMode"

/* Sync parameters */
#define KetekSyncAcquireString              "KetekSyncAcquire"
#define KetekSyncCycleTimeString            "KetekSyncCycleTime"
#define KetekSyncPointsString               "KetekSyncPoints"
#define KetekSyncCurrentPointString         "KetekSyncCurrentPoint"
#define KetekSyncEnabledString              "KetekSyncEnabled"
#define KetekSyncRunningString              "KetekSyncRunning"

class Ketek : public asynNDArrayDriver
{
public:
    Ketek(const char *portName, const char *ipPortName, const char* hostIPAddress, int hostUDPPort);

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
    int KetekEventRatePeriod;      /** < Time period to calculate rate int32 */
    int KetekEventRateMeasure;     /** < Measure the event rate int32 */
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

    /* Sync parameters */
    int KetekSyncAcquire;                 /* int32, write */
    int KetekSyncCycleTime;               /* float64, write */
    int KetekSyncPoints;                  /* int32, write */
    int KetekSyncCurrentPoint;            /* int32, read */
    int KetekSyncEnabled;                 /* int32 read */
    int KetekSyncRunning;                 /* int32 read */

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
    asynStatus setEventScope();
    asynStatus startEventRate();
    asynStatus getAcquisitionStatistics();
    asynStatus getMcaData();
    asynStatus getScopeTrace();
    asynStatus startAcquiring();
    asynStatus stopAcquiring();
    asynStatus startSyncAcquire();
    asynStatus stopSyncAcquire();
    asynStatus readSingleParam(int paramID, unsigned short *value);
    asynStatus writeSingleParam(int paramID, int value);
    asynStatus writeStopValue(epicsUInt32 value);
    asynStatus sendRcvMsg(ketekRequest_t *request, void *response, size_t responseSize, double timout);
    epicsUInt16 extractSyncParam(int offset, int paramID);
    epicsFloat64 extractDoubleSyncParam(int offset, int paramID);

    /* Data */
    epicsInt32 mcaData_[KETEK_MAX_MCA_BINS];
    epicsUInt8 mcaRaw_[KETEK_MAX_MCA_BINS*sizeof(epicsUInt32)];
    epicsInt32 scopeData_[KETEK_MAX_SCOPE_POINTS];
    epicsUInt8 scopeDataRaw_[KETEK_MAX_SCOPE_POINTS*3];
    epicsFloat64 scopeTimeBuffer_[KETEK_MAX_SCOPE_POINTS];
    epicsUInt8 syncMsgBuffer_[KETEK_MAX_UDP_LEN];
    epicsUInt8 syncRawMCABuffer_[KETEK_MAX_MCA_BINS*KETEK_MAX_BYTES_PER_BIN];

    asynUser *pasynUserRemote_;
    asynUser *pasynUserSync_;

    epicsEvent *cmdStartEvent_;
    epicsEvent *cmdStopEvent_;
    epicsEvent *stoppedEvent_;
    
    int uniqueId_;
    bool polling_;


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
