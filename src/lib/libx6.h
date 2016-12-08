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
#include <complex.h>
#undef I //used in Malibu code
#include <stdbool.h>

#include "X6_errno.h"
#include "X6_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

//Add typedefs for the enums for C compatibility
typedef enum X6_STATUS X6_STATUS;
typedef enum X6_REFERENCE_SOURCE X6_REFERENCE_SOURCE;
typedef struct ChannelTuple ChannelTuple;
typedef enum X6_TRIGGER_SOURCE X6_TRIGGER_SOURCE;
typedef enum X6_DIGITIZER_MODE X6_DIGITIZER_MODE;

EXPORT const char* get_error_msg(X6_STATUS);

void init() __attribute__((constructor));
void cleanup() __attribute__((destructor));

EXPORT X6_STATUS connect_x6(int);
EXPORT X6_STATUS disconnect_x6(int);
EXPORT X6_STATUS get_num_devices(unsigned*);
EXPORT X6_STATUS get_firmware_version(int, uint32_t*, uint32_t*, uint32_t*, char*);

EXPORT X6_STATUS set_reference_source(int, X6_REFERENCE_SOURCE);
EXPORT X6_STATUS get_reference_source(int, X6_REFERENCE_SOURCE*);

EXPORT X6_STATUS set_digitizer_mode(int, X6_DIGITIZER_MODE);
EXPORT X6_STATUS get_digitizer_mode(int, X6_DIGITIZER_MODE*);

EXPORT X6_STATUS set_input_channel_enable(int, unsigned, bool);
EXPORT X6_STATUS get_input_channel_enable(int, unsigned, bool*);
EXPORT X6_STATUS set_output_channel_enable(int, unsigned, bool);
EXPORT X6_STATUS get_output_channel_enable(int, unsigned, bool*);

EXPORT X6_STATUS enable_stream(int, int, int, int);
EXPORT X6_STATUS disable_stream(int, int, int, int);

EXPORT X6_STATUS set_nco_frequency(int, int, int, double);
EXPORT X6_STATUS get_nco_frequency(int, int, int, double*);

EXPORT X6_STATUS set_averager_settings(int, int, int, int, int);
EXPORT X6_STATUS set_threshold(int, int, int, double);
EXPORT X6_STATUS get_threshold(int, int, int, double*);
EXPORT X6_STATUS set_threshold_invert(int, int, int, bool);
EXPORT X6_STATUS get_threshold_invert(int, int, int, bool*);

EXPORT X6_STATUS write_kernel(int, unsigned, unsigned, unsigned, double _Complex*, unsigned);
EXPORT X6_STATUS read_kernel(int, unsigned, unsigned, unsigned, unsigned, double _Complex*);
//use pointer in `set_kernel_bias` for compatibility with Matlab because it doesn't support C99 _Complex
EXPORT X6_STATUS set_kernel_bias(int, unsigned, unsigned, unsigned, double _Complex*);
EXPORT X6_STATUS get_kernel_bias(int, unsigned, unsigned, unsigned, double _Complex*);

EXPORT X6_STATUS acquire(int);
EXPORT X6_STATUS wait_for_acquisition(int, unsigned);
EXPORT X6_STATUS get_is_running(int, int*);
EXPORT X6_STATUS get_num_new_records(int, unsigned*);
EXPORT X6_STATUS get_data_available(int, bool*);
EXPORT X6_STATUS stop(int);
EXPORT X6_STATUS transfer_stream(int, ChannelTuple*, unsigned, double*, unsigned);
EXPORT X6_STATUS transfer_variance(int, ChannelTuple*, unsigned, double*, unsigned);
EXPORT X6_STATUS get_buffer_size(int, ChannelTuple*, unsigned, unsigned*);
EXPORT X6_STATUS get_record_length(int, ChannelTuple*, unsigned*);
EXPORT X6_STATUS get_variance_buffer_size(int, ChannelTuple*, unsigned, int*);

EXPORT X6_STATUS set_log(char*);
X6_STATUS update_log(FILE*);
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
EXPORT X6_STATUS read_register(int, uint32_t, uint32_t, uint32_t*);
EXPORT X6_STATUS write_register(int, uint32_t, uint32_t, uint32_t);

// II X6-1000M Test Interface
EXPORT X6_STATUS get_logic_temperature(int, float*);

#ifdef __cplusplus
}
#endif

#endif /* LIBX6_H */
