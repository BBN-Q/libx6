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
  X6_INVALID_FRAMESIZE = -8,
  X6_RAW_STREAM_TOO_LONG = -9,
  X6_MODULE_ERROR = -10
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
{X6_INVALID_FRAMESIZE, "Invalid frame size, frame size must be a mulitple of 4."},
{X6_RAW_STREAM_TOO_LONG, "Maximum raw stream size is 4096 points.  Disable raw streams to take long records."},
{X6_MODULE_ERROR, "Failed to open X6 card using Malibu."}
};

#endif

#endif
