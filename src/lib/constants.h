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

//Command byte bits
static const int LSB_MASK = 0xFF;

// WishBone interface
static const std::vector<unsigned> BASE_DSP = {0x2000, 0x2100};

//Registers we read from
static const int WB_ADDR_VERSION  = 0x10; // UPDATE ME
static const int WB_OFFSET_VERSION  = 0x01; // UPDATE ME
static const int WB_ADDR_DIGITIZER_MODE = 0x11; //update me
static const int WB_OFFSET_DIGITIZER_MODE = 0x01; //update me

static const int WB_FRAME_SIZE_OFFSET    = 0x00;
static const int WB_STREAM_ENABLE_OFFSET = 0x0f;
static const int WB_PHASE_INC_OFFSET     = 0x10;
static const int WB_KERNEL_LENGTH_OFFSET = 0x18;
static const int WB_STREAM_ID_OFFSET     = 0x20;
static const int WB_KERNEL_ADDR_OFFSET   = 0x30;
static const int WB_KERNEL_DATA_OFFSET   = 0x31;
static const int WB_THRESHOLD_OFFSET     = 0x38;
static const int WB_RECORD_LENGTH_OFFSET = 0x3f;

//Expected version
static const int FIRMWARE_VERSION =  0x1;

//Readout filter parameters
static const int VIRTUAL_CH_RATIO = 4; // Number of virtual channels per physical channel
static const int DECIMATION_FACTOR = 16;

// Correlations
static const int MAX_N_BODY_CORRELATIONS = 3;

#endif /* CONSTANTS_H_ */
