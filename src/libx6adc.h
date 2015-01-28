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

#ifdef __cplusplus
extern "C" {
#endif

enum X6ErrorCode {
	X6_OK,
	X6_UNKNOWN_ERROR = -1,
	X6_BUFFER_OVERFLOW = -2,
	X6_NOT_IMPLEMENTED_ERROR = -3,
	X6_INVALID_CHANNEL = -4,
	X6_FILE_ERROR = -5,
	X6_INVALID_DEVICEID = -6,
	X6_TIMEOUT = -7
};

struct ChannelTuple {
	int a;
	int b;
	int c;
};

void init() __attribute__((constructor));
void cleanup() __attribute__((destructor));

EXPORT int connect_by_ID(int);
EXPORT int disconnect(int);
EXPORT unsigned get_num_devices();
EXPORT int is_open(int);

EXPORT int initX6(int);
EXPORT int read_firmware_version(int);

EXPORT int set_digitizer_mode(int, int);
EXPORT int get_digitizer_mode(int);

EXPORT int set_sampleRate(int, double);
EXPORT double get_sampleRate(int);

EXPORT int set_trigger_source(int, int);
EXPORT int get_trigger_source(int);

EXPORT int set_reference(int, int);
EXPORT int get_reference(int);

EXPORT int enable_stream(int, int, int, int);
EXPORT int disable_stream(int, int, int, int);

EXPORT int set_averager_settings(int, int, int, int, int);

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

/* debug methods */
EXPORT int read_register(int, int, int);
EXPORT int write_register(int, int, int, int);

// II X6-1000M Test Interface
EXPORT float get_logic_temperature(int,int);

#ifdef __cplusplus
}
#endif

#endif /* LIBX6ADC_H */
