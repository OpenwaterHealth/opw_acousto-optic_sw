// prettyPrintOctopusRegisters.c
// Ian O'Donnell [Openwater]
// Created 1/28/2020

#pragma once 

#ifdef __cplusplus
extern "C" {
#endif 

#include "stdint.h"

// Octopus helper function, matches 'sel' mux input [0:31] to name of signal
const char *OctopusSel2Name(uint32_t sel);

// Use this to parse DDS (AOM) SPI write or read bytes
// Returns 0 if ok.  Output to FP (unless FP==NULL then to stdout)
int32_t prettyPrintOctopusDDSSPI(FILE *FP, uint8_t inst_byte,
                                 uint8_t *data_bytes);

// Use this for 2-Byte octopus address/data or for pretty-printing read response
// Returns 0 if ok.  Output to FP (unless FP==NULL then to stdout)
int32_t prettyPrintOctopusReg(FILE *FP, uint32_t octopus_addr,
                              uint32_t octopus_data);

// Use this for an array of bytes (e.g. bytes sent down USB)
// Returns 0 if ok.  Output to FP (unless FP==NULL then to stdout)
int32_t prettyPrintOctopusRegisters(FILE *FP, uint8_t *buf, uint32_t num_bytes);

#ifdef __cplusplus
}
#endif


