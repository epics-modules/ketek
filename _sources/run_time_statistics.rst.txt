Run-time status and statistics
------------------------------
These are the records for run-time status and statistics.

These parameters are contained in ketek.template.

.. cssclass:: table-bordered table-striped table-hover
.. list-table::
   :header-rows: 1
   :widths: auto

   * - EPICS record names
     - Record types
     - drvInfo string
     - Description
   * - mca1.ERTM
     - mca
     - MCA_ELAPSED_REAL
     - The elapsed real time in seconds.
   * - mca1.ELTM
     - mca
     - MCA_ELAPSED_LIVE
     - The elapsed "slow" live time in seconds.  This is defined as (RealTime * OutputCounts / InputCounts).
       It is effectively the live time of the slow filter.
   * - FastLiveTime
     - ai
     - KetekFastLiveTime
     - The elapsed "fast" live time.  This is reported by the DPP3 system,
       and is defined as "the time in seconds in which the pulse detection was active during the spectrum run
       (pulse detection filter below trigger threshold and no reset detected)".
       It is effectively the live time of the fast filter.
   * - InputCountRate
     - ai
     - KetekInputCountRate
     - The input count rate in Hz.
   * - OutputCountRate
     - ai
     - KetekOutputCountRate
     - The output count rate in Hz.
   * - InputCounts
     - longin
     - KetekInputCounts
     - The number of fast events received.
   * - OutputCounts
     - longin
     - KetekOutputCounts
     - The number of counts in the MCA spectrum.
   * - BoardTemperature
     - ai
     - KetekBoardTemperature
     - The temperature of the DPP3 board.
   * - MCUReady
     - bi
     - KetekMCUReady
     - The status of the MCU, "Ready" or "Not ready. 
       It will be "Not ready" for up to 1 minute after a powering on.
