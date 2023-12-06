/*=========================================================================
| (c) 2004-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aamonitor.c
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

//=========================================================================
// INCLUDES
//=========================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "aardvark.h"


//=========================================================================
// CONSTANTS
//=========================================================================
#define BUFFER_SIZE      32767

#define INITIAL_TIMEOUT   5000
#define INTERVAL_TIMEOUT   500


//=========================================================================
// FUNCTIONS
//=========================================================================
static u16   data[BUFFER_SIZE];

static void dump (Aardvark handle)
{
    int result;
    u16 last_data1 = 0;
    u16 last_data0 = 0;
    
    // Wait for data on the bus
    printf("Waiting %d ms for first transaction...\n", INITIAL_TIMEOUT);
    result = aa_async_poll(handle, INITIAL_TIMEOUT);
    if (result == AA_ASYNC_NO_DATA) {
        printf("  no data pending.\n");
        return;
    }
    
    printf("  data received.\n");

    // Loop until aa_async_poll times out
    for (;;) {
        // Read the next monitor transaction.
        // This function has an internal timeout (see datasheet), though
        // since we have already checked for data using aa_async_poll,
        // the timeout should never be exercised.
        int num_bytes = aa_i2c_monitor_read(handle, BUFFER_SIZE, data);
        int i;

        if (num_bytes < 0) {
            printf("error: %s\n", aa_status_string(num_bytes));
            return;
        }

        for (i = 0; i < num_bytes; ++i) {
            // Check for start condition
            if (data[i] == AA_I2C_MONITOR_CMD_START) {
                char *buffer;

                time_t res;
                res = time(NULL);
                
                buffer = asctime(localtime(&res));
                buffer[strlen(buffer)-1] = 0;
                printf("\n%s - [S] ", buffer);
            }

            // Check for stop condition
            else if (data[i] == AA_I2C_MONITOR_CMD_STOP) {
                printf("[P]\n");
            }

            else {
                int nack = (data[i] & AA_I2C_MONITOR_NACK);
                // 7-bit addresses
                if (last_data0 == AA_I2C_MONITOR_CMD_START &&
                    ((data[i] & 0xf8) != 0xf0 || nack)) {
                    printf("<%02x:%c>%s ", (data[i] & 0xff) >> 1,
                           data[i] & 0x01 ? 'r' : 'w',
                           nack ? "*" : "");
                }

                // 10-bit addresses
                // See Philips specification for more details.
                else if (last_data1 == AA_I2C_MONITOR_CMD_START &&
                         (last_data0 & 0xf8) == 0xf0) {
                    printf("<%03x:%c>%s ",
                           ((last_data0 << 7) & 0x300) | (data[i] & 0xff),
                           last_data0 & 0x01 ? 'r' : 'w',
                           nack ? "*" : "");
                }

                // Normal data
                else if (last_data0 != AA_I2C_MONITOR_CMD_START) {
                    printf("%02x%s ", data[i] & 0xff,
                           nack ? "*" : "");
                }
            }

            last_data1 = last_data0;
            last_data0 = data[i];
            fflush(stdout);
        }

        printf("\nWaiting %d ms for subsequent transaction...\n",
               INTERVAL_TIMEOUT);
        result = aa_async_poll(handle, INTERVAL_TIMEOUT);
        if (result == AA_ASYNC_NO_DATA) {
            printf("  no more data pending.\n");
            break;
        }
        printf(" data received.\n\n");
    }
}


//=========================================================================
// MAIN PROGRAM
//=========================================================================
int main (int argc, char *argv[]) {
    int port;
    Aardvark handle;
    AardvarkExt aaext;
    int result;

    if (argc < 2) {
        printf("usage: aamonitor PORT\n");
        return 1;
    }

    // Open the device
    port    = atoi(argv[1]);
    handle  = aa_open_ext(port, &aaext);

    if (handle <= 0) {
        printf("Unable to open Aardvark device on port %d\n", port);
        printf("Error code = %d\n", handle);
        return 1;
    }

    printf("Opened Aardvark; features = 0x%02x\n", aaext.features);

    // Disable the I2C bus pullup resistors (2.2k resistors).
    // This command is only effective on v2.0 hardware or greater.
    // The pullup resistors on the v1.02 hardware are enabled by default.
    aa_i2c_pullup(handle, AA_I2C_PULLUP_NONE);

    // Disable the Aardvark adapter's power pins.
    // This command is only effective on v2.0 hardware or greater.
    // The power pins on the v1.02 hardware are not enabled by default.
    aa_target_power(handle, AA_TARGET_POWER_NONE);

    // Enable the monitor
    result = aa_i2c_monitor_enable(handle);
    if (result < 0) {
        printf("error: %s\n", aa_status_string(result));
        return 1;
    }
    printf("Enabled I2C monitor.\n");

    // Dump the data to the console
    dump(handle);
    
    // Disable the monitor and close the device
    aa_i2c_monitor_disable(handle);
    aa_close(handle);

    return 0;
}
