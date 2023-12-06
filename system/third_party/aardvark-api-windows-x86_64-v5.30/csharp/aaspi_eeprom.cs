/*=========================================================================
| (c) 2006-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aaspi_eeprom.cs
|--------------------------------------------------------------------------
| Perform simple read and write operations to an SPI EEPROM device.
|--------------------------------------------------------------------------
| Redistribution and use of this file in source and binary forms, with
| or without modification, are permitted.
|
| THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
| "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
| LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
| FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
| COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
| INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
| BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
| CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
| LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
| ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
| POSSIBILITY OF SUCH DAMAGE.
 ========================================================================*/

using System;
using TotalPhase;


/*=========================================================================
| CLASS
 ========================================================================*/
public class AASpiEeprom {

    /*=====================================================================
    | CONSTANTS
     ====================================================================*/
    public const int PAGE_SIZE = 32;


    /*=====================================================================
    | FUNCTIONS
     ====================================================================*/
    static void _writeMemory (int handle, short addr, short length, bool zero)
    {
        short i, n;
        byte[] dataOut = null;
        byte[] dataIn  = null;
        byte[] writeEnableBuf = new byte[1];

        // Write to the SPI EEPROM
        //
        // The AT25080A EEPROM has 32 byte pages.  Data can written
        // in pages, to reduce the number of overall SPI transactions
        // executed through the Aardvark adapter.
        n = 0;
        while (n < length) {
            // Calculate the amount of data to be written this iteration
            // and make sure dataOut is just large enough for it.
            int size = Math.Min(((addr & (PAGE_SIZE-1)) ^ (PAGE_SIZE-1)) + 1,
                                length - n);
            size += 3;  // Add 3 for the address and command fields.
            if (dataOut == null || dataOut.Length != size) {
                dataOut = new byte[size];
                dataIn  = new byte[size];
            }

            // Send write enable command
            writeEnableBuf[0] = 0x06;
            AardvarkApi.aa_spi_write(handle, (ushort)size, writeEnableBuf,
                                     (ushort)size, dataIn);

            // Assemble write command and address
            dataOut[0] = 0x02;
            dataOut[1] = (byte)((addr >> 8) & 0xff);
            dataOut[2] = (byte)((addr >> 0) & 0xff);

            // Assemble a page of data
            i = 3;
            do {
                dataOut[i++] = zero ? (byte)0 : (byte) n;
                ++addr; ++n;
            } while ( n < length && (addr & (PAGE_SIZE-1)) != 0);

            // Write the transaction
            AardvarkApi.aa_spi_write(handle, (ushort)size, dataOut,
                                     (ushort)size, dataIn);
            AardvarkApi.aa_sleep_ms(10);
        }
    }


    static void _readMemory (int handle, short addr, short length)
    {
        int count;
        int i;

        byte[] dataOut  = new byte[length+3];
        byte[] dataIn   = new byte[length+3];

        // Assemble read command and address
        dataOut[0] = 0x03;
        dataOut[1] = (byte)((addr >> 8) & 0xff);
        dataOut[2] = (byte)((addr >> 0) & 0xff);

        // Write length+3 bytes for data plus command and 2 address bytes
        count = AardvarkApi.aa_spi_write(handle, (ushort)(length+3), dataOut,
                                         (ushort)(length+3), dataIn);

        if (count < 0) {
            Console.WriteLine("error: {0}",
                              AardvarkApi.aa_status_string(count));
        }
        else if (count != length+3) {
            Console.WriteLine("error: read {0} bytes (expected {1})",
                              count-3, length);
        }


        // Dump the data to the screen
        Console.Write("\nData read from device:");
        for (i = 0; i < length; ++i) {
            if ((i&0x0f) == 0)      Console.Write("\n{0:x4}:  ", addr+i);
            Console.Write("{0:x2} ", dataIn[i+3] & 0xff);
            if (((i+1)&0x07) == 0)  Console.Write(" ");
        }
        Console.WriteLine();

    }


