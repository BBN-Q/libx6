// X6_registers.h
//
// Register definitions for different firmware versions
//
// Original authors: Guilhem Ribeill
//
// Copyright 2013-2019 Raytheon BBN Technologies

#include "X6_registers.h"

QDSP_registers::QDSP_registers() : {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

QDSP_registers::QDSP_registers(const firmware_v10_tag &tag) {

	WB_QDSP_DEMOD_KERNEL_LENGTH			= 0x14;
	
	WB_QDSP_RAW_KERNEL_ADDR_DATA		= 0x20;
	WB_QDSP_DEMOD_KERNEL_ADDR_DATA		= 0x28;

	WB_QDSP_PHASE_INC					= 0x34;

	WB_QDSP_THRESHOLD 					= 0x30;
	WB_QDSP_THRESHOLD_INVERT			= 0x38;
	WB_QDSP_THRESHOLD_INPUT_SEL			= 0x00;

	WB_QDSP_RAW_KERNEL_BIAS  			= 0x40;
	WB_QDSP_DEMOD_KERNEL_BIAS			= 0x48;

	WB_QDSP_CORRELATOR_SIZE				= 0x00;
	WB_QDSP_CORRELATOR_M_ADDR			= 0x00;
	WB_QDSP_CORRELATOR_M_DATA			= 0x00;
	WB_QDSP_CORRELATOR_SEL 				= 0x00;

}

QDSP_registers::QDSP_registers(const int numRawKi, const int numDemod, const firmware_v20_tag &tag) {

	WB_QDSP_DEMOD_KERNEL_LENGTH     = (WB_QDSP_RAW_KERNEL_LENGTH + numRawKi);
	WB_QDSP_RAW_KERNEL_ADDR_DATA    = (WB_QDSP_DEMOD_KERNEL_LENGTH + numDemod);
	WB_QDSP_DEMOD_KERNEL_ADDR_DATA  = (WB_QDSP_RAW_KERNEL_ADDR_DATA + 2*numRawKi);
	WB_QDSP_THRESHOLD               = (WB_QDSP_DEMOD_KERNEL_ADDR_DATA + 2*numDemod);
	WB_QDSP_PHASE_INC               = (WB_QDSP_THRESHOLD + numRawKi);
	WB_QDSP_THRESHOLD_INVERT        = (WB_QDSP_PHASE_INC + numRawKi + numDemod);
	WB_QDSP_THRESHOLD_INPUT_SEL     = (WB_QDSP_THRESHOLD_INVERT + 1);
	WB_QDSP_RAW_KERNEL_BIAS         = (WB_QDSP_THRESHOLD_INPUT_SEL + 1);
	WB_QDSP_DEMOD_KERNEL_BIAS       = (WB_QDSP_RAW_KERNEL_BIAS + 2*numRawKi);
	WB_QDSP_CORRELATOR_SIZE         = (WB_QDSP_DEMOD_KERNEL_BIAS + 2*numDemod);
	WB_QDSP_CORRELATOR_M_ADDR       = (WB_QDSP_CORRELATOR_SIZE + 1);
	WB_QDSP_CORRELATOR_M_DATA       = (WB_QDSP_CORRELATOR_M_ADDR + 1);
	WB_QDSP_CORRELATOR_SEL          = (WB_QDSP_CORRELATOR_M_DATA + 1);

}