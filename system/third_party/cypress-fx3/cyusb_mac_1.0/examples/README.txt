================================================================================
                       Cypress Semiconductor Corporation
                             CyUSB Suite For MAC
================================================================================

In order to compile the example source files, change to the /Examples directory
in a terminal and invoke the "make" command.

The following sample applications are provided in this CyUSB Suite release:
1. download_fx3.c
2. download_fx2.c
3. cybulk_reader.c
4. cybulk_writer.c
5. cyisowrite_test.c
6. cyisoread_test.c
7. cybulkread_performance.c
8. cybulkwrite_performance.c

Note: The examples were tested on a MAC Pro running OS X 10.8.

================================================================================
Example Name: download_fx3
================================================================================

Description:
This program downloads a EZ-USB FX3 firmware binary to the device RAM, I2C
EEPROM or SPI Flash memory.

Usage:
./download_fx3 -i <Path to fX3 image> -t [RAM|I2C|SPI]

e.g.: To download bulk loop firmware to the FX3 device RAM, call:
      ./download_fx3 -i ../fx3_images/cyfxbulklpautoenum.img -t RAM

Please note that the CYUSB_ROOT environment variable should be set to point
to the root folder where this CyUSB suite package is installed.

The FX3 device supports firmware download at USB 2.0 speeds only.
Refer to Programmer's manual for further details on the APIs used.

================================================================================
Example Name: download_fx2
================================================================================

Description:
This program downloads a EZ-USB FX2LP firmware binary to the RAM or I2C EEPROM.
Firmware download to internal and external RAM is supported. Downloading a USB
configuration (VID/PID) file to a small EEPROM is also supported.

Usage:
./download_fx2 -i <Path to fX2 image> -t [RAM|SI2C|LI2C]

e.g.: To download the bulk loop firmware onto the FX2LP device RAM, call:
      ./download_fx2 ../fx2_images/bulkloop.hex -t RAM

Please note that only IIC binaries generated using the hex2bix tool should
be downloaded to the EEPROM.

================================================================================
Example Name: cybulk_writer.c
================================================================================

Description:
This example does a bulk write from USB host to a FX2LP / FX3 device.
This example works at USB SuperSpeed, Hi-Speed and Full Speed.

Usage:
./cybulk_writer <Num of bytes to write>

e.g.: To write 512 bytes of data to the first BULK OUT endpoint found, call:
      ./cybulk_writer 512

Note: Make sure that the proper firmware is loaded prior to running this binary.

================================================================================
Example Name: cybulk_reader.c
================================================================================

Description:
This sample reads the specified amount of data from a BULK IN endpoint on the
targeted FX2LP/FX3 device. This example works at USB SuperSpeed, Hi-Speed and
Full Speed.

Usage:
./cybulk_reader <Num of bytes to read>

e.g.: To read a packet with a maximum of 512 bytes from the first BULK IN
      endpoint found, call:
      ./cybulk_reader 512

Note: Make sure that the proper firmware is loaded on the device before using
      this application. Also ensure that the size of the data read specified
      is an integral multiple of the maximum packet size for the endpoint.

================================================================================
Example Name: cyisowrite_test.c
================================================================================

Description:
Sends the specified number of full data packets to an Isochronous OUT endpoint
on the FX2LP/FX3 device. After transfer is complete, the application reports
the number of packets that were sent successfully, and the number that
failed.

Usage:
./cyisowrite_test <Number of packets to be sent>

e.g.: To try to send 64 packets to the first ISO OUT endpoint, call:
      ./cyisowrite_test 64

Note: The libcyusb.dylib library does not provide APIs for ISO data transfers.
      This application directly makes use of libusb functions to perform the
      transfers.

================================================================================
Example Name: cyisoread_test.c
================================================================================

Description:
Tries to receive the specified number of data packets from an Isochronous IN
endpoint on the FX2LP/FX3 device. After transfer is complete, the application
reports the number of packets that were received successfully, and the number
that failed.

Usage:
./cyisoread_test <Number of packets to be read>

e.g.: To try to read 64 packets of data from the first ISO IN endpoint, call:
      ./cyisoread_test 64

================================================================================
Example Name: cybulkread_performance.c
================================================================================

Description:
This example helps in doing performance analysis of BULK IN transfers from
FX2LP/FX3 devices. The application tries to read records of size 4MB and takes
the number of such records to be read as an input parameter. The total time
taken for transfer is displayed at the end, allowing the performance to be
computed.

Usage:
./cybulkread_performance <Number of 4Mb records to be read>

e.g.: To try to read 400 MB of data (100 records of 4 MB each), call:
      ./cybulkread_performance 100

Note: Ensure that the firmware application on the device is capable of
      sending the required amount of data without any other interaction.
      The BULK Src/Sink applications can be used to ensure this.

================================================================================
Example Name: cybulkwrite_performance.c
================================================================================

Description:
This example helps in doing performance analysis of BULK OUT transfers to
FX2LP/FX3 devices. The application tries to send records of size 4MB and takes
the number of such records to be written as an input parameter. The total time
taken for transfer is displayed at the end, allowing the performance to be
computed.

Usage:
./cybulkwrite_performance <Number of 4Mb records to be written>

e.g.: To try to send 400 MB of data (100 records of 4 MB each), call:
      ./cybulkwrite_performance 100

Note: Ensure that the firmware application on the device is capable of
      receiving the required amount of data without any other interaction.
      The BULK Src/Sink applications can be used to ensure this.

