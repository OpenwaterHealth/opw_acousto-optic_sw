/*=========================================================================
| (c) 2006-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aai2c_file.cs
|--------------------------------------------------------------------------
| Configure the device as an I2C master and send data.
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
using System.IO;
using TotalPhase;


/*=========================================================================
| CLASS
 ========================================================================*/
public class AAI2cFile {

    /*=====================================================================
    | CONSTANTS
     ====================================================================*/
    public const int BUFFER_SIZE = 2048;
    public const int I2C_BITRATE = 400;


    /*=====================================================================
    | FUNCTIONS
     ====================================================================*/
    static void blastBytes (int handle, byte slave_addr, string filename)
    {
        FileStream file;
        int        trans_num = 0;
        byte[]     dataOut   = new byte[BUFFER_SIZE];

        // Open the file
        try {
            file = new FileStream(filename, FileMode.Open, FileAccess.Read);
        }
        catch (Exception) {
            Console.WriteLine("Unable to open file '{0}'", filename);
            return;
        }

        while (file.Length != file.Position) {
            int numWrite, count;
            int i;

            // Read from the file
            numWrite = file.Read(dataOut, 0, BUFFER_SIZE);
            if (numWrite == 0)  break;

            if (numWrite < BUFFER_SIZE) {
                byte[] temp = new byte[numWrite];
                for (i = 0; i < numWrite; i++)
                    temp[i] = dataOut[i];
                dataOut = temp;
            }

            // Write the data to the bus
            count = AardvarkApi.aa_i2c_write(handle, slave_addr,
                                             AardvarkI2cFlags.AA_I2C_NO_FLAGS,
                                             BUFFER_SIZE, dataOut);
            if (count < 0) {
                Console.WriteLine("error: {0}",
                                  AardvarkApi.aa_status_string(count));
                break;
            } else if (count == 0) {
                Console.WriteLine("error: no bytes written");
                Console.WriteLine("  are you sure you have the right slave " +
                                  "address?");
                break;
            } else if (count != numWrite) {
                Console.WriteLine("error: only a partial number of bytes " +
                                  "written");
                Console.WriteLine("  ({0}) instead of full ({1})",
                                  count, numWrite);
                break;
            }

            Console.WriteLine("*** Transaction #{0:d2}", trans_num);

            // Dump the data to the screen
            Console.WriteLine("Data written to device:");
            for (i = 0; i < count; ++i) {
                if ((i&0x0f) == 0)      Console.Write("\n{0:x4}:  ", i);
                Console.Write("{0:x2} ", dataOut[i] & 0xff);
                if (((i+1)&0x07) == 0)  Console.Write(" ");
            }
            Console.WriteLine();
            Console.WriteLine();

            ++trans_num;

            // Sleep a tad to make sure slave has time to process this request
            AardvarkApi.aa_sleep_ms(10);
        }

        file.Close();
    }


    /*=====================================================================
    | MAIN PROGRAM
     ====================================================================*/
    public static void Main (string[] args) {
        int  handle;
        int  port  = 0;
        byte addr  = 0;

        string filename;

        int bitrate;

        if (args.Length != 3) {
            Console.WriteLine("usage: aai2c_file PORT SLAVE_ADDR filename");
            Console.WriteLine("  SLAVE_ADDR is the target slave address");
            Console.WriteLine();
            Console.WriteLine("  'filename' should contain data to be sent");
            Console.WriteLine("  to the downstream i2c device");
            return;
        }

        // Parse the port argument
        try {
            port = Convert.ToInt32(args[0]);
        }
        catch (Exception) {
            Console.WriteLine("Error: invalid port");
            return;
        }

        // Parse the device address argument
        try {
            if (args[1].StartsWith("0x"))
                addr = Convert.ToByte(args[1].Substring(2), 16);
            else
                addr = Convert.ToByte(args[1]);
        }
        catch (Exception) {
            Console.WriteLine("Error: invalid device addr");
            return;
        }

        filename = args[2];

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
        AardvarkApi.aa_configure(handle, AardvarkConfig.AA_CONFIG_SPI_I2C);

        // Enable the I2C bus pullup resistors (2.2k resistors).
        // This command is only effective on v2.0 hardware or greater.
        // The pullup resistors on the v1.02 hardware are enabled by default.
        AardvarkApi.aa_i2c_pullup(handle, AardvarkApi.AA_I2C_PULLUP_BOTH);

        // Enable the Aardvark adapter's power pins.
        // This command is only effective on v2.0 hardware or greater.
        // The power pins on the v1.02 hardware are not enabled by default.
        AardvarkApi.aa_target_power(handle, AardvarkApi.AA_TARGET_POWER_BOTH);

        // Setup the bitrate
        bitrate = AardvarkApi.aa_i2c_bitrate(handle, I2C_BITRATE);
        Console.WriteLine("Bitrate set to {0} kHz", bitrate);

        blastBytes(handle, addr, filename);

        // Close the device
        AardvarkApi.aa_close(handle);

    }

}
