#ifndef KETEK_H
#define KETEK_H

#include <vector>
#include <epicsEvent.h>
#include <epicsMessageQueue.h>

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
#define KETEK_MAX_REPLY_LEN    16

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
#define KetekCollectModeString              "KetekCollectMode"
#define KetekCurrentPixelString             "KetekCurrentPixel"
#define KetekMaxEnergyString                "KetekMaxEnergy"
#define KetekSpectrumXAxisString            "KetekSpectrumXAxis"

/* Internal asyn driver parameters */
#define KetekErasedString                   "KetekErased"
#define KetekAcquiringString                "KetekAcquiring"  /* Internal use only !!! */
#define KetekPollTimeString                 "KetekPollTime"

/* Diagnostic trace parameters */
#define KetekTraceDataString                "KetekTraceData"
#define KetekTraceTimeArrayString           "KetekTraceTimeArray"
#define KetekTraceTimeString                "KetekTraceTime"
#define KetekTraceTriggerInstantString      "KetekTraceTriggerInstant"
#define KetekTraceTriggerRisingString       "KetekTraceTriggerRising"
#define KetekTraceTriggerFallingString      "KetekTraceTriggerFalling"
#define KetekTraceTriggerLevelString        "KetekTraceTriggerLevel"
#define KetekTraceTriggerWaitString         "KetekTraceTriggerWait"
#define KetekTraceLengthString              "KetekTraceLength"
#define KetekReadTraceString                "KetekReadTrace"

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

class Ketek : public asynNDArrayDriver
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
    int KetekCollectMode;                   /** < Change mapping mode (0=mca; 1=spectra mapping; 2=sca mapping) (int32 read/write) addr: all/any */
    #define FIRST_KETEK_PARAM KetekCollectMode
    int KetekCurrentPixel;                  /** < Mapping mode only: read the current pixel that is being acquired into (int) */
    int KetekMaxEnergy;
    int KetekSpectrumXAxis;

    /* Internal asyn driver parameters */
    int KetekErased;               /** < Erased flag. (0=not erased; 1=erased) */
    int KetekAcquiring;            /** < Internal acquiring flag, not exposed via drvUser */
    int KetekPollTime;             /** < Status/data polling time in seconds */
    int KetekForceRead;            /** < Force reading MCA spectra - used for mcaData when addr=ALL */
    int KetekEnableBoard;          /** < Enable/disable specific board */
    int KetekEnableConfigure;      /** < Enable/disable calling configure() when a parameter changes */

    /* Diagnostic trace parameters */
    int KetekTraceData;            /** < The trace array data (read) int32 array */
    int KetekTraceTimeArray;       /** < The trace timebase array (read) float64 array */
    int KetekTraceTime;            /** < The trace time per point, float64 */
    int KetekTraceTriggerInstant;  /** < Trigger instant int32 */
    int KetekTraceTriggerRising;   /** < Trigger rising crossing of trigger level int32 */
    int KetekTraceTriggerFalling;  /** < Trigger falling crossing of trigger level int32 */
    int KetekTraceTriggerLevel;    /** < Trigger level (0 - 65535) int32 */
    int KetekTraceTriggerWait;     /** < Time to wait for trigger float64 */
    int KetekTraceLength;          /** < Length of trace, multiples of 16K int32 */
    int KetekReadTrace;            /** < Command to read the trace data */

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

    /* Other parameters */
    int KetekInputMode;                   /* int32 */
    int KetekAnalogOffset;                /* int32, 8-bit DAC units */
    int KetekGatingMode;                  /* int32 */
    int KetekMappingPoints;               /* int32 */
    int KetekListBufferSize;              /* int32 */
    int KetekKeepAlive;                   /* int32 */

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
    asynStatus setKetekConfiguration();
    bool dataAcquiring();
    bool waveformAcquiring();
    asynStatus getAcquisitionStatistics();
    asynStatus getMcaData();
    asynStatus pollMCAMappingMode();
    asynStatus pollListMode();
    asynStatus getTraces();
    asynStatus startAcquiring();
    asynStatus stopAcquiring();
    asynStatus readSingleParam(int paramID, unsigned short *value);
    asynStatus writeSingleParam(int paramID, int value);
    asynStatus writeStopValue(epicsUInt32 value);
    asynStatus sendRcvMsg(ketekRequest_t *request, void *response, size_t responseSize);

    /* Data */
    epicsInt32 mcaData_[KETEK_MAX_MCA_BINS];
    uint16_t **pMappingMCAData_;
    uint32_t **pMappingSpectrumId_;

    int uniqueId_;
    
    asynUser *pasynUserRemote_;
    ketekRequest_t requestMsg_;
    ketekResponse_t responseMsg_;

    epicsEvent *cmdStartEvent_;
    epicsEvent *cmdStopEvent_;
    epicsEvent *stoppedEvent_;
    epicsMessageQueue *msgQ_;

    uint32_t traceLength_;
    bool newTraceTime_;
    uint16_t *traceBuffer_;
    int32_t *traceBufferInt32_;
    epicsFloat64 *traceTimeBuffer_;
    epicsFloat64 *spectrumXAxisBuffer_;

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
