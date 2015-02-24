/*
 * libaps.h
 *
 *  Created on: Jun 25, 2012
 *      Author: qlab
 */

#ifndef LIBX6ADC_H
#define LIBX6ADC_H

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#include <stdio.h>

#include "X6_1000.h"
#include "X6_errno.h"
#include "X6_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ChannelTuple {
	int a;
	int b;
	int c;
};

void init() __attribute__((constructor));
void cleanup() __attribute__((destructor));

EXPORT X6_STATUS connect_by_ID(int);
EXPORT X6_STATUS disconnect(int);
EXPORT X6_STATUS get_num_devices(unsigned *);
EXPORT int is_open(int);
EXPORT X6_STATUS read_firmware_version(int, uint32_t*);

EXPORT X6_STATUS set_reference(int, ReferenceSource);
EXPORT X6_STATUS get_reference(int, ReferenceSource*);

EXPORT X6_STATUS enable_stream(int, int, int, int);
EXPORT X6_STATUS disable_stream(int, int, int, int);

EXPORT X6_STATUS set_averager_settings(int, int, int, int, int);
EXPORT X6_STATUS set_nco_frequency(int, int, int, double);
EXPORT X6_STATUS set_threshold(int, int, int, double);
EXPORT X6_STATUS write_kernel(int, int, int, double *, unsigned);

EXPORT X6_STATUS acquire(int);
EXPORT X6_STATUS wait_for_acquisition(int, unsigned);
EXPORT X6_STATUS get_is_running(int, int*);
EXPORT X6_STATUS get_has_new_data(int, int*);
EXPORT X6_STATUS stop(int);
EXPORT X6_STATUS transfer_waveform(int, ChannelTuple *, unsigned, double *, unsigned);
EXPORT X6_STATUS transfer_variance(int, ChannelTuple *, unsigned, double *, unsigned);
EXPORT X6_STATUS get_buffer_size(int, ChannelTuple *, unsigned, int*);
EXPORT X6_STATUS get_variance_buffer_size(int, ChannelTuple *, unsigned, int*);

EXPORT X6_STATUS set_log(char *);
X6_STATUS update_log(FILE * pFile);
EXPORT X6_STATUS set_logging_level(int);

/* unused/unfinished methods */
EXPORT X6_STATUS initX6(int);
EXPORT X6_STATUS set_digitizer_mode(int, DigitizerMode);
EXPORT X6_STATUS get_digitizer_mode(int, DigitizerMode*);
EXPORT X6_STATUS set_trigger_source(int, TriggerSource);
EXPORT X6_STATUS get_trigger_source(int, TriggerSource*);
EXPORT X6_STATUS set_sampleRate(int, double);
EXPORT X6_STATUS get_sampleRate(int, double*);

/* debug methods */
EXPORT X6_STATUS read_register(int, int, int, uint32_t*);
EXPORT X6_STATUS write_register(int, int, int, int);

// II X6-1000M Test Interface
EXPORT X6_STATUS get_logic_temperature(int, int, float*);

#ifdef __cplusplus
}
#endif

#endif /* LIBX6ADC_H */
