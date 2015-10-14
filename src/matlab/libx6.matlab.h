// libx6.h
//
// Provides C shared library interface to BBN's custom firmware for the II X6-1000 card
//
// Original authors: Brian Donnovan, Colm Ryan and Blake Johnson
//
// Copyright 2013-2015 Raytheon BBN Technologies

#ifndef LIBX6_H
#define LIBX6_H

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#include <stdio.h>

#include "../lib/X6_errno.h"
#include "../lib/X6_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

//Add typedefs for the enums for C compatibility
typedef enum X6_STATUS X6_STATUS;
typedef enum X6_REFERENCE_SOURCE X6_REFERENCE_SOURCE;
typedef struct ChannelTuple ChannelTuple;
typedef enum X6_TRIGGER_SOURCE X6_TRIGGER_SOURCE;
typedef enum X6_DIGITIZER_MODE X6_DIGITIZER_MODE;
typedef enum X6_MODULE_FIRMWARE_VERSION X6_MODULE_FIRMWARE_VERSION;

EXPORT const char* get_error_msg(X6_STATUS);


EXPORT X6_STATUS connect_x6(int);
EXPORT X6_STATUS disconnect_x6(int);
EXPORT X6_STATUS get_num_devices(unsigned*);
EXPORT X6_STATUS get_firmware_version(int, X6_MODULE_FIRMWARE_VERSION, unsigned short*);

EXPORT X6_STATUS set_reference_source(int, X6_REFERENCE_SOURCE);
EXPORT X6_STATUS get_reference_source(int, X6_REFERENCE_SOURCE*);

EXPORT X6_STATUS set_digitizer_mode(int, X6_DIGITIZER_MODE);
EXPORT X6_STATUS get_digitizer_mode(int, X6_DIGITIZER_MODE*);

EXPORT X6_STATUS set_input_channel_enable(int, unsigned, int);
EXPORT X6_STATUS get_input_channel_enable(int, unsigned, int*);
EXPORT X6_STATUS set_output_channel_enable(int, unsigned, int);
EXPORT X6_STATUS get_output_channel_enable(int, unsigned, int*);

EXPORT X6_STATUS enable_stream(int, int, int, int);
EXPORT X6_STATUS disable_stream(int, int, int, int);

EXPORT X6_STATUS set_nco_frequency(int, int, int, double);
EXPORT X6_STATUS get_nco_frequency(int, int, int, double*);

EXPORT X6_STATUS set_averager_settings(int, int, int, int, int);
EXPORT X6_STATUS set_threshold(int, int, int, double);
EXPORT X6_STATUS get_threshold(int, int, int, double*);
EXPORT X6_STATUS write_kernel(int, int, int, int, double *, unsigned);
EXPORT X6_STATUS read_kernel(int, unsigned, unsigned, unsigned, unsigned, double *);

EXPORT X6_STATUS acquire(int);
EXPORT X6_STATUS wait_for_acquisition(int, unsigned);
EXPORT X6_STATUS get_is_running(int, int*);
EXPORT X6_STATUS get_num_new_records(int, unsigned*);
EXPORT X6_STATUS stop(int);
EXPORT X6_STATUS transfer_waveform(int, ChannelTuple*, unsigned, double*, unsigned);
EXPORT X6_STATUS transfer_variance(int, ChannelTuple*, unsigned, double*, unsigned);
EXPORT X6_STATUS get_buffer_size(int, ChannelTuple*, unsigned, int*);
EXPORT X6_STATUS get_variance_buffer_size(int, ChannelTuple*, unsigned, int*);

EXPORT X6_STATUS set_log(char*);
EXPORT X6_STATUS set_logging_level(int);

/* unused/unfinished methods */
EXPORT X6_STATUS initX6(int);
EXPORT X6_STATUS set_trigger_source(int, X6_TRIGGER_SOURCE);
EXPORT X6_STATUS get_trigger_source(int, X6_TRIGGER_SOURCE*);
EXPORT X6_STATUS get_sampleRate(int, double*);

/* Pulse generator methods */
EXPORT X6_STATUS write_pulse_waveform(int, unsigned, double*, unsigned);
EXPORT X6_STATUS read_pulse_waveform(int, unsigned, unsigned, double*);

/* debug methods */
EXPORT X6_STATUS read_register(int, unsigned, unsigned, unsigned*);
EXPORT X6_STATUS write_register(int, unsigned, unsigned, unsigned);

// II X6-1000M Test Interface
EXPORT X6_STATUS get_logic_temperature(int, float*);

#ifdef __cplusplus
}
#endif

#endif /* LIBX6_H */
