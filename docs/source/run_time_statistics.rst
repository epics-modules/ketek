Run-time statistics
-------------------
These are the records for run-time statistics.

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
     - The elapsed live time in seconds.
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
