IOC startup script
------------------
The command to configure a Ketek in the startup script is::

  KetekConfig(portName, ketekIP, totalBoards, maxMemory)

``portName`` is the name for the Ketek port driver

``asynIPPort`` is the name of the drvAsynIPPort driver to talk to the Ketek.  
This port must have been previously created in the startup script.
It can use either TCP or UDP.  UDP is recommended, because it is required when using sync mode, and also because TCP mode requires about
a 10 second delay when restarting the IOC.

``hostIP`` is the IP address of the IOC host.  This is used for streaming UDP data to the host in sync mode.

``hostUDPPort`` is UDP port on the IOC host to be used for streaming sync mode data.  3142 is typically used.


