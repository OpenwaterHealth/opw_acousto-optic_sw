/*=========================================================================
| (c) 2006-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aadetect.cs
|--------------------------------------------------------------------------
| Auto-detection test routine
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
public class AADetect {

    /*=====================================================================
    | MAIN PROGRAM
     ====================================================================*/
    public static void Main (string[] args)
    {
        ushort[] ports      = new ushort[16];
        uint[]   uniqueIds  = new uint[16];
        int     numElem    = 16;
        int     i;

        // Find all the attached devices
        int count = AardvarkApi.aa_find_devices_ext(numElem, ports,
                                                    numElem, uniqueIds);

        Console.WriteLine("{0} device(s) found:", count);
        if (count > numElem)  count = numElem;

        // Print the information on each device
        for (i = 0; i < count; ++i) {
            // Determine if the device is in-use
            string status = "(avail) ";
            if ((ports[i] & AardvarkApi.AA_PORT_NOT_FREE) != 0) {
                ports[i] &= unchecked((ushort)~AardvarkApi.AA_PORT_NOT_FREE);
                status = "(in-use)";
            }

            // Display device port number, in-use status, and serial number
            uint id = unchecked((uint)uniqueIds[i]);
            Console.WriteLine("    port={0,-3} {1} ({2:d4}-{3:d6})",
                              ports[i], status, id / 1000000,
                              id % 1000000);
        }

    }

}
