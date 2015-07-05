/*
 * constants.h
 *
 *  Created on: Jul 3, 2012
 *      Author: cryan
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

static const int MAX_NUM_DEVICES = 5;

//Some maximum sizes of things we can fit
static const int X6_READTIMEOUT = 1000;
static const int X6_WRITETIMEOUT = 500;

static const int MAX_LENGTH_RAW_STREAM = 4096;

//Command byte bits
static const int LSB_MASK = 0xFF;

// WishBone interface
static const std::vector<unsigned> BASE_DSP = {0x2000, 0x2100};

//Registers we read from
static const int WB_ADDR_VERSION  = 0x10; // UPDATE ME
static const int WB_OFFSET_VERSION  = 0x01; // UPDATE ME
static const int WB_ADDR_DIGITIZER_MODE = 0x11; //update me
static const int WB_OFFSET_DIGITIZER_MODE = 0x01; //update me

static const int WB_QDSP_TEST                   = 0x01;
static const int WB_QDSP_RECORD_LENGTH          = 0x02;
static const int WB_QDSP_STREAM_ENABLE          = 0x03;
static const int WB_QDSP_RAW_KERNEL_LENGTH      = 0x10;
static const int WB_QDSP_DEMOD_KERNEL_LENGTH    = 0x14;
static const int WB_QDSP_RAW_KERNEL_ADDR_DATA   = 0x20;
static const int WB_QDSP_DEMOD_KERNEL_ADDR_DATA = 0x20;
static const int WB_QDSP_THRESHOLD              = 0x40;
static const int WB_QDSP_PHASE_INC              = 0x44;

//pulse generator offsets
static const std::vector<uint32_t> BASE_PG = {0x2200, 0x2300};

//Expected version
static const int FIRMWARE_VERSION =  0x1;

//Readout filter parameters
static const int VIRTUAL_CH_RATIO = 4; // Number of virtual channels per physical channel
static const int DECIMATION_FACTOR = 32;

// Correlations
static const int MAX_N_BODY_CORRELATIONS = 3;

#endif /* CONSTANTS_H_ */
