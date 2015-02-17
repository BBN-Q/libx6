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

#include "X6_errno.h"

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

EXPORT int connect_by_ID(int);
EXPORT int disconnect(int);
EXPORT X6_STATUS get_num_devices(unsigned *);
EXPORT int is_open(int);
EXPORT int read_firmware_version(int);

EXPORT int set_reference(int, int);
EXPORT int get_reference(int);

EXPORT int enable_stream(int, int, int, int);
EXPORT int disable_stream(int, int, int, int);

EXPORT int set_averager_settings(int, int, int, int, int);
EXPORT int set_nco_frequency(int, int, int, double);
EXPORT int set_threshold(int, int, int, double);
EXPORT int write_kernel(int, int, int, double *, unsigned);

EXPORT int acquire(int);
EXPORT int wait_for_acquisition(int, int);
EXPORT int get_is_running(int);
EXPORT int get_has_new_data(int);
EXPORT int stop(int);
EXPORT int transfer_waveform(int, ChannelTuple *, unsigned, double *, unsigned);
EXPORT int transfer_variance(int, ChannelTuple *, unsigned, double *, unsigned);
EXPORT int get_buffer_size(int, ChannelTuple *, unsigned);
EXPORT int get_variance_buffer_size(int, ChannelTuple *, unsigned);

EXPORT int set_log(char *);
int update_log(FILE * pFile);
EXPORT int set_logging_level(int);

/* unused/unfinished methods */
EXPORT int initX6(int);
EXPORT int set_digitizer_mode(int, int);
EXPORT int get_digitizer_mode(int);
EXPORT int set_trigger_source(int, int);
EXPORT int get_trigger_source(int);
EXPORT int set_sampleRate(int, double);
EXPORT double get_sampleRate(int);

/* debug methods */
EXPORT unsigned read_register(int, int, int);
EXPORT int write_register(int, int, int, int);

// II X6-1000M Test Interface
EXPORT float get_logic_temperature(int,int);

#ifdef __cplusplus
}
#endif

#endif /* LIBX6ADC_H */
