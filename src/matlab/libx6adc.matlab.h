/*
 * libx6.h
 *
 */

#ifndef LIBX6ADC_H
#define LIBX6ADC_H

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
typedef enum ReferenceSource ReferenceSource;
typedef struct ChannelTuple ChannelTuple;
typedef enum TriggerSource TriggerSource;

EXPORT const char* get_error_msg(X6_STATUS);


EXPORT X6_STATUS connect_x6(int);
EXPORT X6_STATUS disconnect_x6(int);
EXPORT X6_STATUS get_num_devices(unsigned*);
EXPORT X6_STATUS read_firmware_version(int, unsigned*);

EXPORT X6_STATUS set_reference(int, ReferenceSource);
EXPORT X6_STATUS get_reference(int, ReferenceSource*);

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
EXPORT X6_STATUS get_has_new_data(int, int*);
EXPORT X6_STATUS stop(int);
EXPORT X6_STATUS transfer_waveform(int, ChannelTuple*, unsigned, double*, unsigned);
EXPORT X6_STATUS transfer_variance(int, ChannelTuple*, unsigned, double*, unsigned);
EXPORT X6_STATUS get_buffer_size(int, ChannelTuple*, unsigned, int*);
EXPORT X6_STATUS get_variance_buffer_size(int, ChannelTuple*, unsigned, int*);

EXPORT X6_STATUS set_log(char*);
EXPORT X6_STATUS set_logging_level(int);

/* unused/unfinished methods */
EXPORT X6_STATUS initX6(int);
EXPORT X6_STATUS set_trigger_source(int, TriggerSource);
EXPORT X6_STATUS get_trigger_source(int, TriggerSource*);
EXPORT X6_STATUS set_sampleRate(int, double);
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

#endif /* LIBX6ADC_H */
