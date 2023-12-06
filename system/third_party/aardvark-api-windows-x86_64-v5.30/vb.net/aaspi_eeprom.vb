'==========================================================================
' (c) 2004-2009  Total Phase, Inc.
'--------------------------------------------------------------------------
' Project : Aardvark Sample Code
' File    : aaspi_eeprom.vb
'--------------------------------------------------------------------------
' Perform simple read and write operations to an SPI EEPROM device.
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

Module aaspi_eeprom

    '==========================================================================
    ' CONSTANTS
    '==========================================================================
    Const SPI_BITRATE As Integer = 1000  'kHz


    '==========================================================================
    ' MAIN PROGRAM
    '==========================================================================
    Sub aaspi_eeprom_run()
        Dim handle As Long

        ' Open the device
        handle = AardvarkApi.aa_open(0)
        If (handle <= 0) Then
            Console.WriteLine("Unable to open Aardvark device on port 0")
            Console.WriteLine("Error code = " & handle)
            Exit Sub
        End If

        ' Ensure that the I2C subsystem is enabled
        Call AardvarkApi.aa_configure(handle, AardvarkConfig.AA_CONFIG_SPI_I2C)

        ' Power the board using the Aardvark adapter's power supply.
        ' This command is only effective on v2.0 hardware or greater.
        ' The power pins on the v1.02 hardware are not enabled by default.
        Call AardvarkApi.aa_target_power(handle, AardvarkApi.AA_TARGET_POWER_BOTH)

        ' Set the bitrate
        Dim bitrate As Long
        bitrate = AardvarkApi.aa_spi_bitrate(handle, SPI_BITRATE)
        Console.WriteLine("Bitrate set to " & bitrate & " kHz")

        ' Write the offset and read the data
        Dim data_out(3 + 15) As Byte
        Dim data_in(18) As Byte
        Dim result As Long

        ' Set read command and address
        data_out(0) = &H3
        data_out(1) = 0
        data_out(2) = 0

        ' Write the transaction
        result = AardvarkApi.aa_spi_write(handle, 19, data_out, 19, data_in)
        If result < 0 Then
            Console.WriteLine("spi write error")
        Else
            Dim i As Integer
            Console.WriteLine("Read data bytes:")
            For i = 0 To 15
                ' First 3 bytes are command and address, so add 3
                Console.WriteLine(data_in(i + 3))
            Next
        End If

        ' Close the device and exit
        AardvarkApi.aa_close(handle)
    End Sub
End Module
