/*=========================================================================
| (c) 2006-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aaspi_slave.cs
|--------------------------------------------------------------------------
| Configure the device as an SPI slave and watch incoming data.
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
public class AASpiSlave {

    /*=====================================================================
    | CONSTANTS
     ====================================================================*/
    public const int BUFFER_SIZE      = 65535;
    public const int SLAVE_RESP_SIZE  = 26;


    /*=====================================================================
    | FUNCTIONS
     ====================================================================*/
    static void dump (int handle, int timeout_ms)
    {
        int    transNum = 0;
        int    result;
        byte[] dataIn = new byte[BUFFER_SIZE];

        Console.WriteLine("Watching slave SPI data...");

        // Wait for data on bus
        result = AardvarkApi.aa_async_poll(handle, timeout_ms);
        if (result != AardvarkApi.AA_ASYNC_SPI) {
            Console.WriteLine("No data available.");
            return;
        }
        Console.WriteLine();

        // Loop until aa_spi_slave_read times out
        for (;;) {
            int numRead;

            // Read the SPI message.
            // This function has an internal timeout (see datasheet).
            // To use a variable timeout the function aa_async_poll could
            // be used for subsequent messages.
            numRead = AardvarkApi.aa_spi_slave_read(
                handle, unchecked((ushort)BUFFER_SIZE), dataIn);

            if (numRead < 0 &&
                numRead != (int)AardvarkStatus.AA_SPI_SLAVE_TIMEOUT) {
                Console.WriteLine("error: {0}",
                                  AardvarkApi.aa_status_string(numRead));
                return;
            }
            else if (numRead == 0 ||
                     numRead == (int)AardvarkStatus.AA_SPI_SLAVE_TIMEOUT) {
                Console.WriteLine("No more data available from SPI master.");
                return;
            }
            else {
                int i;
                // Dump the data to the screen
                Console.WriteLine("*** Transaction #{0:d2}", transNum);
                Console.Write("Data read from device:");
                for (i = 0; i < numRead; ++i) {
                    if ((i&0x0f) == 0)      Console.Write("\n{0:x4}:  ", i);
                    Console.Write("{0:x2} ", dataIn[i] & 0xff);
                    if (((i+1)&0x07) == 0)  Console.Write(" ");
                }
                Console.WriteLine();
                Console.WriteLine();

                ++transNum;
            }
            result = AardvarkApi.aa_async_poll(handle, timeout_ms);
            if (result != AardvarkApi.AA_ASYNC_SPI) {
                Console.WriteLine("No data available.");
                return;
            }
        }
    }


    /*=====================================================================
    | MAIN PROGRAM
     ====================================================================*/
    static public void Main (string[] args) {
        int handle;

        int port       = 0;
        int mode       = 0;
        int timeoutMs = 0;

        byte[] slaveResp = new byte[SLAVE_RESP_SIZE];
        int i;

        if (args.Length != 3) {
            Console.WriteLine("usage: aaspi_slave PORT MODE TIMEOUT_MS");
            Console.WriteLine("  mode 0 : pol = 0, phase = 0");
            Console.WriteLine("  mode 1 : pol = 0, phase = 1");
            Console.WriteLine("  mode 2 : pol = 1, phase = 0");
            Console.WriteLine("  mode 3 : pol = 1, phase = 1");
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

        // Parse the mode argument
        try {
            mode = Convert.ToInt32(args[1]) & 0x3;
        }
        catch (Exception) {
            Console.WriteLine("Error: invalid mode");
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

        // Ensure that the SPI subsystem is enabled
        AardvarkApi.aa_configure(handle, AardvarkConfig.AA_CONFIG_SPI_I2C);

        // Disable the Aardvark adapter's power pins.
        // This command is only effective on v2.0 hardware or greater.
        // The power pins on the v1.02 hardware are not enabled by default.
        AardvarkApi.aa_target_power(handle, AardvarkApi.AA_TARGET_POWER_NONE);

        // Setup the clock phase
        AardvarkApi.aa_spi_configure(handle, (AardvarkSpiPolarity)(mode >> 1),
                                     (AardvarkSpiPhase)(mode & 1),
                                     AardvarkSpiBitorder.AA_SPI_BITORDER_MSB);

        // Set the slave response
        for (i = 0; i < SLAVE_RESP_SIZE; ++i)
            slaveResp[i] = (byte)('A' + i);

        AardvarkApi.aa_spi_slave_set_response(handle, SLAVE_RESP_SIZE,
                                              slaveResp);

        // Enable the slave
        AardvarkApi.aa_spi_slave_enable(handle);

        // Watch the SPI port
        dump(handle, timeoutMs);

        // Disable the slave and close the device
        AardvarkApi.aa_spi_slave_disable(handle);
        AardvarkApi.aa_close(handle);

    }

}
