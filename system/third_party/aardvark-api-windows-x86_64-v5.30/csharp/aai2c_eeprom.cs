/*=========================================================================
| (c) 2006-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aai2c_eeprom.cs
|--------------------------------------------------------------------------
| Perform simple read and write operations to an I2C EEPROM device.
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
public class AAI2cEeprom {

    /*=====================================================================
    | CONSTANTS
     ====================================================================*/
    public const int PAGE_SIZE   = 8;
    public const int BUS_TIMEOUT = 150;  // ms


    /*=====================================================================
    | FUNCTION
     ====================================================================*/
    static void _writeMemory (int handle, byte device, byte addr,
                              short length, bool zero)
    {
        short i, n;
        byte[] dataOut = null;

        // Write to the I2C EEPROM
        //
        // The AT24C02 EEPROM has 8 byte pages.  Data can written
        // in pages, to reduce the number of overall I2C transactions
        // executed through the Aardvark adapter.
        n = 0;
        while (n < length) {
            // Calculate the amount of data to be written this iteration
            // and make sure dataOut is just large enough for it.
            int size = Math.Min(((addr & (PAGE_SIZE-1)) ^ (PAGE_SIZE-1)) + 1,
                                length - n);
            size++;  // Add 1 for the address field
            if (dataOut == null || dataOut.Length != size)
                dataOut = new byte[size];

            // Fill the packet with data
            dataOut[0] = addr;

            // Assemble a page of data
            i = 1;
            do {
                dataOut[i++] = zero ? (byte)0 : (byte)n;
                ++addr; ++n;
            } while ( n < length && (addr & (PAGE_SIZE-1)) != 0 );

            // Write the address and data
            AardvarkApi.aa_i2c_write(handle, device,
                                     AardvarkI2cFlags.AA_I2C_NO_FLAGS,
                                     (ushort)size, dataOut);
            AardvarkApi.aa_sleep_ms(10);
        }
    }


    static void _readMemory (int handle, byte device, byte addr,
                             short length)
    {
        int count, i;
        byte[] dataOut = { addr };
        byte[] dataIn  = new byte[length];

        // Write the address
        AardvarkApi.aa_i2c_write(handle, device,
                                 AardvarkI2cFlags.AA_I2C_NO_STOP,
                                 (ushort)length, dataOut);

        count = AardvarkApi.aa_i2c_read(handle, device,
                                        AardvarkI2cFlags.AA_I2C_NO_FLAGS,
                                        (ushort)length, dataIn);
        if (count < 0) {
            Console.WriteLine("error: {0}\n",
                              AardvarkApi.aa_status_string(count));
            return;
        }
        if (count == 0) {
            Console.WriteLine("error: no bytes read");
            Console.WriteLine("  are you sure you have the right slave address?");
            return;
        }
        else if (count != length) {
            Console.WriteLine("error: read {0} bytes (expected {1})",
                              count, length);
        }

        // Dump the data to the screen
        Console.Write("\nData read from device:");
        for (i = 0; i < count; ++i) {
            if ((i&0x0f) == 0)      Console.Write("\n{0:x4}:  ", addr+i);
            Console.Write("{0:x2} ", dataIn[i] & 0xff);
            if (((i+1)&0x07) == 0)  Console.Write(" ");
        }
        Console.WriteLine();
    }


    /*=====================================================================
    | MAIN PROGRAM
     ====================================================================*/
    public static void Main (string[] args) {
        int    handle;
        int    port    = 0;
        int    bitrate = 100;
        int    bus_timeout;
        byte   device;
        byte   addr;
        short  length;

        if (args.Length != 6) {
            Console.WriteLine("usage: aai2c_eeprom PORT BITRATE read  SLAVE_ADDR OFFSET LENGTH");
            Console.WriteLine("usage: aai2c_eeprom PORT BITRATE write SLAVE_ADDR OFFSET LENGTH");
            Console.WriteLine("usage: aai2c_eeprom PORT BITRATE zero  SLAVE_ADDR OFFSET LENGTH");
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

        // Parse the slave address argument
        try {
            if (args[3].StartsWith("0x"))
                device = Convert.ToByte(args[3].Substring(2), 16);
            else
                device = Convert.ToByte(args[3]);
        }
        catch (Exception) {
            Console.WriteLine("Error: invalid device number");
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

        // Open the device
        handle = AardvarkApi.aa_open(port);
        if (handle <= 0) {
            Console.WriteLine("Unable to open Aardvark device on port {0}",
                              port);
            Console.WriteLine("error: {0}",
                              AardvarkApi.aa_status_string(handle));
            return;
        }

        // Ensure that the I2C subsystem is enabled
        AardvarkApi.aa_configure(handle,  AardvarkConfig.AA_CONFIG_SPI_I2C);

        // Enable the I2C bus pullup resistors (2.2k resistors).
        // This command is only effective on v2.0 hardware or greater.
        // The pullup resistors on the v1.02 hardware are enabled by default.
        AardvarkApi.aa_i2c_pullup(handle, AardvarkApi.AA_I2C_PULLUP_BOTH);

        // Power the EEPROM using the Aardvark adapter's power supply.
        // This command is only effective on v2.0 hardware or greater.
        // The power pins on the v1.02 hardware are not enabled by default.
        AardvarkApi.aa_target_power(handle,
                                     AardvarkApi.AA_TARGET_POWER_BOTH);

        // Set the bitrate
        bitrate = AardvarkApi.aa_i2c_bitrate(handle, bitrate);
        Console.WriteLine("Bitrate set to {0} kHz", bitrate);

        // Set the bus lock timeout
        bus_timeout = AardvarkApi.aa_i2c_bus_timeout(handle, BUS_TIMEOUT);
        Console.WriteLine("Bus lock timeout set to {0} ms", bus_timeout);

        // Perform the operation
        if (command == "write") {
            _writeMemory(handle, device, addr, length, false);
            Console.WriteLine("Wrote to EEPROM");
        }
        else if (command == "read") {
            _readMemory(handle, device, addr, length);
        }
        else if (command == "zero") {
            _writeMemory(handle, device, addr, length, true);
            Console.WriteLine("Zeroed EEPROM");
        }
        else {
            Console.WriteLine("unknown command: {0}", command);
        }

        // Close the device and exit
        AardvarkApi.aa_close(handle);

    }

}
