#ifndef X6_ERRNO_H_
#define X6_ERRNO_H_

enum X6_STATUS {
  X6_OK,
  X6_UNKNOWN_ERROR = -1,
  X6_NO_DEVICE_FOUND = -2,
  X6_UNCONNECTED = -3,
  X6_INVALID_FREQUENCY = -4,
  X6_TIMEOUT = -5,
  X6_INVALID_CHANNEL = -6,
  X6_LOGFILE_ERROR = -7,
  X6_INVALID_RECORD_LENGTH = -8,
  X6_MODULE_ERROR = -9,
  X6_INVALID_WF_LEN = -10,
  X6_WF_OUT_OF_RANGE = -11,
  X6_INVALID_KERNEL_STREAM = -12,
  X6_INVALID_KERNEL_LENGTH = -13,
  X6_KERNEL_OUT_OF_RANGE = -14,
  X6_MODE_ERROR = -15,
  X6_SOCKET_ERROR = -16,
  X6_FIRMWARE_INVALID = -17
};

#ifdef __cplusplus

#include <map>
#include <string>

static std::map<X6_STATUS, std::string> errorsMsgs = {
{X6_UNKNOWN_ERROR, "API call failed with unknown exeption. Sorry :-("},
{X6_NO_DEVICE_FOUND, "Unable to connect to requested X6 card.  Make sure it is in the computer."},
{X6_UNCONNECTED, "API call made on unconnected X6."},
{X6_INVALID_FREQUENCY, "Attempt to set invalid clock frequency on X6."},
{X6_TIMEOUT, "Insufficient number of records were taken before timeout was hit."},
{X6_INVALID_CHANNEL, "API call attempted on invalid channel tuple."},
{X6_LOGFILE_ERROR, "Failed to open log file."},
{X6_INVALID_RECORD_LENGTH, "Invalid record length: must be greater than 128 points; less than 16384 and a mulitple of 128."},
{X6_MODULE_ERROR, "Failed to open X6 card using Malibu."},
{X6_INVALID_WF_LEN, "Pulse generator waveform must be multiple of 4."},
{X6_WF_OUT_OF_RANGE, "Pulse generator waveform values must be between -1.0 and (1-1/2^15)."},
{X6_INVALID_KERNEL_STREAM, "Attempted to write kernel to non kernel (raw or demod.) stream."},
{X6_KERNEL_OUT_OF_RANGE, "Kernel values must be between -1.0 and (1-1/2^15)."},
{X6_MODE_ERROR, "Feature requested incompatible with digitizer mode."},
{X6_SOCKET_ERROR, "Error occured writing data to socket."},
{X6_FIRMWARE_INVALID, "The requested operation is not supported on this version of the X6 firmware."}
};

#endif

#endif
