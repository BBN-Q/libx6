// X6_registers.h
//
// Register definitions for different firmware versions
//
// Original authors: Guilhem Ribeill
//
// Copyright 2013-2019 Raytheon BBN Technologies

#ifndef X6_REGISTERS_H_
#define X6_REGISTERS_H_ 

#include <cstdint>
#include "constants.h"

#define WB_QDSP_TEST                            0x01
#define WB_QDSP_RECORD_LENGTH                   0x02
#define WB_QDSP_STREAM_ENABLE                   0x03
#define WB_QDSP_MODULE_FIRMWARE_VERSION         0x04
#define WB_QDSP_MODULE_FIRMWARE_GIT_SHA1        0x05
#define WB_QDSP_MODULE_FIRMWARE_BUILD_TIMESTAMP	0x06
#define WB_QDSP_NUM_RAW_KI                      0x07
#define WB_QDSP_NUM_DEMOD                       0x08
#define WB_QDSP_STATE_VLD_MASK                  0x09
#define WB_QDSP_RAW_KERNEL_LENGTH               0x10

struct firmware_v10_tag firmware_v10 {};
struct firmware_v20_tag firmware_v20 {};

struct QDSP_registers {

	QDSP_registers(const firmware_v10_tag &);
	QDSP_registers(const int, const int, const firmware_v20_tag &);

	const uint32_t WB_QDSP_DEMOD_KERNEL_LENGTH;
	
	const uint32_t WB_QDSP_RAW_KERNEL_ADDR_DATA;
	const uint32_t WB_QDSP_DEMOD_KERNEL_ADDR_DATA;

	const uint32_t WB_QDSP_PHASE_INC;

	const uint32_t WB_QDSP_THRESHOLD;
	const uint32_t WB_QDSP_THRESHOLD_INVERT;
	const uint32_t WB_QDSP_THRESHOLD_INPUT_SEL;

	const uint32_t WB_QDSP_RAW_KERNEL_BIAS;
	const uint32_t WB_QDSP_DEMOD_KERNEL_BIAS;

	const uint32_t WB_QDSP_CORRELATOR_SIZE;
	const uint32_t WB_QDSP_CORRELATOR_M_ADDR;
	const uint32_t WB_QDSP_CORRELATOR_M_DATA;
	const uint32_t WB_QDSP_CORRELATOR_SEL;

};


#endif