    /*=====================================================================
    | MAIN PROGRAM
     ====================================================================*/
    public static void Main (string[] args) {
        int   handle;
        int   port    = 0;
        int   bitrate = 100;
        int   mode    = 0;
        short addr;
        short length;

        if (args.Length != 6) {
            Console.WriteLine(
                "usage: aaspi_eeprom PORT BITRATE read  MODE ADDR LENGTH");
            Console.WriteLine(
                "usage: aaspi_eeprom PORT BITRATE write MODE ADDR LENGTH");
            Console.WriteLine(
                "usage: aaspi_eeprom PORT BITRATE zero  MODE ADDR LENGTH");
            Console.WriteLine("  mode 0 : pol = 0, phase = 0");
            Console.WriteLine("  mode 3 : pol = 1, phase = 1");
            Console.WriteLine("  modes 1 and 2 are not supported");
            return;
        }

        string command = args[2];

        // Parse the port argument
        try {
            port = Convert.ToInt32(args[0]);
        }
        catch (Exception) {
            Console.WriteLine("Error: invalid port number");
            return;
        }

        // Parse the bitrate argument
        try {
            bitrate = Convert.ToInt32(args[1]);
        }
        catch (Exception) {
            Console.WriteLine("Error: invalid bitrate");
            return;
        }

        // Parse the mode argument
        try {
            mode = Convert.ToInt32(args[3]) & 0x3;
        }
        catch (Exception) {
            Console.WriteLine("Error: invalid mode");
            return;
        }

        // Parse the memory offset argument
        try {
            if (args[4].StartsWith("0x"))
                addr = Convert.ToByte(args[4].Substring(2), 16);
            else
                addr = Convert.ToByte(args[4]);
        }
        catch (Exception) {
            Console.WriteLine("Error: invalid memory addr");
            return;
        }

        // Parse the length
        try {
            length = Convert.ToInt16(args[5]);
        }
        catch (Exception) {
            Console.WriteLine("Error: invalid length");
            return;
        }

        if (mode == 1 || mode == 2) {
            Console.WriteLine(
                "error: spi modes 1 and 2 are not supported by the AT25080A");
            return;
        }

        // Open the device
        handle = AardvarkApi.aa_open(port);
        if (handle <= 0) {
            Console.WriteLine("Unable to open Aardvark device on port {0}",
                              port);
            Console.WriteLine("error: {0}",
                              AardvarkApi.aa_status_string(handle));
            return;
        }

        // Ensure that the SPI subsystem is enabled.
        AardvarkApi.aa_configure(handle, AardvarkConfig.AA_CONFIG_SPI_I2C);

        // Power the EEPROM using the Aardvark adapter's power supply.
        // This command is only effective on v2.0 hardware or greater.
        // The power pins on the v1.02 hardware are not enabled by default.
        //aa_target_power(handle, AA_TARGET_POWER_BOTH);
        AardvarkApi.aa_target_power(handle, AardvarkApi.AA_TARGET_POWER_BOTH);

        // Setup the clock phase
        AardvarkApi.aa_spi_configure(handle, (AardvarkSpiPolarity)(mode >> 1),
                                     (AardvarkSpiPhase)(mode & 1),
                                     AardvarkSpiBitorder.AA_SPI_BITORDER_MSB);

        // Set the bitrate
        bitrate = AardvarkApi.aa_spi_bitrate(handle, bitrate);
        Console.WriteLine("Bitrate set to {0} kHz", bitrate);

        // Perform the operation
        if (command == "write") {
            _writeMemory(handle, addr, length, false);
            Console.WriteLine("Wrote to EEPROM");
        }
        else if (command == "read") {
            _readMemory(handle, addr, length);
        }
        else if (command == "zero") {
            _writeMemory(handle, addr, length, true);
            Console.WriteLine("Zeroed EEPROM");
        }
        else {
            Console.WriteLine("unknown command: {0}", command);
        }

        // Close the device
        AardvarkApi.aa_close(handle);

    }

}
