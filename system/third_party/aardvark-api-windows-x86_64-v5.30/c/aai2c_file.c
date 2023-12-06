/*=========================================================================
| (c) 2004-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aai2c_file.c
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

//=========================================================================
// INCLUDES
//=========================================================================
#include <stdio.h>
#include <stdlib.h>
#include "aardvark.h"


//=========================================================================
// CONSTANTS
//=========================================================================
#define BUFFER_SIZE  2048
#define I2C_BITRATE   400


//=========================================================================
// FUNCTIONS
//=========================================================================
static u08 data_out[BUFFER_SIZE];

static void blast_bytes (Aardvark handle, u08 slave_addr, char *filename)
{
    FILE *file;
    int trans_num = 0;

    // Open the file
    file = fopen(filename, "rb");
    if (!file) {
        printf("Unable to open file '%s'\n", filename);
        return;
    }
    
    while (!feof(file)) {
        int num_write, count;
        int i;

        // Read from the file
        num_write = fread((void *)data_out, 1, BUFFER_SIZE, file);
        if (!num_write)  break;

        // Write the data to the bus
        count = aa_i2c_write(handle, slave_addr, AA_I2C_NO_FLAGS,
                             (u16)num_write, data_out);
        if (count < 0) {
            printf("error: %s\n", aa_status_string(count));
            goto cleanup;
        } else if (count == 0) {
            printf("error: no bytes written\n");
            printf("  are you sure you have the right slave address?\n");
            goto cleanup;
        } else if (count != num_write) {
            printf("error: only a partial number of bytes written\n");
            printf("  (%d) instead of full (%d)\n", count, num_write);
            goto cleanup;
        }

        printf("*** Transaction #%02d\n", trans_num);
        
        // Dump the data to the screen
        printf("Data written to device:");
        for (i = 0; i < count; ++i) {
            if ((i&0x0f) == 0)      printf("\n%04x:  ", i);
            printf("%02x ", data_out[i] & 0xff);
            if (((i+1)&0x07) == 0)  printf(" ");
        }
        printf("\n\n");
        
        ++trans_num;
        
        // Sleep a tad to make sure slave has time to process this request
        aa_sleep_ms(10);
    }

cleanup:    
    fclose(file);
}



//=========================================================================
// MAIN PROGRAM
//=========================================================================
int main (int argc, char *argv[]) {
    Aardvark handle;
    int port  = 0;
    u08 addr  = 0;

    char *filename;
    
    int bitrate;

    if (argc < 4) {
        printf("usage: aai2c_file PORT SLAVE_ADDR filename\n");
        printf("  SLAVE_ADDR is the target slave address\n");
        printf("\n");
        printf("  'filename' should contain data to be sent\n");
        printf("  to the downstream i2c device\n");
        return 1;
    }

    port  = atoi(argv[1]);
    addr    = (u08)strtol(argv[2], 0, 0);

    filename = argv[3];

    // Open the device
    handle = aa_open(port);
    if (handle <= 0) {
        printf("Unable to open Aardvark device on port %d\n", port);
        printf("Error code = %d\n", handle);
        return 1;
    }

    // Ensure that the I2C subsystem is enabled
    aa_configure(handle, AA_CONFIG_SPI_I2C);

    // Enable the I2C bus pullup resistors (2.2k resistors).
    // This command is only effective on v2.0 hardware or greater.
    // The pullup resistors on the v1.02 hardware are enabled by default.
    aa_i2c_pullup(handle, AA_I2C_PULLUP_BOTH);

    // Enable the Aardvark adapter's power pins.
    // This command is only effective on v2.0 hardware or greater.
    // The power pins on the v1.02 hardware are not enabled by default.
    aa_target_power(handle, AA_TARGET_POWER_BOTH);

    // Setup the bitrate
    bitrate = aa_i2c_bitrate(handle, I2C_BITRATE);
    printf("Bitrate set to %d kHz\n", bitrate);

    blast_bytes(handle, addr, filename);

    // Close the device
    aa_close(handle);

    return 0;
}
