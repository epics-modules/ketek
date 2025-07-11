Sync or MCA mapping mode
------------------------
The Ketek can collect spectra in what they call "sync" mode.  For other detectors this is generally called MCA Mapping Mode.
It is used to rapidly collect spectra in response to an external trigger signal that is provided to the TRIG input on the AXAS 3.0.

In sync mode the Ketek streams data to a UDP port on the IOC computer.  The IP address of the IOC and the UDP port to use are parameters
to the KetekConfigure command in the startup script.
   
The driver reads the spectra and metadata from the UDP port and copies it into NDArrays of dimensions [NumMCAChannels]. 
The run-time statistics for each spectrum are copied into NDAttributes attached to each
NDArray.  The data type of the NDArray depends on the value of the BytesPerBin configuration parameter.
It will be NDUInt8, NDUInt16, or NDUInt32 for BytesPerBin = 1, 2, or 3.

The data can be displayed in the MCA record when a sync mode run is in progress.
This is done by setting SyncReadMCA.SCAN to a periodic scan rate.

The NDArrays can be used by any of the standard areaDetector plugins.  For example, they can be streamed
to HDF5, netCDF, or TIFF files.

These are the records for Sync/MCA Mapping Mode.  They are contained in ketek.template.

.. cssclass:: table-bordered table-striped table-hover
.. list-table::
   :header-rows: 1
   :widths: auto

   * - EPICS record names
     - Record types
     - drvInfo string
     - Description
   * - SyncAcquire, SyncAcquire_RBV
     - busy, bi
     - KetekSyncAcquire
     - Writing 1 to this record start a sync mode acquisition, and writing 0 stops the acquisition.
   * - SyncCycleTime, SyncCycleTime_RBV
     - ao, ai
     - KetekSyncCycleTime
     - The acquisition time in sync mode.  This is the time that the Ketek will acquire for after receiving a trigger pulse.
       The time between external trigger pulses must be less than this value.
   * - SyncPoints, SyncPoints_RBV
     - longout, longin
     - KetekSyncPoints
     - The number of spectra to collect in sync mode.  The driver will stop acquisition automatically after this number of spectra
       have been collected.
   * - SyncCurrentPoint
     - longin
     - KetekSyncCurrentPoint
     - The current sync cycle number, i.e. the number of spectra collected so far.
   * - SyncEnabled
     - bi
     - KetekSyncEnabled
     - A flag that indicates that sync mode is correctly configured to stream data to the IOC machine over UDP.
   * - SyncRunning
     - bi
     - KetekSyncRunning
     - A flag that indicates that sync mode is currently active.  This is read from the hardware.
   * - SyncReadMCA
     - bo
     - N.A.
     - This record can be periodically scanned to cause the MCA record to display the sync mode spectra.  This is useful for monitoring
       the MCA spectra during a sync mode acquisition.


The following is the MEDM screen ketek.adl when the Ketek is collecting in sync mode.

.. figure:: ketek_sync_mode.png
    :align: center


The following is the MEDM screen NDFileHDF5.adl when the Ketek is saving sync mode data to an HDF5 file.

.. figure:: ketek_sync_hdf5.png
    :align: center


The following is the MEDM screen mca.adl when the Ketek is collecting in sync mode and SyncReadMCA is processing the MCA record at 1 Hz.

.. figure:: ketek_sync_mca.png
    :align: center
