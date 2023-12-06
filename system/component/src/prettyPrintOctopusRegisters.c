// prettyPrintOctopusRegisters.c
// Ian O'Donnell [Openwater]
// Created 1/28/2020
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "system/component/inc/prettyPrintOctopusRegisters.h"

#ifdef __cplusplus
extern "C" {
#endif 

// Octopus helper function, matches 'sel' mux input [0:31] to name of signal
const char *OctopusSel2Name(uint32_t sel) {
  static const char NAME0[]  = "GPIO_IN[0]/DIG IN CH1";
  static const char NAME1[]  = "GPIO_IN[1]/DIG IN CH2";
  static const char NAME2[]  = "GPIO_IN[2]/DIG IN CH3";
  static const char NAME3[]  = "GPIO_IN[3]/DIG IN CH4";
  static const char NAME4[]  = "GPIO_IN[4]/DIG IN CH5";
  static const char NAME5[]  = "GPIO_IN[5]/DIG IN CH6";
  static const char NAME6[]  = "GPIO_IN[6]/DIG IN CH7";
  static const char NAME7[]  = "GPIO_IN[7]/DIG IN CH8";
  static const char NAME8[]  = "Timer[ 0]/DIG OUT CH1 BOT";
  static const char NAME9[]  = "Timer[ 1]/DIG OUT CH2 BOT";
  static const char NAME10[] = "Timer[ 2]/DIG OUT CH3 BOT";
  static const char NAME11[] = "Timer[ 3]/DIG OUT CH4 BOT";
  static const char NAME12[] = "Timer[ 4]/DIG OUT CH5 BOT";
  static const char NAME13[] = "Timer[ 5]/DIG OUT CH6 BOT";
  static const char NAME14[] = "Timer[ 6]/DIG OUT CH7 BOT";
  static const char NAME15[] = "Timer[ 7]/DIG OUT CH8 BOT";
  static const char NAME16[] = "Timer[ 8]/DIG OUT CH1 TOP";
  static const char NAME17[] = "Timer[ 9]/DIG OUT CH2 TOP";
  static const char NAME18[] = "Timer[10]/DIG OUT CH3 TOP";
  static const char NAME19[] = "Timer[11]/DIG OUT CH4 TOP";
  static const char NAME20[] = "Timer[12]/DIG OUT CH5 TOP";
  static const char NAME21[] = "Timer[13]/DIG OUT CH6 TOP";
  static const char NAME22[] = "Timer[14]/DIG OUT CH7 TOP";
  static const char NAME23[] = "Timer[15]/DIG OUT CH8 TOP";
  static const char NAME24[] = "hard-coded 0";
  static const char NAME25[] = "hard-coded 0";
  static const char NAME26[] = "hard-coded 0";
  static const char NAME27[] = "hard-coded 0";
  static const char NAME28[] = "hard-coded 0";
  static const char NAME29[] = "hard-coded 0";
  static const char NAME30[] = "hard-coded 0";
  static const char NAME31[] = "HARD-CODED 1";
  static const char ERROR[] =
      "ERROR: 'OctopusSel2Name(sel)' out of bounds [0:31]";

  switch (sel) {
    case 0:
      return &NAME0[0];
      break;
    case 1:
      return &NAME1[0];
      break;
    case 2:
      return &NAME2[0];
      break;
    case 3:
      return &NAME3[0];
      break;
    case 4:
      return &NAME4[0];
      break;
    case 5:
      return &NAME5[0];
      break;
    case 6:
      return &NAME6[0];
      break;
    case 7:
      return &NAME7[0];
      break;
    case 8:
      return &NAME8[0];
      break;
    case 9:
      return &NAME9[0];
      break;
    case 10:
      return &NAME10[0];
      break;
    case 11:
      return &NAME11[0];
      break;
    case 12:
      return &NAME12[0];
      break;
    case 13:
      return &NAME13[0];
      break;
    case 14:
      return &NAME14[0];
      break;
    case 15:
      return &NAME15[0];
      break;
    case 16:
      return &NAME16[0];
      break;
    case 17:
      return &NAME17[0];
      break;
    case 18:
      return &NAME18[0];
      break;
    case 19:
      return &NAME19[0];
      break;
    case 20:
      return &NAME20[0];
      break;
    case 21:
      return &NAME21[0];
      break;
    case 22:
      return &NAME22[0];
      break;
    case 23:
      return &NAME23[0];
      break;
    case 24:
      return &NAME24[0];
      break;
    case 25:
      return &NAME25[0];
      break;
    case 26:
      return &NAME26[0];
      break;
    case 27:
      return &NAME27[0];
      break;
    case 28:
      return &NAME28[0];
      break;
    case 29:
      return &NAME29[0];
      break;
    case 30:
      return &NAME30[0];
      break;
    case 31:
      return &NAME31[0];
      break;
    default:
      return &ERROR[0];
      break;
  }
}

// prettyPrintOctopusDDSSPI() -- pretty print DDS (AOM) SPI write or read bytes
// Returns 0 if no problems, else negative # identifying error seen
// Output to FP unless FP==NULL then to stdout
int32_t prettyPrintOctopusDDSSPI(FILE *FP, uint8_t spi_inst_byte,
                                 uint8_t *spi_data_bytes) {
  FILE *FDEST;
  int32_t err = 0;
  uint32_t spi_data_word;
  uint8_t spi_addr;
  static uint32_t dds_csr;
  static uint32_t dds_fr1;
  static uint32_t dds_fr2;
  static uint32_t dds_cfr;
  static uint32_t dds_cftw0;
  static uint32_t dds_cpow0;
  static uint32_t dds_acr;
  static uint32_t dds_lsrr;
  static uint32_t dds_rdw;
  static uint32_t dds_fdw;
  static uint32_t dds_cw[16];

  static int32_t init = 1;  // use to initialize registers to 0 on first pass
  int32_t i;

  // if no destination file specified, use stdout
  if (FP == NULL) {
    FDEST = stdout;
  } else {
    FDEST = FP;
  }

  // init regs to zero
  if (init != 0) {
    dds_csr = 0;
    dds_fr1 = 0;
    dds_fr2 = 0;
    dds_cfr = 0;
    dds_cftw0 = 0;
    dds_cpow0 = 0;
    dds_acr = 0;
    dds_lsrr = 0;
    dds_rdw = 0;
    dds_fdw = 0;
    for (i = 0; i < 16; i++) {
      dds_cw[i] = 0;
    }
    init = 0;  // done init
  }

  // if no data_bytes[], error
  if (spi_data_bytes == NULL) {
    fprintf(FDEST,
            "Error: prettyPrintOctopusDDSSPI() called will null ptr to "
            "spi_data_bytes[]?\n");
    err = -30;  // who calls this with a null ptr to the bytes?  I mean who does
                // that?
  } else {
    spi_addr = spi_inst_byte & 0x01f;
    if (spi_addr > 24) {
      fprintf(FDEST,
              "Error: prettyPrintOctopusDDSSPI() called with out of bounds "
              "[0:24] SPI Reg Addr=0x%x (%u) \n",
              spi_addr, spi_addr);
      err = -24;
    } else {
      // Get num of expected bytes -- note spi_data_word is MS-bit aligned
      switch (spi_addr) {
        case 0:
          spi_data_word = (((uint32_t)spi_data_bytes[0]) << 24);
          break;
        case 2:
        case 5:
        case 7:
          spi_data_word = (((uint32_t)spi_data_bytes[0]) << 24) |
                          (((uint32_t)spi_data_bytes[1]) << 16);
          break;
        case 1:
        case 3:
        case 6:
          spi_data_word = (((uint32_t)spi_data_bytes[0]) << 24) |
                          (((uint32_t)spi_data_bytes[1]) << 16) |
                          (((uint32_t)spi_data_bytes[2]) << 8);
          break;
        case 4:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
          spi_data_word = (((uint32_t)spi_data_bytes[0]) << 24) |
                          (((uint32_t)spi_data_bytes[1]) << 16) |
                          (((uint32_t)spi_data_bytes[2]) << 8) |
                          ((uint32_t)spi_data_bytes[3]);
          break;
        default:
          fprintf(FDEST,
                  "Internal Error: prettyPrintOctopusDDSSPI() called with out "
                  "of bounds [0:24] SPI Reg Addr=0x%x (%u) but never should "
                  "have hit this default case... \n",
                  spi_addr, spi_addr);
          err = -25;
          break;
      }
      // Commence pretty-printing
      if ((spi_inst_byte & 0x80) != 0) {
        fprintf(FDEST,
                "        AD9959 SPI READ:  INST_BYTE=0x%02x [Addr=0x%02x (%u)] "
                " DATA=0x%08x\n",
                spi_inst_byte, spi_addr, spi_addr, spi_data_word);
      } else {
        fprintf(FDEST,
                "        AD9959 SPI WRITE: INST_BYTE=0x%02x [Addr=0x%02x (%u)] "
                " DATA=0x%08x\n",
                spi_inst_byte, spi_addr, spi_addr, spi_data_word);
      }
      switch (spi_addr) {
        case 0:  // CSR
          dds_csr = (spi_data_word & 0xff000000) >> 24;
          fprintf(FDEST,
                  "        AD9959 Control Select Register (CSR): 0x%01x\n",
                  dds_csr & 0x0ff);
          fprintf(
              FDEST,
              "        AD9959 CSR: CH3_EN                     = 0x%x (%u)\n",
              (dds_csr & 0x80) >> 7, (dds_csr & 0x80) >> 7);
          fprintf(
              FDEST,
              "        AD9959 CSR: CH2_EN                     = 0x%x (%u)\n",
              (dds_csr & 0x40) >> 6, (dds_csr & 0x40) >> 6);
          fprintf(
              FDEST,
              "        AD9959 CSR: CH1_EN                     = 0x%x (%u)\n",
              (dds_csr & 0x20) >> 5, (dds_csr & 0x20) >> 5);
          fprintf(
              FDEST,
              "        AD9959 CSR: CH0_EN                     = 0x%x (%u)\n",
              (dds_csr & 0x10) >> 4, (dds_csr & 0x10) >> 4);
          if (((dds_csr & 0x08) >> 3) != 0) {
            fprintf(FDEST,
                    "Error AD9959 CSR bit[3] is expected to be zero but it "
                    "isn't.\n");
          }
          fprintf(
              FDEST,
              "        AD9959 CSR: I/O MODE SEL               = 0x%x (%u) : ",
              (dds_csr & 0x06) >> 1, (dds_csr & 0x06) >> 1);
          switch ((dds_csr & 0x06) >> 1) {
            case 0:
              fprintf(FDEST, "single-bit serial (2-wire)\n");
              break;
            case 1:
              fprintf(FDEST, "single-bit serial (3-wire)\n");
              break;
            case 2:
              fprintf(FDEST, "2-bit serial\n");
              break;
            case 3:
              fprintf(FDEST, "4-bit serial\n");
              break;
          }
          fprintf(
              FDEST,
              "        AD9959 CSR: LSb FIRST                  = 0x%x (%u)\n",
              dds_csr & 0x01, dds_csr & 0x01);
          break;
        case 1:  // FR1
          dds_fr1 = (spi_data_word & 0xffffff00) >> 8;
          fprintf(FDEST, "        AD9959 Function Register 1 (FR1): 0x%04x\n",
                  spi_data_word);
          fprintf(
              FDEST,
              "        AD9959 FR1: VCO gain ctrl              = 0x%x (%u) : ",
              (dds_fr1 & 0x800000) >> 23, (dds_fr1 & 0x800000) >> 23);
          switch ((dds_fr1 & 0x800000) >> 23) {
            case 0:
              fprintf(FDEST, "<160MHz\n");
              break;
            case 1:
              fprintf(FDEST, "255-500MHz\n");
              break;
          }
          fprintf(
              FDEST,
              "        AD9959 FR1: PLL div ratio              = 0x%x (%u)\n",
              (dds_fr1 & 0x7c0000) >> 18, (dds_fr1 & 0x7c0000) >> 18);
          fprintf(
              FDEST,
              "        AD9959 FR1: CP ctrl                    = 0x%x (%u): ",
              (dds_fr1 & 0x030000) >> 16, (dds_fr1 & 0x030000) >> 16);
          switch ((dds_fr1 & 0x030000) >> 16) {
            case 0:
              fprintf(FDEST, "75uA\n");
              break;
            case 1:
              fprintf(FDEST, "100uA\n");
              break;
            case 2:
              fprintf(FDEST, "125uA\n");
              break;
            case 3:
              fprintf(FDEST, "150uA\n");
              break;
          }
          fprintf(
              FDEST,
              "        AD9959 FR1: Profile Pin Ctrl           = 0x%x (%u)\n",
              (dds_fr1 & 0x007000) >> 12, (dds_fr1 & 0x007000) >> 12);
          fprintf(
              FDEST,
              "        AD9959 FR1: Ramp-up/down               = 0x%x (%u)\n",
              (dds_fr1 & 0x000c00) >> 10, (dds_fr1 & 0x000c00) >> 10);
          fprintf(
              FDEST,
              "        AD9959 FR1: Mod level                  = 0x%x (%u)\n",
              (dds_fr1 & 0x000300) >> 8, (dds_fr1 & 0x000300) >> 8);
          fprintf(
              FDEST,
              "        AD9959 FR1: Ref clk pdn                = 0x%x (%u)\n",
              (dds_fr1 & 0x000080) >> 7, (dds_fr1 & 0x000080) >> 7);
          fprintf(
              FDEST,
              "        AD9959 FR1: Ext pdn mode               = 0x%x (%u)\n",
              (dds_fr1 & 0x000040) >> 6, (dds_fr1 & 0x000040) >> 6);
          fprintf(
              FDEST,
              "        AD9959 FR1: SYNC_CLK disable           = 0x%x (%u)\n",
              (dds_fr1 & 0x000020) >> 5, (dds_fr1 & 0x000020) >> 5);
          fprintf(
              FDEST,
              "        AD9959 FR1: DAC ref pdn                = 0x%x (%u)\n",
              (dds_fr1 & 0x000010) >> 4, (dds_fr1 & 0x000010) >> 4);
          fprintf(
              FDEST,
              "        AD9959 FR1: Manual hw sync             = 0x%x (%u)\n",
              (dds_fr1 & 0x000002) >> 1, (dds_fr1 & 0x000002) >> 1);
          fprintf(
              FDEST,
              "        AD9959 FR1: Manual sw sync             = 0x%x (%u)\n",
              dds_fr1 & 0x000001, dds_fr1 & 0x000001);
          break;
        case 2:  // FR2
          dds_fr2 = (spi_data_word & 0xffff0000) >> 16;
          fprintf(FDEST, "        AD9959 Function Register 2 (FR2): 0x%04x\n",
                  spi_data_word);
          fprintf(
              FDEST,
              "        AD9959 FR2: All chan autoclr sweep acc = 0x%x (%u)\n",
              (dds_fr2 & 0x8000) >> 15, (dds_fr2 & 0x8000) >> 15);
          fprintf(
              FDEST,
              "        AD9959 FR2: All chan clr sweep acc     = 0x%x (%u)\n",
              (dds_fr2 & 0x4000) >> 14, (dds_fr2 & 0x4000) >> 14);
          fprintf(
              FDEST,
              "        AD9959 FR2: All chan autoclr phase acc = 0x%x (%u)\n",
              (dds_fr2 & 0x2000) >> 13, (dds_fr2 & 0x2000) >> 13);
          fprintf(
              FDEST,
              "        AD9959 FR2: All chan clr phase acc     = 0x%x (%u)\n",
              (dds_fr2 & 0x1000) >> 12, (dds_fr2 & 0x1000) >> 12);
          fprintf(
              FDEST,
              "        AD9959 FR2: Auto sync en               = 0x%x (%u)\n",
              (dds_fr2 & 0x0080) >> 7, (dds_fr2 & 0x0080) >> 7);
          fprintf(
              FDEST,
              "        AD9959 FR2: Multidev sync master en    = 0x%x (%u)\n",
              (dds_fr2 & 0x0040) >> 6, (dds_fr2 & 0x0040) >> 6);
          fprintf(
              FDEST,
              "        AD9959 FR2: Multidev sync status       = 0x%x (%u)\n",
              (dds_fr2 & 0x0020) >> 5, (dds_fr2 & 0x0020) >> 5);
          fprintf(
              FDEST,
              "        AD9959 FR2: Multidev sync mask         = 0x%x (%u)\n",
              (dds_fr2 & 0x0010) >> 4, (dds_fr2 & 0x0010) >> 4);
          fprintf(
              FDEST,
              "        AD9959 FR2: System clk offset          = 0x%x (%u)\n",
              dds_fr2 & 0x0003, dds_fr2 & 0x0003);
          break;
        case 3:
          dds_cfr = (spi_data_word & 0xffffff00) >> 8;
          fprintf(FDEST, "        AD9959 Channel Function Reg (CFR): 0x%04x\n",
                  spi_data_word);
          fprintf(
              FDEST,
              "        AD9959 CFR: Amp Freq Phase sel         = 0x%x (%u)\n",
              (dds_cfr & 0xc00000) >> 22, (dds_cfr & 0xc00000) >> 22);
          fprintf(
              FDEST,
              "        AD9959 CFR:Lin Sweep no-dwell          = 0x%x (%u)\n",
              (dds_cfr & 0x008000) >> 15, (dds_cfr & 0x008000) >> 15);
          fprintf(
              FDEST,
              "        AD9959 CFR:Lin Sweep en                = 0x%x (%u)\n",
              (dds_cfr & 0x004000) >> 14, (dds_cfr & 0x004000) >> 14);
          fprintf(
              FDEST,
              "        AD9959 CFR:Load SRR at IO_UPDATE       = 0x%x (%u)\n",
              (dds_cfr & 0x002000) >> 13, (dds_cfr & 0x002000) >> 13);
          if ((dds_cfr & 0x000400) >> 10 != 0) {
            fprintf(FDEST,
                    "Error AD9959 CFR bit[10] is expected to be zero but it "
                    "isn't.\n");
          }
          fprintf(
              FDEST,
              "        AD9959 CFR:DAC FullScale Ctrl          = 0x%x (%u)\n",
              (dds_cfr & 0x000300) >> 8, (dds_cfr & 0x000300) >> 8);
          fprintf(
              FDEST,
              "        AD9959 CFR:Digital pdn                 = 0x%x (%u)\n",
              (dds_cfr & 0x000080) >> 7, (dds_cfr & 0x000080) >> 7);
          fprintf(
              FDEST,
              "        AD9959 CFR:DAC pdn                     = 0x%x (%u)\n",
              (dds_cfr & 0x000040) >> 6, (dds_cfr & 0x000040) >> 6);
          fprintf(
              FDEST,
              "        AD9959 CFR:Matched pipe delays         = 0x%x (%u)\n",
              (dds_cfr & 0x000020) >> 5, (dds_cfr & 0x000020) >> 5);
          fprintf(
              FDEST,
              "        AD9959 CFR:Autoclr sweep acc           = 0x%x (%u)\n",
              (dds_cfr & 0x000018) >> 3, (dds_cfr & 0x000018) >> 3);
          fprintf(
              FDEST,
              "        AD9959 CFR:Autoclr phase acc           = 0x%x (%u)\n",
              (dds_cfr & 0x000006) >> 1, (dds_cfr & 0x000006) >> 1);
          fprintf(
              FDEST,
              "        AD9959 CFR:Sine wave (vs not Cos)      = 0x%x (%u)\n",
              dds_cfr & 0xc00001, dds_cfr & 0xc00001);
          break;
        case 4:
          dds_cftw0 = spi_data_word;
          fprintf(FDEST,
                  "        AD9959 Channel Freq Tune Word 0 (CFTW0): 0x%04x\n",
                  spi_data_word);
          fprintf(
              FDEST,
              "        AD9959 CFTW0: Freq Tuning Word         = 0x%x (%u)\n",
              dds_cftw0, dds_cftw0);
          break;
        case 5:
          dds_cpow0 = (spi_data_word & 0x3fff0000) >> 16;
          fprintf(
              FDEST,
              "        AD9959 Channel Phase Offset Word 0 (CPOW0): 0x%04x\n",
              spi_data_word);
          fprintf(
              FDEST,
              "        AD9959 CPOW0: Phase Word               = 0x%x (%u)\n",
              dds_cpow0, dds_cpow0);
          break;
        case 6:
          dds_acr = (spi_data_word & 0xffffff00) >> 8;
          fprintf(FDEST,
                  "        AD9959 Amplitude Control Register (ACR): 0x%04x\n",
                  spi_data_word);
          fprintf(
              FDEST,
              "        AD9959 ACR: Amp ramp rate              = 0x%x (%u)\n",
              (dds_acr & 0xff0000) >> 16, (dds_acr & 0xff0000) >> 16);
          fprintf(
              FDEST,
              "        AD9959 ACR: Incr/Decr step             = 0x%x (%u)\n",
              (dds_acr & 0x00c000) >> 14, (dds_acr & 0x00c000) >> 14);
          fprintf(
              FDEST,
              "        AD9959 ACR: Amp mult en                = 0x%x (%u)\n",
              (dds_acr & 0x001000) >> 12, (dds_acr & 0x001000) >> 12);
          fprintf(
              FDEST,
              "        AD9959 ACR: Ramp up/down en            = 0x%x (%u)\n",
              (dds_acr & 0x000800) >> 11, (dds_acr & 0x000800) >> 11);
          fprintf(
              FDEST,
              "        AD9959 ACR: Load ARR on IO_UPDATE      = 0x%x (%u)\n",
              (dds_acr & 0x000400) >> 10, (dds_acr & 0x000400) >> 10);
          fprintf(
              FDEST,
              "        AD9959 ACR: Amp scale factor           = 0x%x (%u)\n",
              dds_acr & 0x0003ff, dds_acr & 0x0003ff);
          break;
        case 7:
          dds_lsrr = (spi_data_word & 0xffff0000) >> 16;
          fprintf(FDEST,
                  "        AD9959 Linear Sweep Ramp Rate (LSRR): 0x%02x\n",
                  dds_lsrr);
          fprintf(
              FDEST,
              "        AD9959 LSRR: Falling SRR               = 0x%0x (%u)\n",
              (dds_lsrr & 0xff00) >> 8, (dds_lsrr & 0xff00) >> 8);
          fprintf(
              FDEST,
              "        AD9959 LSRR: Rising SRR                = 0x%0x (%u)\n",
              dds_lsrr & 0x00ff, dds_lsrr & 0x00ff);
          break;
        case 8:
          dds_rdw = spi_data_word;
          fprintf(FDEST, "        AD9959 LSR Rising Delta Word (RDW): 0x%04x\n",
                  dds_rdw);
          fprintf(
              FDEST,
              "        AD9959 RDW: Rising Delta Word          = 0x%x (%u)\n",
              dds_rdw, dds_rdw);
          break;
        case 9:
          dds_fdw = spi_data_word;
          fprintf(FDEST,
                  "        AD9959 LSR Falling Delta Word (FDW): 0x%04x\n",
                  dds_fdw);
          fprintf(
              FDEST,
              "        AD9959 FDW: Falling Delta Word         = 0x%x (%u)\n",
              dds_fdw, dds_fdw);
          break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
          dds_cw[spi_addr - 9] = spi_data_word;
          fprintf(FDEST, "        AD9959 Channel Word %u (CW%u): 0x%04x\n",
                  spi_addr - 9, spi_addr - 9, dds_cw[spi_addr - 9]);
          fprintf(FDEST,
                  "        AD9959 CW%u: Freq Tuning Word    = 0x%x (%u)\n",
                  spi_addr - 9, dds_cw[spi_addr - 9], dds_cw[spi_addr - 9]);
          fprintf(FDEST,
                  "        AD9959 CW%u: or Phase Word       = 0x%x (%u)\n",
                  spi_addr - 9, dds_cw[spi_addr - 9] >> 18,
                  dds_cw[spi_addr - 9] >> 18);
          fprintf(FDEST,
                  "        AD9959 CW%u: or Amplitude Word   = 0x%x (%u)\n",
                  spi_addr - 9, dds_cw[spi_addr - 9] >> 22,
                  dds_cw[spi_addr - 9] >> 22);
          break;
        default:
          fprintf(FDEST,
                  "Internal Error: prettyPrintOctopusDDSSPI() called with out "
                  "of bounds [0:24] SPI Reg Addr=0x%x (%u) but never should "
                  "have hit this default case... \n",
                  spi_addr, spi_addr);
          err = -26;
          break;
      }  // done switch(spi_addr)

    }  // spi_addr in range -- if( spi_addr <= 24)
  }    // spi_data_bytes non-NULL

  return err;
}

// prettyPrintOctopusReg() -- print/process the contents of one register
// write/read If you want to process a buffer of bytes (a collection of register
// read/writes) use prettyPrintOctopusRegisters() below Registers 'pretty'
// printed by bitfields to FP (or stdout if FP==NULL) Note only bottom 16 bits
// of octopus_data/addr 32-bit words are used. Returns 0 if no problems, else
// negative # identifying error seen Output to FP unless FP==NULL then to stdout
int32_t prettyPrintOctopusReg(FILE *FP, uint32_t octopus_addr,
                              uint32_t octopus_data) {
  FILE *FDEST;
  int32_t err = 0;
  uint32_t octopus_read_not_write;
  uint32_t octopus_module, om;
  uint32_t octopus_module_reg;

  uint32_t ul;

  static int32_t init = 1;  // use to initialize registers to 0 on first pass
  int32_t i, j;

  // Octopus registers
  static uint32_t Timer_Reg[16][17];
  static uint32_t Mod30_Reg[8];
  static uint8_t Mod30_Inst_Byte;
  static uint8_t Mod30_Data_Bytes[4];
  static uint32_t Mod31_Reg0;

  // if no destination file specified, use stdout
  if (FP == NULL) {
    FDEST = stdout;
  } else {
    FDEST = FP;
  }

  // init registers to zero
  if (init != 0) {
    for (i = 0; i < 16; i++) {
      for (j = 0; j < 17; j++) {
        Timer_Reg[i][j] = 0;
      }
    }
    for (i = 0; i < 8; i++) {
      Mod30_Reg[i] = 0;
    }
    Mod31_Reg0 = 0;
    init = 0;  // done init
  }

  // Print short (2-byte) address/data info
  octopus_read_not_write = (octopus_addr & 0x8000) ? 1 : 0;
  octopus_module = (octopus_addr & 0x1f00) >> 8;
  octopus_module_reg = (octopus_addr & 0x00ff);
  if (octopus_read_not_write != 0) {
    fprintf(FDEST,
            "Octopus WRITE: 0x%04x Module=%u (0x%2x) RegAddr=%3u (0x%2x) "
            "Data=0x%04x\n",
            octopus_addr, octopus_module, octopus_module, octopus_module_reg,
            octopus_module_reg, octopus_data);
  } else {
    fprintf(FDEST,
            "Octopus READ:  0x%04x Module=%u (0x%2x) RegAddr=%3u (0x%2x) "
            "Data=0x%04x\n",
            octopus_addr, octopus_module, octopus_module, octopus_module_reg,
            octopus_module_reg, octopus_data);
  }
  switch (octopus_module) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:  // Timers 0:15
      if (octopus_module_reg > 16) {
        fprintf(FDEST,
                "Error: prettyPrintOctopusReg() called with out of bounds "
                "[0:16] Reg=%u for Timer=%u?\n",
                octopus_module_reg, octopus_module);
        err = -7;  // Timer reg out of bounds [0:16]
      } else {
        ul = octopus_data & 0x0ffff;
        om = octopus_module;
        Timer_Reg[om][octopus_module_reg] = ul;
        // now pretty print it
        switch (octopus_module_reg) {
          case 0:
            fprintf(FDEST, "    Timer%u_Reg0: DATA=0x%04x\n", octopus_module,
                    ul);
            fprintf(FDEST,
                    "    Timer%u_Reg0[   15]: Unused                = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x08000) >> 15, (ul & 0x08000) >> 15);
            fprintf(FDEST,
                    "    Timer%u_Reg0[   14]: End:  OutputValue     = 0x%02x "
                    "(%u)\n",
                    octopus_module, (ul & 0x04000) >> 14, (ul & 0x04000) >> 14);
            fprintf(FDEST,
                    "    Timer%u_Reg0[   13]: Start:OutputValue     = 0x%02x "
                    "(%u)\n",
                    octopus_module, (ul & 0x02000) >> 13, (ul & 0x02000) >> 13);
            fprintf(FDEST,
                    "    Timer%u_Reg0[   12]: GateSense             = 0x%02x "
                    "(%u): Level==%u\n",
                    octopus_module, (ul & 0x01000) >> 12, (ul & 0x01000) >> 12,
                    (ul & 0x01000) >> 12);
            fprintf(FDEST,
                    "    Timer%u_Reg0[11: 7]: GateSelect            = 0x%02x "
                    "(%u): %s\n",
                    octopus_module, (ul & 0x00f80) >> 7, (ul & 0x00f80) >> 7,
                    OctopusSel2Name((ul & 0x00f80) >> 7));
            fprintf(FDEST,
                    "    Timer%u_Reg0[ 6: 5]: TriggerSense          = 0x%02x "
                    "(%u): ",
                    octopus_module, (ul & 0x00060) >> 5, (ul & 0x00060) >> 5);
            switch ((ul & 0x00060) >> 5) {
              case 0:
                fprintf(FDEST, "Level==0\n");
                break;
              case 1:
                fprintf(FDEST, "Rising Edge 0->1\n");
                break;
              case 2:
                fprintf(FDEST, "Falling Edge 1->0\n");
                break;
              case 3:
                fprintf(FDEST, "Level==1\n");
                break;
            }
            fprintf(FDEST,
                    "    Timer%u_Reg0[ 4: 0]: TriggerSelect         = 0x%02x "
                    "(%u): %s\n",
                    octopus_module, (ul & 0x0001f), (ul & 0x0001f),
                    OctopusSel2Name((ul & 0x0001f) ));
            break;
          case 1:
            fprintf(FDEST, "    Timer%u_Reg1: DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg1[   15]: Unused                = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x08000) >> 15, (ul & 0x08000) >> 15);
            fprintf(FDEST,
                    "    Timer%u_Reg1[14:12]: S3:NextState          = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x07000) >> 12, (ul & 0x07000) >> 12);
            fprintf(FDEST,
                    "    Timer%u_Reg1[   11]: S3:UseTrigger         = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00800) >> 11, (ul & 0x00800) >> 11);
            fprintf(FDEST,
                    "    Timer%u_Reg1[   10]: S3:OutputValue        = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00400) >> 10, (ul & 0x00400) >> 10);
            fprintf(FDEST,
                    "    Timer%u_Reg1[ 9: 7]: S2:NextState          = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00380) >> 7, (ul & 0x00380) >> 7);
            fprintf(FDEST,
                    "    Timer%u_Reg1[    6]: S2:UseTrigger         = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00040) >> 6, (ul & 0x00040) >> 6);
            fprintf(FDEST,
                    "    Timer%u_Reg1[    5]: S2:OutputValue        = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00020) >> 5, (ul & 0x00020) >> 5);
            fprintf(FDEST,
                    "    Timer%u_Reg1[ 4: 2]: S1:NextState          = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x0001c) >> 2, (ul & 0x0001c) >> 2);
            fprintf(FDEST,
                    "    Timer%u_Reg1[    1]: S1:UseTrigger         = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00002) >> 1, (ul & 0x00002) >> 1);
            fprintf(FDEST,
                    "    Timer%u_Reg1[    0]: S1:OutputValue        = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00001), (ul & 0x00001));
            break;
          case 2:
            fprintf(FDEST, "    Timer%u_Reg2: DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg2[15: 0]: S1:CountLower         =0x%04x "
                    "[S1:Count=0x%08x (%u)]\n",
                    octopus_module, ul, (Timer_Reg[om][3] << 16) | ul,
                    (Timer_Reg[om][3] << 16) | ul);
            break;
          case 3:
            fprintf(FDEST, "    Timer%u_Reg3: DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg3[15: 0]: S1:CountUpper         =0x%04x "
                    "[S1:Count=0x%08x (%u)]\n",
                    octopus_module, ul, (ul << 16) | Timer_Reg[om][2],
                    (ul << 16) | Timer_Reg[om][2]);
            break;
          case 4:
            fprintf(FDEST, "    Timer%u_Reg4: DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg4[15: 0]: S2:CountLower         =0x%04x "
                    "[S2:Count=0x%08x (%u)]\n",
                    octopus_module, ul, (Timer_Reg[om][5] << 16) | ul,
                    (Timer_Reg[om][5] << 16) | ul);
            break;
          case 5:
            fprintf(FDEST, "    Timer%u_Reg5: DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg5[15: 0]: S2:CountUpper         =0x%04x "
                    "[S2:Count=0x%08x (%u)]\n",
                    octopus_module, ul, (ul << 16) | Timer_Reg[om][4],
                    (ul << 16) | Timer_Reg[om][4]);
            break;
          case 6:
            fprintf(FDEST, "    Timer%u_Reg6: DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg6[15: 0]: S3:CountLower         =0x%04x "
                    "[S3:Count=0x%08x (%u)]\n",
                    octopus_module, ul, (Timer_Reg[om][7] << 16) | ul,
                    (Timer_Reg[om][7] << 16) | ul);
            break;
          case 7:
            fprintf(FDEST, "    Timer%u_Reg7: DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg7[15: 0]: S3:CountUpper         =0x%04x "
                    "[S3:Count=0x%08x (%u)]\n",
                    octopus_module, ul, (ul << 16) | Timer_Reg[om][6],
                    (ul << 16) | Timer_Reg[om][6]);
            break;
          case 8:
            fprintf(FDEST, "    Timer%u_Reg8: DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg8[   15]: Unused                = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x08000) >> 15, (ul & 0x08000) >> 15);
            fprintf(FDEST,
                    "    Timer%u_Reg8[14:12]: S6:NextState          = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x07000) >> 12, (ul & 0x07000) >> 12);
            fprintf(FDEST,
                    "    Timer%u_Reg8[   11]: S6:UseTrigger         = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00800) >> 11, (ul & 0x00800) >> 11);
            fprintf(FDEST,
                    "    Timer%u_Reg8[   10]: S6:OutputValue        = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00400) >> 10, (ul & 0x00400) >> 10);
            fprintf(FDEST,
                    "    Timer%u_Reg8[ 9: 7]: S5:NextState          = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00380) >> 7, (ul & 0x00380) >> 7);
            fprintf(FDEST,
                    "    Timer%u_Reg8[    6]: S5:UseTrigger         = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00040) >> 6, (ul & 0x00040) >> 6);
            fprintf(FDEST,
                    "    Timer%u_Reg8[    5]: S5:OutputValue        = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00020) >> 5, (ul & 0x00020) >> 5);
            fprintf(FDEST,
                    "    Timer%u_Reg8[ 4: 2]: S4:NextState          = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x0001c) >> 2, (ul & 0x0001c) >> 2);
            fprintf(FDEST,
                    "    Timer%u_Reg8[    1]: S4:UseTrigger         = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00002) >> 1, (ul & 0x00002) >> 1);
            fprintf(FDEST,
                    "    Timer%u_Reg8[    0]: S4:OutputValue        = 0x%01x "
                    "(%u)\n",
                    octopus_module, (ul & 0x00001), (ul & 0x00001));
            break;
          case 9:
            fprintf(FDEST, "    Timer%u_Reg9: DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg9[15:0]: S4:CountLower          =0x%04x "
                    "[S4:Count=0x%08x (%u)]\n",
                    octopus_module, ul, (Timer_Reg[om][10] << 16) | ul,
                    (Timer_Reg[om][10] << 16) | ul);
            break;
          case 10:
            fprintf(FDEST, "    Timer%u_Reg10:DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg10[15:0]:S4:CountUpper          =0x%04x "
                    "[S4:Count=0x%08x (%u)]\n",
                    octopus_module, ul, (ul << 16) | Timer_Reg[om][9],
                    (ul << 16) | Timer_Reg[om][9]);
            break;
          case 11:
            fprintf(FDEST, "    Timer%u_Reg11:DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg11[15:0]:S5:CountLower          =0x%04x "
                    "[S5:Count=0x%08x (%u)]\n",
                    octopus_module, ul, (Timer_Reg[om][12] << 16) | ul,
                    (Timer_Reg[om][12] << 16) | ul);
            break;
          case 12:
            fprintf(FDEST, "    Timer%u_Reg12:DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg12[15:0]:S5:CountUpper          =0x%04x "
                    "[S5:Count=0x%08x (%u)]\n",
                    octopus_module, ul, (ul << 16) | Timer_Reg[om][11],
                    (ul << 16) | Timer_Reg[om][11]);
            break;
          case 13:
            fprintf(FDEST, "    Timer%u_Reg13:DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg13[15:0]:S6:CountLower          =0x%04x "
                    "[S6:Count=0x%08x (%u)]\n",
                    octopus_module, ul, (Timer_Reg[om][14] << 16) | ul,
                    (Timer_Reg[om][14] << 16) | ul);
            break;
          case 14:
            fprintf(FDEST, "    Timer%u_Reg14:DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg14[15:0]:S6:CountUpper          =0x%04x "
                    "[S6:Count=0x%08x (%u)]\n",
                    octopus_module, ul, (ul << 16) | Timer_Reg[om][13],
                    (ul << 16) | Timer_Reg[om][13]);
            break;
          case 15:
            fprintf(FDEST, "    Timer%u_Reg15:DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg15[15:0]:TransitionCountLower   =0x%04x "
                    "[TransitionCount=0x%08x (%u)]\n",
                    octopus_module, ul, (Timer_Reg[om][16] << 16) | ul,
                    (Timer_Reg[om][16] << 16) | ul);
            break;
          case 16:
            fprintf(FDEST, "    Timer%u_Reg16:DATA=0x%04x\n", octopus_module,
                    octopus_data);
            fprintf(FDEST,
                    "    Timer%u_Reg16[15:0]:TransitionCountUpper   =0x%04x "
                    "[TransitionCount=0x%08x (%u)]\n",
                    octopus_module, ul, (ul << 16) | Timer_Reg[om][15],
                    (ul << 16) | Timer_Reg[om][15]);
            break;
        }  // switch( octopus_module_reg )
      }    // octopus_module_reg =[0:16]
      break;
    case 30:
      if ((octopus_module_reg == 3) || (octopus_module_reg == 4) ||
          (octopus_module_reg > 7)) {
        fprintf(FDEST,
                "Error: prettyPrintOctopusReg() called with Reg=%u (only 0:2 "
                "and 5:7 are valid) for Module30 (DDS/AOM)\n",
                octopus_module_reg);
        err = -30;  // output of bounds, only reg0:2 and reg5:7 are valid
      } else {
        Mod30_Reg[octopus_module_reg] = (octopus_data & 0x0ffff);
        switch (octopus_module_reg) {
          case 0:
            fprintf(FDEST, "    DDS/AOM: INST_BYTE=0x%02x (%u)",
                    (Mod30_Reg[0] & 0x000ff), (Mod30_Reg[0] & 0x000ff));
            fprintf(FDEST, " BYTE_COUNT=%u\n", (Mod30_Reg[0] & 0x00700) >> 8);
            break;
          case 1:
            fprintf(
                FDEST, "    DDS/AOM: DATA_LOWER=0x%04x [SPI DATA=%08x]\n",
                (Mod30_Reg[1] & 0x0ffff),
                ((Mod30_Reg[2] & 0x0ffff) << 16) | (Mod30_Reg[1] & 0x0ffff));
            break;
          case 2:
            fprintf(
                FDEST, "    DDS/AOM: DATA_UPPER=0x%04x [SPI DATA=%08x]\n",
                (Mod30_Reg[2] & 0x0ffff),
                ((Mod30_Reg[2] & 0x0ffff) << 16) | (Mod30_Reg[1] & 0x0ffff));
            break;
          case 5:
            fprintf(FDEST, "    DDS/AOM: CONFIG_LOWER=0x%04x\n",
                    (Mod30_Reg[5] & 0x0ffff));
            fprintf(
                FDEST,
                "    DDS/AOM: CONFIG_LOWER[15]              Unused=0x%x (%u)\n",
                (Mod30_Reg[5] & 0x08000) >> 15, (Mod30_Reg[5] & 0x08000) >> 15);
            fprintf(FDEST,
                    "    DDS/AOM: CONFIG_LOWER[14:10] (AOM CH4) P1_SEL=0x%0x "
                    "(%u) %s\n",
                    (Mod30_Reg[5] & 0x07c00) >> 10,
                    (Mod30_Reg[5] & 0x07c00) >> 10,
                    OctopusSel2Name((Mod30_Reg[5] & 0x07c00) >> 10));
            fprintf(FDEST,
                    "    DDS/AOM: CONFIG_LOWER[ 9: 5] (AOM CH1) P2_SEL=0x%0x "
                    "(%u) %s\n",
                    (Mod30_Reg[5] & 0x003e0) >> 5,
                    (Mod30_Reg[5] & 0x003e0) >> 5,
                    OctopusSel2Name((Mod30_Reg[5] & 0x003e0) >> 5));
            fprintf(FDEST,
                    "    DDS/AOM: CONFIG_LOWER[ 4: 0] (AOM CH2) P3_SEL=0x%0x "
                    "(%u) %s\n",
                    (Mod30_Reg[5] & 0x0001f), (Mod30_Reg[5] & 0x0001f),
                    OctopusSel2Name((Mod30_Reg[5] & 0x0001f)));
            break;
            //                                            15 14 13 12___11 10 9
            //                                            8___7  6  5  4___3 2 1
            //                                            0
          case 6:
            fprintf(FDEST, "    DDS/AOM: CONFIG_UPPER=0x%04x\n",
                    (Mod30_Reg[6] & 0x0ffff));
            fprintf(
                FDEST,
                "    DDS/AOM: CONFIG_UPPER[15:12]           Unused=0x%x (%u)\n",
                (Mod30_Reg[6] & 0x0f000) >> 12, (Mod30_Reg[6] & 0x0f000) >> 12);
            fprintf(FDEST,
                    "    DDS/AOM: CONFIG_UPPER[   11] (AOM CH2) EN=0x%x (%u)\n",
                    (Mod30_Reg[6] & 0x00800) >> 11,
                    (Mod30_Reg[6] & 0x00800) >> 11);
            fprintf(FDEST,
                    "    DDS/AOM: CONFIG_UPPER[   10] (AOM CH1) EN=0x%x (%u)\n",
                    (Mod30_Reg[6] & 0x00400) >> 10,
                    (Mod30_Reg[6] & 0x00400) >> 10);
            fprintf(FDEST,
                    "    DDS/AOM: CONFIG_UPPER[    9] (AOM CH4) EN=0x%x (%u)\n",
                    (Mod30_Reg[6] & 0x00200) >> 9,
                    (Mod30_Reg[6] & 0x00200) >> 9);
            fprintf(FDEST,
                    "    DDS/AOM: CONFIG_UPPER[    8] (AOM CH3) EN=0x%x (%u)\n",
                    (Mod30_Reg[6] & 0x00100) >> 8,
                    (Mod30_Reg[6] & 0x00100) >> 8);
            fprintf(
                FDEST,
                "    DDS/AOM: CONFIG_UPPER[ 7: 5]           Unused=0x%x (%u)\n",
                (Mod30_Reg[6] & 0x000e0) >> 5, (Mod30_Reg[6] & 0x000e0) >> 5);
            fprintf(FDEST,
                    "    DDS/AOM: CONFIG_UPPER[ 4: 0] (AOM CH3) P0_SEL=0x%0x "
                    "(%u) %s\n",
                    (Mod30_Reg[6] & 0x0001f), (Mod30_Reg[6] & 0x0001f),
                    OctopusSel2Name((Mod30_Reg[6] & 0x0001f)));
            break;
          case 7:
            fprintf(FDEST, "    DDS/AOM: CONTROL=0x%04x\n",
                    (Mod30_Reg[7] & 0x0ffff));
            fprintf(FDEST, "    DDS/AOM: CONTROL[15:2] Unused   =0x%x (%u)\n",
                    (Mod30_Reg[7] & 0x0fffc) >> 2,
                    (Mod30_Reg[7] & 0x0fffc) >> 2);
            fprintf(FDEST, "    DDS/AOM: CONTROL[   1] IO_UPDATE=%u \n",
                    (Mod30_Reg[7] & 0x00002) >> 1);
            fprintf(FDEST, "    DDS/AOM: CONTROL[   0] SPI_GO   =%u \n",
                    (Mod30_Reg[7] & 0x00001) );
            if ((Mod30_Reg[7] & 0x02) != 0) {
              Mod30_Reg[7] = (Mod30_Reg[7] & 0x0fffd);  // clear IO_UPDATE
            }
            if ((Mod30_Reg[7] & 0x01) != 0) {
              // SPI GO: make charbuf[4] and send INST and charbuf[4] to
              // prettyprintfunction
              Mod30_Inst_Byte = (uint8_t)(Mod30_Reg[0] & 0x000ff);
              Mod30_Data_Bytes[0] =
                  (uint8_t)((Mod30_Reg[2] & 0x0ff00) >> 8);  // MS-Byte first
              Mod30_Data_Bytes[1] = (uint8_t)(Mod30_Reg[2] & 0x000ff);
              Mod30_Data_Bytes[2] = (uint8_t)((Mod30_Reg[1] & 0x0ff00) >> 8);
              Mod30_Data_Bytes[3] = (uint8_t)(Mod30_Reg[1] & 0x000ff);
              err = prettyPrintOctopusDDSSPI(FDEST, Mod30_Inst_Byte,
                                             &Mod30_Data_Bytes[0]);
              Mod30_Reg[7] = (Mod30_Reg[7] & 0x0fffe);  // clear GO
            }
            break;
        }  // switch( octopus_module_reg )
      }
      break;
    case 31:
      if (octopus_module_reg != 0) {
        fprintf(FDEST,
                "Error: prettyPrintOctopusReg() called with Reg=%u (only 0 is "
                "valid) for Module31 (GlobalReg)\n",
                octopus_module_reg);
        err = -31;  // output of bounds, only reg0 is valid
      } else {
        Mod31_Reg0 = (octopus_data & 0x0ffff);
        // Print reg0 (Timer enables)
        fprintf(FDEST,
                "    GlobalReg0: Timer:      15 14 13 12 11 10  9  8  7  6  5  "
                "4  3  2  1  0\n");
        fprintf(FDEST, "    GlobalReg0: TimerEnable: ");
        for (i = 15; i >= 0; i--) {
          if ( ((Mod31_Reg0 >> i) & 0x1) != 0) {
            fprintf(FDEST, "Y  ");
          } else {
            fprintf(FDEST, ".  ");
          }
        }
        fprintf(FDEST, "\n");
      }
      break;
    default:
      fprintf(FDEST,
              "Error: prettyPrintOctopusReg() unrecognized module=%d (Do what "
              "with this?)\n",
              octopus_module);
      err = -3;  //
      break;
  }  // switch( octopus_module )

  return err;
}

// prettyPrintOctopusRegisters() -- process an array of bytes (e.g. sent via
// USB) of register writes/reads.  Note that we do not assume that register
// reads have their 2 byte responses inlined in the 'buf[]' array hence read
// fields are always all '1's.  Fixing this is future work, or use
// prettyPrintOctopusReg() above to process the read response (since that is
// essentially what this function is doing anyway.) Returns 0 if no problems,
// else negative # identifying error seen Output to FP unless FP==NULL then to
// stdout
int32_t prettyPrintOctopusRegisters(FILE *FP, uint8_t *buf,
                                    uint32_t num_bytes) {
  FILE *FDEST;
  int32_t err = 0;
  int32_t flag_send_reg = 0;  // mark when to call prettyPrintOctopusReg()
  uint32_t byte_count = 0;
  static uint32_t octopus_addr = 0;  // use bottom 2 Bytes
  static uint32_t octopus_data = 0;  // use bottom 2 Bytes
  static int32_t octopus_index =
      0;  // used to step through bytes in addr and data above

  // if no destination file specified, use stdout
  if (FP == NULL) {
    FDEST = stdout;
  } else {
    FDEST = FP;
  }

  // if no buf[], error
  if (buf == NULL) {
    fprintf(
        FDEST,
        "Error: prettyPrintOctopusRegisters called will null ptr to buf[]?\n");
    err = -1;  // who calls this with a null ptr to the bytes?  I mean who does
               // that?
  }

  // go through bytes in buf[] creating octopus_addr/data pairs, when once is
  // complete 'send' it to registers
  while ((byte_count < num_bytes) && (err == 0)) {
    switch (octopus_index) {
      case 0:
        octopus_addr = ((uint32_t)(buf[byte_count]));
        byte_count++;
        flag_send_reg = 0;
        octopus_index++;
        break;
      case 1:
        octopus_addr |= (((uint32_t)(buf[byte_count])) << 8);
        byte_count++;
        if ((octopus_addr & 0x8000) == 0) {  // read is '0'
          flag_send_reg = 1;  // We now have an addr, let's process the read
          // For now, I assume we are looking at only the commands being sent to
          // octopus, i.e. the byte array 'buf[]' does not have the response to
          // a read inlined into it. Future work might be to figure out how to
          // make this work for reads. Alternately, keep the 'octopus_addr' 2
          // bytes and call prettyPrintOctopusReg(addr,data) with the read data
          octopus_data = 0xffffffff;  // hence let's set this all 'high' for a
                                      // read as don't have data bytes
          octopus_index = 0;          // reset for next pair of bytes in buf[]
        } else {                      // write is '1'
          flag_send_reg = 0;
          octopus_index++;  // keep counting, need data bytes for write
        }
        break;
      case 2:
        octopus_data = ((uint32_t)(buf[byte_count]));
        byte_count++;
        flag_send_reg = 0;
        octopus_index++;
        break;
      case 3:
        octopus_data |= (((uint32_t)(buf[byte_count])) << 8);
        byte_count++;
        flag_send_reg =
            1;  // We now have an addr/data pair, let's process the write
        octopus_index = 0;  // reset for next pair
        break;              // end of octopus addr/data pair
      default:
        fprintf(FDEST,
                "Internal Error: prettyPrintOctopusRegisters() "
                "octopus_index=%d not 0:3 (this should never happen)!\n",
                octopus_index);
        err = -4;  // octopus_index should never see this state, unknown
                   // internal error in prettyPrintOctopusRegisters()
        break;
    }  // end of switch(octopus_index)

    if (flag_send_reg == 1) {
      // This function is broken out to allow for pretty-printing to be called
      // after a read
      err = prettyPrintOctopusReg(FDEST, octopus_addr, octopus_data);
      flag_send_reg =
          0;  // out of an abudance of caution, zero out flag when done
    }
  }  // ! while( (byte_count < num_bytes) && (err==0) )

  return err;
}

#ifdef __cplusplus
}
#endif
