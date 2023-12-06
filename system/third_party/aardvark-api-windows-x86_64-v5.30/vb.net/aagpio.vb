'==========================================================================
' (c) 2004-2009  Total Phase, Inc.
'--------------------------------------------------------------------------
' Project : Aardvark Sample Code
' File    : aagpio.vb
'--------------------------------------------------------------------------
' Perform some simple GPIO operations with a single Aardvark adapter.
'--------------------------------------------------------------------------
' Redistribution and use of this file in source and binary forms, with
' or without modification, are permitted.
'
' THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
' "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
' LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
' FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
' COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
' INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
' BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
' LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
' CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
' LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
' ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
' POSSIBILITY OF SUCH DAMAGE.
'==========================================================================
Imports TotalPhase

Module aagpio

    '==========================================================================
    ' MAIN PROGRAM
    '==========================================================================
    Sub aagpio_run()
        Dim handle As Long
        Dim aaext As AardvarkApi.AardvarkExt
        Dim i As Integer

        ' Open the device
        handle = AardvarkApi.aa_open_ext(0, aaext)
        If handle <= 0 Then
            Console.WriteLine("Unable to open Aardvark device on port 0")
            Console.WriteLine("Error code = " & handle)
            Exit Sub
        End If

        Console.WriteLine("Opened Aardvark adapter; features = " & _
            aaext.features)

        ' Configure the Aardvark adapter so all pins
        ' are now controlled by the GPIO subsystem
        Call AardvarkApi.aa_configure(handle, _
            AardvarkConfig.AA_CONFIG_GPIO_ONLY)

        ' Configure the Aardvark adapter so all pins
        ' are now controlled by the GPIO subsystem
        Call AardvarkApi.aa_configure(handle, _
            AardvarkConfig.AA_CONFIG_GPIO_ONLY)

        ' Turn off the external I2C line pullups
        Call AardvarkApi.aa_i2c_pullup(handle, AardvarkApi.AA_I2C_PULLUP_NONE)

        ' Make sure the charge has dissipated on those lines
        Call AardvarkApi.aa_gpio_set(handle, &H0)
        Call AardvarkApi.aa_gpio_direction(handle, &HFF)

        ' By default all GPIO pins are inputs.  Writing 1 to the
        ' bit position corresponding to the appropriate line will
        ' configure that line as an output
        Call AardvarkApi.aa_gpio_direction(handle, _
            AardvarkGpioBits.AA_GPIO_SS Or AardvarkGpioBits.AA_GPIO_SCL)

        ' By default all GPIO outputs are logic low.  Writing a 1
        ' to the appropriate bit position will force that line
        ' high provided it is configured as an output.  If it is
        ' not configured as an output the line state will be
        ' cached such that if the direction later changed, the
        ' latest output value for the line will be enforced.
        Call AardvarkApi.aa_gpio_set(handle, AardvarkGpioBits.AA_GPIO_SCL)
        Console.WriteLine("Setting SCL to logic low")

        ' The get method will return the line states of all inputs.
        ' If a line is not configured as an input the value of
        ' that particular bit position in the mask will be 0.
        Dim val As Long
        val = AardvarkApi.aa_gpio_get(handle)

        ' Check the state of SCK
        If (val And AardvarkGpioBits.AA_GPIO_SCK) Then
            Console.WriteLine("Read the SCK line as logic high")
        Else
            Console.WriteLine("Read the SCK line as logic low")
        End If

        ' Optionally we can set passive pullups on certain lines.
        ' This can prevent input lines from floating.  The pullup
        ' configuration is only valid for lines configured as inputs.
        ' If the line is not currently input the requested pullup
        ' state will take effect only if the line is later changed
        ' to be an input line.
        Call AardvarkApi.aa_gpio_pullup(handle, _
            AardvarkGpioBits.AA_GPIO_MISO Or AardvarkGpioBits.AA_GPIO_MOSI)

        ' Now reading the MISO line should give a logic high,
        ' provided there is nothing attached to the Aardvark
        ' adapter that is driving the pin low.
        val = AardvarkApi.aa_gpio_get(handle)
        If (val And AardvarkGpioBits.AA_GPIO_MISO) Then
            Console.WriteLine("Read the MISO line as logic high " & _
                              "(passive pullup)")
        Else
            Console.WriteLine("Read the MISO line as logic low " & _
                              "(is pin driven low?)")
        End If

        ' Now do a 1000 gets from the GPIO to test performance
        Console.WriteLine("Doing 1000 iterations of GPIO read...")
        For i = 1 To 1000
            AardvarkApi.aa_gpio_get(handle)
        Next

        ' Demonstrate use of aa_gpio_change
        Call AardvarkApi.aa_gpio_direction(handle, &H0)
        Dim oldval As Long
        Dim newval As Long
        oldval = AardvarkApi.aa_gpio_get(handle)
        Console.WriteLine("Calling aa_gpio_change for 2 seconds...")
        newval = AardvarkApi.aa_gpio_change(handle, 2000)
        If (newval <> oldval) Then
            Console.WriteLine("  GPIO inputs changed.")
        Else
            Console.WriteLine("  GPIO inputs did not change.")
        End If

        ' Turn on the I2C line pullups since we are done
        Call AardvarkApi.aa_i2c_pullup(handle, AardvarkApi.AA_I2C_PULLUP_BOTH)

        ' Configure the Aardvark adapter back to SPI/I2C mode.
        Call AardvarkApi.aa_configure(handle, AardvarkConfig.AA_CONFIG_SPI_I2C)

        ' Close the device
        AardvarkApi.aa_close(handle)
        Console.WriteLine("Finished")
    End Sub
End Module
