'==========================================================================
' (c) 2004-2009  Total Phase, Inc.
'--------------------------------------------------------------------------
' Project : Aardvark Sample Code
' File    : aai2c_eeprom.bas
'--------------------------------------------------------------------------
' Perform simple read and write operations to an I2C EEPROM device.
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

Module aai2c_eeprom

    '==========================================================================
    ' CONSTANTS
    '==========================================================================
    Const I2C_BITRATE As Integer = 100  'kHz
    Const I2C_BUS_TIMEOUT As UShort = 150  'ms
    Const I2C_DEVICE As UShort = &H50


    '==========================================================================
    ' MAIN PROGRAM
    '==========================================================================
    Sub aai2c_eeprom_run()
        Dim handle As Long

        handle = AardvarkApi.aa_open(0)
        If (handle <= 0) Then
            Console.WriteLine("Unable to open Aardvark device on port 0")
            Console.WriteLine("Error code = " & handle)
            Exit Sub
        End If

        ' Ensure that the I2C subsystem is enabled
        Call AardvarkApi.aa_configure(handle, AardvarkConfig.AA_CONFIG_SPI_I2C)

        ' Enable the I2C bus pullup resistors (2.2k resistors).
        ' This command is only effective on v2.0 hardware or greater.
        ' The pullup resistors on the v1.02 hardware are enabled by default.
        Call AardvarkApi.aa_i2c_pullup(handle, AardvarkApi.AA_I2C_PULLUP_BOTH)

        ' Power the board using the Aardvark adapter's power supply.
        ' This command is only effective on v2.0 hardware or greater.
        ' The power pins on the v1.02 hardware are not enabled by default.
        Call AardvarkApi.aa_target_power(handle, AardvarkApi.AA_TARGET_POWER_BOTH)

        ' Set the bitrate
        Dim bitrate As Long
        bitrate = AardvarkApi.aa_i2c_bitrate(handle, I2C_BITRATE)
        Console.WriteLine("Bitrate set to " & bitrate & " kHz")

        ' Set the bus lock timeout
        Dim bus_timeout As Long
        bus_timeout = AardvarkApi.aa_i2c_bus_timeout(handle, I2C_BUS_TIMEOUT)
        Console.WriteLine("Bus lock timeout set to " & bus_timeout & " ms")

        ' Write the offset and read the data
        Dim offset(0) As Byte
        Dim data(15) As Byte
        Dim result As Long

        offset(0) = 0
        Call AardvarkApi.aa_i2c_write(handle, I2C_DEVICE, AardvarkI2cFlags.AA_I2C_NO_STOP, 1, offset)

        result = AardvarkApi.aa_i2c_read(handle, I2C_DEVICE, AardvarkI2cFlags.AA_I2C_NO_FLAGS, 16, data)
        If result <= 0 Then
            Console.WriteLine("i2c read error")
        Else
            Dim i As Integer
            Console.WriteLine("Read data bytes:")
            For i = 0 To 15
                Console.WriteLine(data(i))
            Next
        End If

        ' Close the device and exit
        AardvarkApi.aa_close(handle)
    End Sub
End Module
