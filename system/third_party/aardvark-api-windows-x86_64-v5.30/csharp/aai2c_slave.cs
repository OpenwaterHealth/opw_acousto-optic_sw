/*=========================================================================
| (c) 2006-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aai2c_slave.cs
|--------------------------------------------------------------------------
| Configure the device as an I2C slave and watch incoming data.
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
public class AAI2cSlave {

    /*=====================================================================
    | CONSTANTS
     ====================================================================*/
    public const int BUFFER_SIZE      = 65535;
    public const int SLAVE_RESP_SIZE  = 26;
    public const int INTERVAL_TIMEOUT = 500;


    /*=====================================================================
    | FUNCTIONS
     ====================================================================*/
    static void dump (int handle, int timeout_ms)
    {
        int    result;
        byte   addr     = 0;
        int    transNum = 0;
        byte[] dataIn   = new byte[BUFFER_SIZE];

        Console.WriteLine("Watching slave I2C data...");

        // Wait for data on bus
        result = AardvarkApi.aa_async_poll(handle, timeout_ms);
        if (result == AardvarkApi.AA_ASYNC_NO_DATA) {
            Console.WriteLine("No data available.");
            return;
        }
        Console.WriteLine();

        // Loop until aa_async_poll times out
        for (;;) {
            // Read the I2C message.
            // This function has an internal timeout (see datasheet), though
            // since we have already checked for data using aa_async_poll,
            // the timeout should never be exercised.
            if (result == AardvarkApi.AA_ASYNC_I2C_READ) {
                // Get data written by master
                int numBytes = AardvarkApi.aa_i2c_slave_read(
                    handle, ref addr, unchecked((ushort)BUFFER_SIZE),
                    dataIn);
                int i;

                if (numBytes < 0) {
                    Console.WriteLine("error: {0}",
                                      AardvarkApi.aa_status_string(numBytes));
                    return;
                }

                // Dump the data to the screen
                Console.WriteLine("*** Transaction #{0:d2}", transNum);
                Console.Write("Data read from master:");
                for (i = 0; i < numBytes; ++i) {
                    if ((i&0x0f) == 0)      Console.Write("\n{0:x4}:  ", i);
                    Console.Write("{0:x2} ", dataIn[i] & 0xff);
                    if (((i+1)&0x07) == 0)  Console.Write(" ");
                }
                Console.WriteLine();
                Console.WriteLine();
            }

            else if (result == AardvarkApi.AA_ASYNC_I2C_WRITE) {
                // Get number of bytes written to master
                int numBytes = AardvarkApi.aa_i2c_slave_write_stats(handle);

                if (numBytes < 0) {
                    Console.WriteLine("error: {0}",
                                      AardvarkApi.aa_status_string(numBytes));
                    return;
                }

                // Print status information to the screen
                Console.WriteLine("*** Transaction #{0:d2}", transNum);
                Console.WriteLine("Number of bytes written to master: {0:d4}",
                                  numBytes);
                Console.WriteLine();
            }

            else {
                Console.WriteLine("error: non-I2C asynchronous message is " +
                                  "pending");
                return;
            }

            ++transNum;

            // Use aa_async_poll to wait for the next transaction
            result = AardvarkApi.aa_async_poll(handle, INTERVAL_TIMEOUT);
            if (result == AardvarkApi.AA_ASYNC_NO_DATA) {
                Console.WriteLine("No more data available from I2C master.");
                break;
            }
        }
    }


    /*=====================================================================
    | MAIN PROGRAM
     ====================================================================*/
    public static void Main (string[] args) {
        int handle;

        int  port      = 0;
        byte addr      = 0;
        int  timeoutMs = 0;

        byte[] slaveResp = new byte[SLAVE_RESP_SIZE];
        int i;

        if (args.Length != 3) {
            Console.WriteLine("usage: aai2c_slave PORT SLAVE_ADDR TIMEOUT_MS");
            Console.WriteLine("  SLAVE_ADDR is the slave address for this device");
            Console.WriteLine();
            Console.WriteLine("  The timeout value specifies the time to");
            Console.WriteLine("  block until the first packet is received.");
            Console.WriteLine("  If the timeout is -1, the program will");
            Console.WriteLine("  block indefinitely.");
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

        // Parse the timeout argument
        try {
            timeoutMs = Convert.ToInt32(args[2]);
        }
        catch (Exception) {
            Console.WriteLine("Error: invalid timeout value");
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
        AardvarkApi.aa_configure(handle, AardvarkConfig.AA_CONFIG_SPI_I2C);

        // Disable the Aardvark adapter's power pins.
        // This command is only effective on v2.0 hardware or greater.
        // The power pins on the v1.02 hardware are not enabled by default.
        AardvarkApi.aa_target_power(handle, AardvarkApi.AA_TARGET_POWER_NONE);

        // Set the slave response; this won't be used unless the master
        // reads bytes from the slave.
        for (i = 0; i < SLAVE_RESP_SIZE; ++i)
            slaveResp[i] = (byte)('A' + i);

        AardvarkApi.aa_i2c_slave_set_response(handle, SLAVE_RESP_SIZE,
                                              slaveResp);

        // Enable the slave
        AardvarkApi.aa_i2c_slave_enable(handle, addr, 0, 0);

        // Watch the I2C port
        dump(handle, timeoutMs);

        // Disable the slave and close the device
        AardvarkApi.aa_i2c_slave_disable(handle);
        AardvarkApi.aa_close(handle);

    }

}
