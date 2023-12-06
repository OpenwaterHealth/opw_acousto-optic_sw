/*=========================================================================
| (C) 2006-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aalights.cs
|--------------------------------------------------------------------------
| Flash the lights on the Aardvark I2C/SPI Activity Board.
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
public class AALights {

    /*=====================================================================
    | CONSTANTS
     ====================================================================*/
    private const int I2C_BITRATE = 100; // kHz


    /*=====================================================================
    | STATIC FUNCTIONS
     ====================================================================*/
    public static int FlashLights (int handle)
    {
        int res, i;
        byte[] data_out = new byte[2];

        // Configure I/O expander lines as outputs
        data_out[0] = 0x03;
        data_out[1] = 0x00;
        res = AardvarkApi.aa_i2c_write(handle, 0x38,
                                       AardvarkI2cFlags.AA_I2C_NO_FLAGS,
                                       2, data_out);
        if (res < 0)  return res;

        if (res == 0) {
            Console.WriteLine("error: slave device 0x38 not found");
            return 0;
        }

        // Turn lights on in sequence
        i = 0xff;
        do {
            i = ((i<<1) & 0xff);
            data_out[0] = 0x01;
            data_out[1] = (byte)i;
            res = AardvarkApi.aa_i2c_write(handle, 0x38,
                                           AardvarkI2cFlags.AA_I2C_NO_FLAGS,
                                           2, data_out);
            if (res < 0)  return res;
            AardvarkApi.aa_sleep_ms(70);
        } while (i != 0);

        // Leave lights on for 100 ms
        AardvarkApi.aa_sleep_ms(100);

        // Turn lights off in sequence
        i = 0x00;
        do {
            i = ((i<<1) | 0x01);
            data_out[0] = 0x01;
            data_out[1] = (byte)i;
            res = AardvarkApi.aa_i2c_write(handle, 0x38,
                                           AardvarkI2cFlags.AA_I2C_NO_FLAGS,
                                           2, data_out);
            if (res < 0)  return res;
            AardvarkApi.aa_sleep_ms(70);
        } while (i != 0xff);

        AardvarkApi.aa_sleep_ms(100);

        // Configure I/O expander lines as inputs
        data_out[0] = 0x03;
        data_out[1] = 0xff;
        res = AardvarkApi.aa_i2c_write(handle, 0x38,
                                       AardvarkI2cFlags.AA_I2C_NO_FLAGS,
                                       2, data_out);
        if (res < 0)  return res;

        return 0;
    }


    /*=====================================================================
    | MAIN PROGRAM
     ====================================================================*/
    public static void Main (string[] args) {
        int   handle;
        int   port    = 0;
        int   bitrate = 100;
        int   res     = 0;

        if (args.Length != 1) {
            Console.WriteLine("usage: aalights PORT");
            return;
        }

        try {
            port = Convert.ToInt32(args[0]);
        }
        catch (Exception) {
            Console.WriteLine("Error: invalid port number");
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

        // Enable logging
        FileStream logfile   = null;
        bool       isLogging = true;
        try {
            logfile = new FileStream("log.txt", FileMode.Append,
                                     FileAccess.Write);
            AardvarkApi.aa_log(handle, 3, (int)logfile.Handle);
        }
        catch (Exception) {
            isLogging = false;
        }

        // Ensure that the I2C subsystem is enabled
        AardvarkApi.aa_configure(handle, AardvarkConfig.AA_CONFIG_SPI_I2C);

        // Enable the I2C bus pullup resistors (2.2k resistors).
        // This command is only effective on v2.0 hardware or greater.
        // The pullup resistors on the v1.02 hardware are enabled by default.
        AardvarkApi.aa_i2c_pullup(handle, AardvarkApi.AA_I2C_PULLUP_BOTH);

        // Power the board using the Aardvark adapter's power supply.
        // This command is only effective on v2.0 hardware or greater.
        // The power pins on the v1.02 hardware are not enabled by default.
        AardvarkApi.aa_target_power(handle,
                                     AardvarkApi.AA_TARGET_POWER_BOTH);

        // Set the bitrate
        bitrate = AardvarkApi.aa_i2c_bitrate(handle, I2C_BITRATE);
        Console.WriteLine("Bitrate set to {0} kHz", bitrate);

        res = AALights.FlashLights(handle);
        if (res < 0)
            Console.WriteLine("error: {0}", AardvarkApi.aa_status_string(res));

        // Close the device and exit
        AardvarkApi.aa_close(handle);

        // Close the logging file
        if (isLogging)
            logfile.Close();

    }
}
