Performance
-----------

Ketek externally triggered sync mode
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following table shows the maximum number of pixels/s for sync/MCA mapping mode as a function of the the number of MCA channels
in the spectrum [512, 1024, 2048, 4096, 8192] and the number of BytesPerBin [1, 2, 3]. The tests were done under the following conditions:

- SyncPoints = 1000
- ArrayCallbacks = Enable
- WaitForPlugins = Yes
- SyncCycleTime = 0.0005

The Ketek was triggered by an external programmable pulse generator.  The pulse duty cycle was 50%.
The pulse generator was programmed to output 1000 pulses.

The pulse frequency was increased until the sync mode acquisition no longer collected the requested number of pixels.

.. cssclass:: table-bordered table-striped table-hover
.. list-table:: Maximum cycle rate in Hz (spectra/second)
   :header-rows: 1
   :widths: auto

   * - MCA Channels
     - BytesPerBin=1
     - BytesPerBin=2
     - BytesPerBin=3
   * - 512
     - 1200
     - 1550
     - 1000
   * - 1024
     - 900
     - 500
     - 400
   * - 2048
     - 500
     - 350
     - 300
   * - 4096
     - 350
     - 225
     - 160
   * - 8192
     - 225
     - 120
     - 120
