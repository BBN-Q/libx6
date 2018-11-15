/*
 * constants.h
 *
 *  Created on: Jul 3, 2012
 *      Author: cryan
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include <vector>

const int MAX_NUM_DEVICES = 5;

//Some maximum sizes of things we can fit
const int MAX_RECORD_LENGTH = 16384; // max packet size (for FIFOs) is 4096; raw streams are quarter rate
const int MIN_RECORD_LENGTH = 128; // max packet size (for FIFOs) is 4096; raw streams are quarter rate
const int RECORD_LENGTH_GRANULARITY = 128; //to ensure decimated demod streams work

const int KERNEL_FRAC_BITS = 15;
const double MAX_KERNEL_VALUE = 1 - 1.0/(1 << KERNEL_FRAC_BITS);
const double MIN_KERNEL_VALUE = -1.0;

const int MAX_RAW_KERNEL_LENGTH = 4096;
const int MAX_DEMOD_KERNEL_LENGTH = 512;

const int WF_FRAC_BITS = 15;
const double MAX_WF_VALUE = 1 - 1.0/(1 << WF_FRAC_BITS);
const double MIN_WF_VALUE = -1.0;

const int CORRELATOR_FRAC_BITS = 14;
const double MAX_CORRELATOR_VALUE = 1.0;
const double MIN_CORRELATOR_VALUE = -1.0;

// WishBone interface
const std::vector<unsigned> BASE_DSP = {0x2000, 0x2100};

//Registers we read from
#define WB_QDSP_TEST							0x01
#define WB_QDSP_RECORD_LENGTH					0x02
#define WB_QDSP_STREAM_ENABLE                   0x03
#define WB_QDSP_MODULE_FIRMWARE_VERSION			0x04
#define WB_QDSP_MODULE_FIRMWARE_GIT_SHA1		0x05
#define WB_QDSP_MODULE_FIRMWARE_BUILD_TIMESTAMP	0x06
#define WB_QDSP_NUM_RAW_KI						0x07
#define WB_QDSP_NUM_DEMOD						0x08
#define WB_QDSP_RAW_KERNEL_LENGTH				0x10
#define WB_QDSP_DEMOD_KERNEL_LENGTH(numRawKi,numDemod)    (WB_QDSP_RAW_KERNEL_LENGTH + numRawKi)
#define WB_QDSP_RAW_KERNEL_ADDR_DATA(numRawKi,numDemod)   (WB_QDSP_DEMOD_KERNEL_LENGTH(numRawKi,numDemod) + numDemod)
#define WB_QDSP_DEMOD_KERNEL_ADDR_DATA(numRawKi,numDemod) (WB_QDSP_RAW_KERNEL_ADDR_DATA(numRawKi,numDemod) + 2*numRawKi)
#define WB_QDSP_THRESHOLD(numRawKi,numDemod)              (WB_QDSP_DEMOD_KERNEL_ADDR_DATA(numRawKi,numDemod) + 2*numDemod)
#define WB_QDSP_PHASE_INC(numRawKi,numDemod)              (WB_QDSP_THRESHOLD(numRawKi,numDemod) + numRawKi)
#define WB_QDSP_THRESHOLD_INVERT(numRawKi,numDemod)       (WB_QDSP_PHASE_INC(numRawKi,numDemod) + numRawKi + numDemod)
#define WB_QDSP_RAW_KERNEL_BIAS(numRawKi,numDemod)        (WB_QDSP_THRESHOLD_INVERT(numRawKi,numDemod) + 1)
#define WB_QDSP_DEMOD_KERNEL_BIAS(numRawKi,numDemod)      (WB_QDSP_RAW_KERNEL_BIAS(numRawKi,numDemod) + 2*numRawKi)
#define WB_QDSP_CORRELATOR_SIZE(numRawKi,numDemod)        (WB_QDSP_DEMOD_KERNEL_BIAS(numRawKi,numDemod) + 2*numDemod)
#define WB_QDSP_CORRELATOR_M_ADDR(numRawKi,numDemod)      (WB_QDSP_CORRELATOR_SIZE(numRawKi,numDemod)+1)
#define WB_QDSP_CORRELATOR_M_DATA(numRawKi,numDemod)      (WB_QDSP_CORRELATOR_M_ADDR(numRawKi,numDemod)+1)
#define WB_QDSP_CORRELATOR_SEL(numRawKi,numDemod)         (WB_QDSP_CORRELATOR_M_DATA(numRawKi,numDemod)+1)

//pulse generator offsets
const std::vector<uint32_t> BASE_PG = {0x2200, 0x2300};

const int WB_PG_CONTROL   = 0x00;
const int WB_PG_MODULE_FIRMWARE_VERSION = 0x02;
const int WB_PG_WF_LENGTH = 0x08;
const int WB_PG_WF_ADDR   = 0x09;
const int WB_PG_WF_DATA   = 0x0A;

//Readout filter parameters
const int VIRTUAL_CH_RATIO = 4; // Number of virtual channels per physical channel
const int RAW_DECIMATION_FACTOR = 4;
const int DEMOD_DECIMATION_FACTOR = 32;

// Correlations
const int MAX_N_BODY_CORRELATIONS = 3;

#endif /* CONSTANTS_H_ */
