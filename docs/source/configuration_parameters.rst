Configuration parameters
------------------------
These records control the configuration of the digital signal processing. The readback (_RBV) values may differ slightly
from the output values because of the discrete nature of the system clocks and MCA bins.

These parameters are contained in ketek.template.

.. cssclass:: table-bordered table-striped table-hover
.. list-table::
   :header-rows: 1
   :widths: auto

   * - EPICS record names
     - Record types
     - drvInfo string
     - Description
   * - NumMCAChannels, NumMCAChannels_RBV
     - mbbo, mbbi
     - MCA_NUM_CHANNELS
     - The number of MCA channels to use.  Choices are 512, 1024, 2048, 4096, and 8192.
   * - BytesPerBin, BytesPerBin_RBV
     - mbbo, mbbi
     - KetekBytesPerBin
     - The number of bytes per MCA bin.  Choices are 1, 2, or 3.
   * - EnergyGain, EnergyGain_RBV
     - ao, ai
     - KetekEnergyGain
     - The gain which controls the number of ADC units per MCA bin. Range is 0 to 100%.
   * - EnergyOffset, EnergyOffset_RBV
     - ao, ai
     - KetekEnergyOffset
     - Offset for the energy to MCA bin calibration. Range is +-256 bins.
   * - FastPeakingTime, FastPeakingTime_RBV
     - ao, ai
     - KetekFastPeakingTime
     - The peaking time of the fast filter in microseconds.
   * - FastGapTime, FastGapTime_RBV
     - ao, ai
     - KetekFastGapTime
     - The gap time of the fast filter in microseconds.
   * - FastThreshold, FastThreshold_RBV
     - ao, ai
     - KetekFastFilterThreshold
     - The fast filter threshold in arbiratry units in the range 0-16384.
   * - FastMaxWidth, FastMaxWidth_RBV
     - ao, ai
     - KetekFastFilterMaxWidth
     - The maximum width of the fast filter in microseconds.
   * - MediumPeakingTime_RBV
     - ai
     - KetekMediumPeakingTime
     - The peaking time of the medium filter in microseconds.
   * - MediumGapTime_RBV
     - ai
     - KetekMediumGapTime
     - The gap time of the medium filter in microseconds.
   * - MediumThreshold, MediumThreshold_RBV
     - ao, ai
     - KetekMediumFilterThreshold
     - The medium filter threshold in arbiratry units in the range 0-16384.
   * - MediumFilterEnable, MediumFilterEnable_RBV
     - bo, bi
     - KetekMediumFilterEnable
     - Disable (0) or Enable (1) the medium filter.
   * - MediumMaxWidth, MediumMaxWidth_RBV
     - ao, ai
     - KetekMediumFilterMaxWidth
     - The maximum width of the medium filter in microseconds.
   * - SlowPeakingTime, SlowPeakingTime_RBV
     - ao, ai
     - KetekSlowPeakingTime
     - The peaking time of the slow filter in microseconds.
   * - SlowGapTime, SlowGapTime_RBV
     - ao, ai
     - KetekSlowGapTime
     - The gap time of the slow filter in microseconds.
   * - ResetInhibitTime, ResetInhibitTime_RBV
     - ao, ai
     - KetekResetInhibitTime
     - The time to wait after a reset event, range is 0-3.1875 microseconds.
   * - DynResetEnable, DynResetEnable_RBV
     - bo, bi
     - KetekDynResetEnable
     - Disable (0) or Enable (1) the dynamic reset.
   * - DynResetThreshold, DynResetThreshold_RBV
     - longout, longin
     - KetekDynResetThreshold
     - Dynamic reset threshold, range is 0-65535 ADC units.
   * - DynResetDuration, DynResetDuration_RBV
     - ao, ai
     - KetekDynResetDuration
     - Dynamic reset duration, range is 0-819.19 microseconds.
   * - BaselineAverageLen, BaselineAverageLen_RBV
     - mbbo, mbbi
     - KetekBaselineAverageLen
     - The number of baseline samples. Choices are 2, 4, 8, 16, 32, 64, 128, and 256.
   * - BaselineTrim, BaselineTrim_RBV
     - mbbo, mbbi
     - KetekBaselineTrim
     - The number of baseline trim. Choices are Longest, Long, Medium, Short, and Shortest.
   * - BaselineCorrEnable, BaselineCorrEnable_RBV
     - bo, bi
     - KetekMediumFilterEnable
     - Disable (0) or Enable (1) the baseline correction.
