/*=========================================================================
| (c) 2006-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aagpio.cs
|--------------------------------------------------------------------------
| Perform some simple GPIO operations with a single Aardvark adapter.
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
public class AAGpio {

    /*=====================================================================
    | MAIN PROGRAM
     ====================================================================*/
    public static void Main (string[] args) {
        int                      port = -1;
        byte                     val;
        int                      handle;
        AardvarkApi.AardvarkExt aaExt = new AardvarkApi.AardvarkExt();

        if (args.Length != 1) {
            Console.WriteLine("usage: aagpio PORT");
            return;
        }

        try {
            port = Convert.ToInt32(args[0]);
        }
        catch (Exception) {
            Console.WriteLine("Error: invalid port number");
        }

        // Open the device
        handle = AardvarkApi.aa_open_ext(port, ref aaExt);

        if (handle <= 0) {
            Console.WriteLine("Unable to open Aardvark device on port {0}",
                              port);
            Console.WriteLine("error: {0}",
                              AardvarkApi.aa_status_string(handle));
            return;
        }

        Console.WriteLine("Opened Aardvark adapter");

        // Configure the Aardvark adapter so all pins
        // are now controlled by the GPIO subsystem
        AardvarkApi.aa_configure(handle, AardvarkConfig.AA_CONFIG_GPIO_ONLY);

        // Turn off the external I2C line pullups
        AardvarkApi.aa_i2c_pullup(handle, AardvarkApi.AA_I2C_PULLUP_NONE);
    
        // Make sure the charge has dissipated on those lines
        AardvarkApi.aa_gpio_set(handle, 0x00);
        AardvarkApi.aa_gpio_direction(handle, 0xff);

        // By default all GPIO pins are inputs.  Writing 1 to the
        // bit position corresponding to the appropriate line will
        // configure that line as an output
        AardvarkApi.aa_gpio_direction(handle,
            (byte)(AardvarkGpioBits.AA_GPIO_SS | AardvarkGpioBits.AA_GPIO_SCL));

        // By default all GPIO outputs are logic low.  Writing a 1
        // to the appropriate bit position will force that line
        // high provided it is configured as an output.  If it is
        // not configured as an output the line state will be
        // cached such that if the direction later changed, the
        // latest output value for the line will be enforced.
        AardvarkApi.aa_gpio_set(handle, (byte)AardvarkGpioBits.AA_GPIO_SCL);
        Console.WriteLine("Setting SCL to logic low");

        // The get method will return the line states of all inputs.
        // If a line is not configured as an input the value of
        // that particular bit position in the mask will be 0.
        val = (byte)AardvarkApi.aa_gpio_get(handle);

        // Check the state of SCK
        if ((val & (byte)AardvarkGpioBits.AA_GPIO_SCK) != 0)
            Console.WriteLine("Read the SCK line as logic high");
        else
            Console.WriteLine("Read the SCK line as logic low");
 
        // Optionally we can set passive pullups on certain lines.
        // This can prevent input lines from floating.  The pullup
        // configuration is only valid for lines configured as inputs.
        // If the line is not currently input the requested pullup
        // state will take effect only if the line is later changed
        // to be an input line.
        AardvarkApi.aa_gpio_pullup(handle,
            (byte)(AardvarkGpioBits.AA_GPIO_MISO | AardvarkGpioBits.AA_GPIO_MOSI));

        // Now reading the MISO line should give a logic high,
        // provided there is nothing attached to the Aardvark
        // adapter that is driving the pin low.
        val = (byte)AardvarkApi.aa_gpio_get(handle);
        if ((val & (byte)AardvarkGpioBits.AA_GPIO_MISO) != 0)
            Console.WriteLine(
                "Read the MISO line as logic high (passive pullup)");
        else
            Console.WriteLine(
                "Read the MISO line as logic low (is pin driven low?)");


        // Now do a 1000 gets from the GPIO to test performance
        for (int i = 0; i < 1000; ++i)
            AardvarkApi.aa_gpio_get(handle);

        int oldval, newval;

        // Demonstrate use of aa_gpio_change
        AardvarkApi.aa_gpio_direction(handle, 0x00);
        oldval = AardvarkApi.aa_gpio_get(handle);

        Console.WriteLine("Calling aa_gpio_change for 2 seconds...");
        newval = AardvarkApi.aa_gpio_change(handle, 2000);

        if (newval != oldval)
            Console.WriteLine("  GPIO inputs changed.\n");
        else
            Console.WriteLine("  GPIO inputs did not change.\n");

        // Turn on the I2C line pullups since we are done
        AardvarkApi.aa_i2c_pullup(handle, AardvarkApi.AA_I2C_PULLUP_BOTH);

        // Configure the Aardvark adapter back to SPI/I2C mode.
        AardvarkApi.aa_configure(handle, AardvarkConfig.AA_CONFIG_SPI_I2C);

        // Close the device
        AardvarkApi.aa_close(handle);

    }

}
