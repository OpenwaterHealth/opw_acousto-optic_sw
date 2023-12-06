/*=========================================================================
| (c) 2006-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aamonitor.cs
|--------------------------------------------------------------------------
| Perform I2C monitoring functions with the Aardvark I2C/SPI adapter.
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
public class AAMonitor {

    /*=====================================================================
    | CONSTANTS
     ====================================================================*/
    public const int BUFFER_SIZE      = 32767;
    public const int INITIAL_TIMEOUT  = 5000;
    public const int INTERVAL_TIMEOUT = 500;


    /*=====================================================================
    | FUNCTIONS
     ====================================================================*/
    static void dump (int handle)
    {
        int      result;
        ushort   last_data1 = 0;
        ushort   last_data0 = 0;
        ushort[] data       = new ushort[BUFFER_SIZE];

        // Wait for data on the bus
        Console.WriteLine("Waiting {0} ms for first transaction...",
                          INITIAL_TIMEOUT);
        result = AardvarkApi.aa_async_poll(handle, INITIAL_TIMEOUT);
        if (result == AardvarkApi.AA_ASYNC_NO_DATA) {
            Console.WriteLine("  no data pending.");
            return;
        }

        Console.WriteLine("  data received.");

        // Loop until aa_async_poll times out
        for (;;) {
            // Read the next monitor transaction.
            // This function has an internal timeout (see datasheet), though
            // since we have already checked for data using aa_async_poll,
            // the timeout should never be exercised.
            int num_bytes = AardvarkApi.aa_i2c_monitor_read(
                handle, (ushort)BUFFER_SIZE, data);
            int i;

            if (num_bytes < 0) {
                Console.WriteLine("error: {0}",
                                  AardvarkApi.aa_status_string(num_bytes));
                return;
            }

            for (i = 0; i < num_bytes; ++i) {
                // Check for start condition
                if (data[i] == AardvarkApi.AA_I2C_MONITOR_CMD_START) {
                    DateTime now = DateTime.Now;
                    Console.Write("\n{0:ddd MMM dd HH:mm:ss yyyy} - [S] ",
                                  now);
                }

                // Check for stop condition
                else if (data[i] == AardvarkApi.AA_I2C_MONITOR_CMD_STOP) {
                    Console.WriteLine("[P]");
                }

                else {
                    int nack = (data[i] & AardvarkApi.AA_I2C_MONITOR_NACK);
                    // 7-bit addresses
                    if (last_data0 ==
                        (int)AardvarkApi.AA_I2C_MONITOR_CMD_START &&
                        ((data[i] & 0xf8) != 0xf0 || nack != 0)) {
                        Console.Write("<{0:x2}:{1}>{2} ",
                                      (data[i] & 0xff) >> 1,
                                      (data[i] & 0x01) != 0 ? 'r' : 'w',
                                      nack != 0 ? "*" : "");
                    }

                    // 10-bit addresses
                    // See Philips specification for more details.
                    else if (last_data1 ==
                             (int)AardvarkApi.AA_I2C_MONITOR_CMD_START &&
                             (last_data0 & 0xf8) == 0xf0) {
                        Console.Write(
                            "<{0:x3}:{1}>{2} ",
                            ((last_data0 << 7) & 0x300) | (data[i] & 0xff),
                            (last_data0 & 0x01) != 0 ? 'r' : 'w',
                            nack != 0 ? "*" : "");
                    }

                    // Normal data
                    else if (last_data0 !=
                             (int)AardvarkApi.AA_I2C_MONITOR_CMD_START) {
                        Console.Write("{0:x2}{1} ", data[i] & 0xff,
                                      nack != 0 ? "*" : "");
                    }
                }

                last_data1 = last_data0;
                last_data0 = data[i];
            }

            Console.WriteLine("\nWaiting {0} ms for subsequent transaction...",
                              INTERVAL_TIMEOUT);
            result = AardvarkApi.aa_async_poll(handle, INTERVAL_TIMEOUT);
            if (result == AardvarkApi.AA_ASYNC_NO_DATA) {
                Console.WriteLine("  no more data pending.");
                break;
            }
            Console.WriteLine(" data received.");
            Console.WriteLine();
        }
    }


    /*=====================================================================
    | MAIN PROGRAM
     ====================================================================*/
    public static void Main (string[] args) {
        int                     port;
        int                     handle;
        int                     result;
        AardvarkApi.AardvarkExt aaExt = new AardvarkApi.AardvarkExt();

        if (args.Length != 1) {
            Console.WriteLine("usage: aamonitor PORT");
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

        // Open the device
        handle  = AardvarkApi.aa_open_ext(port, ref aaExt);

        if (handle <= 0) {
            Console.WriteLine("Unable to open Aardvark device on port {0}",
                              port);
            Console.WriteLine("error: {0}",
                              AardvarkApi.aa_status_string(handle));
            return;
        }

        Console.WriteLine("Opened Aardvark; features = 0x{0:x2}",
                          aaExt.features);

        // Disable the I2C bus pullup resistors (2.2k resistors).
        // This command is only effective on v2.0 hardware or greater.
        // The pullup resistors on the v1.02 hardware are enabled by default.
        AardvarkApi.aa_i2c_pullup(handle, AardvarkApi.AA_I2C_PULLUP_NONE);

        // Disable the Aardvark adapter's power pins.
        // This command is only effective on v2.0 hardware or greater.
        // The power pins on the v1.02 hardware are not enabled by default.
        AardvarkApi.aa_target_power(handle, AardvarkApi.AA_TARGET_POWER_NONE);

        // Enable the monitor
        result = AardvarkApi.aa_i2c_monitor_enable(handle);
        if (result < 0) {
            Console.WriteLine("error: {0}",
                              AardvarkApi.aa_status_string(result));
            return;
        }
        Console.WriteLine("Enabled I2C monitor.");

        // Dump the data to the console
        dump(handle);

        // Disable the monitor and close the device
        AardvarkApi.aa_i2c_monitor_disable(handle);
        AardvarkApi.aa_close(handle);

    }

}
