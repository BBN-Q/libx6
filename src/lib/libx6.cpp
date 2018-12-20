// libx6.cpp
//
// Provides C shared library interface to BBN's custom firmware for the II X6-1000 card
//
// Original authors: Brian Donnovan, Colm Ryan and Blake Johnson
//
// Copyright 2013-2015 Raytheon BBN Technologies

#include <memory> //unique_ptr
#include <string>
using std::string;
#include <cstring>
#include <iostream>

#include "logger.h"

#include "libx6.h"
#include "X6_1000.h"
#include "version.hpp"

// globals
map<unsigned, std::unique_ptr<X6_1000>> X6s_;
unsigned numDevices_ = 0;

// stub class to open/close the logger file handle
class InitAndCleanUp {
public:
  InitAndCleanUp();
  ~InitAndCleanUp();
};

InitAndCleanUp::InitAndCleanUp() {
  //Open the logging file
  FILE* pFile = fopen("libx6.log", "a");
  Output2FILE::Stream() = pFile;
  FILE_LOG(logINFO) << "libx6 driver version: " << get_driver_version();
}

InitAndCleanUp::~InitAndCleanUp() {
  FILE_LOG(logINFO) << "Cleaning up libx6 before driver unloading." << endl;
  if (Output2FILE::Stream()) {
    fclose(Output2FILE::Stream());
  }
}

static InitAndCleanUp initandcleanup_;

const char* get_error_msg(X6_STATUS err) {
  if (errorsMsgs.count(err)) {
    return errorsMsgs[err].c_str();
  }
  else {
    return "No error message for this status number.";
  }
}

//Define a couple of templated wrapper functions to make library calls and catch thrown errors
//First one for void calls
template<typename F, typename... Args>
X6_STATUS x6_call(const unsigned deviceID, F func, Args... args){
  try{
    (X6s_.at(deviceID).get()->*func)(args...); // for some reason the compiler can't infer the correct dereference operator without the get function
    //Nothing thrown then assume OK
    return X6_OK;
  }
  catch (std::out_of_range& e) {
    if (X6s_.find(deviceID) == X6s_.end()) {
      return X6_UNCONNECTED;
    } else {
      return X6_UNKNOWN_ERROR;
    }
  }
  catch (X6_STATUS status) {
    return status;
  }
  catch (...) {
    return X6_UNKNOWN_ERROR;
  }
}

//and one for to store getter values in pointer passed to library
template<typename R, typename F, typename... Args>
X6_STATUS x6_getter(const unsigned deviceID, F func, R* resPtr, Args... args){
  try {
    *resPtr = (X6s_.at(deviceID).get()->*func)(args...);
    //Nothing thrown then assume OK
    return X6_OK;
  }
  catch (std::out_of_range& e) {
    if (X6s_.find(deviceID) == X6s_.end()) {
      return X6_UNCONNECTED;
    } else {
      return X6_UNKNOWN_ERROR;
    }
  }
  catch (X6_STATUS status) {
    return status;
  }
  catch (...) {
    return X6_UNKNOWN_ERROR;
  }
}

#ifdef __cplusplus
extern "C" {
#endif

void update_num_devices() {
  // TODO: figure out if this needs to be static
  static Innovative::X6_1000M x6;
  numDevices_ = static_cast<unsigned int>(x6.BoardCount());
  FILE_LOG(logINFO) << numDevices_ << " X6 device" << (numDevices_ > 1 ? "s" : "") << " found.";
}

X6_STATUS get_num_devices(unsigned* numDevices) {
  update_num_devices();
  *numDevices = numDevices_;
  return X6_OK;
}

X6_STATUS connect_x6(int deviceID) {
  if (deviceID >= static_cast<int>(numDevices_)) {
    update_num_devices();
    if (deviceID >= static_cast<int>(numDevices_)) {
      return X6_NO_DEVICE_FOUND;
    }
  }
  if (X6s_.find(deviceID) == X6s_.end()){
    X6s_[deviceID] = std::unique_ptr<X6_1000>(new X6_1000()); //turn-into make_unique when we can go to gcc 4.9
  }
  return x6_call(deviceID, &X6_1000::open, deviceID);
}

X6_STATUS disconnect_x6(int deviceID) {
  X6_STATUS status = x6_call(deviceID, &X6_1000::close);

  if (status == X6_OK){
    X6s_.erase(deviceID);
  }
  return status;
}

X6_STATUS initX6(int deviceID) {
  return x6_call(deviceID, &X6_1000::init);
}

X6_STATUS get_firmware_version(int deviceID, uint32_t* version, uint32_t* git_sha1, uint32_t* build_timestamp, char* version_string) {
  X6_STATUS status = X6_OK;
  if ( version != nullptr ) {
    status = x6_getter(deviceID, &X6_1000::get_firmware_version, version);
    if (status != X6_OK) { return status; }
  }
  if ( git_sha1 != nullptr ) {
    status = x6_getter(deviceID, &X6_1000::get_firmware_git_sha1, git_sha1);
    if (status != X6_OK) { return status; }
  }
  if ( build_timestamp != nullptr ) {
    status = x6_getter(deviceID, &X6_1000::get_firmware_build_timestamp, build_timestamp);
    if (status != X6_OK) { return status; }
  }
  if ( version_string != nullptr ) {
    uint32_t my_version;
    uint32_t my_git_sha1;
    uint32_t my_build_timestamp;
    if ( version == nullptr ) {
      status = x6_getter(deviceID, &X6_1000::get_firmware_version, &my_version);
      if (status != X6_OK) { return status; }
    } else {
      my_version = *version;
    }
    if ( git_sha1 == nullptr ) {
      status = x6_getter(deviceID, &X6_1000::get_firmware_git_sha1, &my_git_sha1);
      if (status != X6_OK) { return status; }
    } else {
      my_git_sha1 = *git_sha1;
    }
    if ( build_timestamp == nullptr ) {
      status = x6_getter(deviceID, &X6_1000::get_firmware_build_timestamp, &my_build_timestamp);
      if (status != X6_OK) { return status; }
    } else {
      my_build_timestamp = *build_timestamp;
    }
    //put together the version string
    unsigned tag_minor = my_version & 0xff;
    unsigned tag_major = (my_version >> 8) & 0xff;
    unsigned commits_since = (my_version >> 16) & 0xfff;
    bool is_dirty = ((my_version >> 28) & 0xf) == 0xd;
    std::ostringstream version_stream;
    version_stream << "v" << tag_major << "." << tag_minor;
    if ( commits_since > 0 ) {
      version_stream << "-" << commits_since << "-g" << std::hex << my_git_sha1 << std::dec;
    }
    if (is_dirty) {
      version_stream << "-dirty";
    }
    unsigned year = (my_build_timestamp >> 24) & 0xff;
    unsigned month = (my_build_timestamp >> 16) & 0xff;
    unsigned day = (my_build_timestamp >> 8) & 0xff;
    version_stream << " 20" << std::hex << year << "-" << month << "-" << day;
    const string tmp_string = version_stream.str();
    std::strcpy(version_string, tmp_string.c_str());	}
  return status;
}

X6_STATUS get_sampleRate(int deviceID, double* freq) {
  return x6_getter(deviceID, &X6_1000::get_pll_frequency, freq);
}

X6_STATUS set_trigger_source(int deviceID, X6_TRIGGER_SOURCE triggerSource) {
  return x6_call(deviceID, &X6_1000::set_trigger_source, triggerSource);
}

X6_STATUS get_trigger_source(int deviceID, X6_TRIGGER_SOURCE* triggerSource) {
  return x6_getter(deviceID, &X6_1000::get_trigger_source, triggerSource);
}

X6_STATUS set_reference_source(int deviceID, X6_REFERENCE_SOURCE src) {
  return x6_call(deviceID, &X6_1000::set_reference_source, src);
}

X6_STATUS get_reference_source(int deviceID, X6_REFERENCE_SOURCE* src) {
  return x6_getter(deviceID, &X6_1000::get_reference_source, src);
}

X6_STATUS set_digitizer_mode(int deviceID, X6_DIGITIZER_MODE mode) {
  return x6_call(deviceID, &X6_1000::set_digitizer_mode, mode);
}

X6_STATUS get_digitizer_mode(int deviceID, X6_DIGITIZER_MODE* mode) {
  return x6_getter(deviceID, &X6_1000::get_digitizer_mode, mode);
}

X6_STATUS set_input_channel_enable(int deviceID, unsigned chan, bool enable) {
  return x6_call(deviceID, &X6_1000::set_input_channel_enable, chan, enable);
}

X6_STATUS get_input_channel_enable(int deviceID, unsigned chan, bool* enable) {
  return x6_getter(deviceID, &X6_1000::get_input_channel_enable, enable, chan);
}

X6_STATUS set_output_channel_enable(int deviceID, unsigned chan, bool enable) {
  return x6_call(deviceID, &X6_1000::set_output_channel_enable, chan, enable);
}

X6_STATUS get_output_channel_enable(int deviceID, unsigned chan, bool* enable) {
  return x6_getter(deviceID, &X6_1000::get_output_channel_enable, enable, chan);
}

X6_STATUS get_number_of_integrators(int deviceID, int a, int* num) {
  return x6_getter(deviceID, &X6_1000::get_number_of_integrators, num, a);
}

X6_STATUS get_number_of_demodulators(int deviceID, int a, int* num) {
  return x6_getter(deviceID, &X6_1000::get_number_of_demodulators, num, a);
}

X6_STATUS set_state_vld_bitmask(int deviceID, int a, unsigned mask) {
  return x6_call(deviceID, &X6_1000::set_state_vld_bitmask, a, mask);
}

X6_STATUS get_state_vld_bitmask(int deviceID, int a, int* mask) {
  return x6_getter(deviceID, &X6_1000::get_state_vld_bitmask, mask, a);
}

X6_STATUS enable_stream(int deviceID, int a, int b, int c) {
  return x6_call(deviceID, &X6_1000::enable_stream, a, b, c);
}

X6_STATUS disable_stream(int deviceID, int a, int b, int c) {
  return x6_call(deviceID, &X6_1000::disable_stream, a, b, c);
}

X6_STATUS set_averager_settings(int deviceID, int recordLength, int numSegments, int waveforms, int roundRobins) {
  return x6_call(deviceID, &X6_1000::set_averager_settings, recordLength, numSegments, waveforms, roundRobins);
}

X6_STATUS set_nco_frequency(int deviceID, int a, int b, double freq) {
  return x6_call(deviceID, &X6_1000::set_nco_frequency, a, b, freq);
}

X6_STATUS get_nco_frequency(int deviceID, int a, int b, double* freq) {
  return x6_getter(deviceID, &X6_1000::get_nco_frequency, freq, a, b);
}

X6_STATUS set_threshold(int deviceID, int a, int c, double threshold) {
  return x6_call(deviceID, &X6_1000::set_threshold, a, c, threshold);
}

X6_STATUS get_threshold(int deviceID, int a, int c, double* threshold) {
  return x6_getter(deviceID, &X6_1000::get_threshold, threshold, a, c);
}

X6_STATUS set_threshold_invert(int deviceID, int a, int thresholder, bool invert) {
  return x6_call(deviceID, &X6_1000::set_threshold_invert, a, thresholder, invert);
}

X6_STATUS get_threshold_invert(int deviceID, int a, int c, bool* invert) {
  return x6_getter(deviceID, &X6_1000::get_threshold_invert, invert, a, c);
}

X6_STATUS set_threshold_input_sel(int deviceID, int a, int thresholder, bool correlated) {
  return x6_call(deviceID, &X6_1000::set_threshold_input_sel, a, thresholder, correlated);
}

X6_STATUS get_threshold_input_sel(int deviceID, int a, int thresholder, bool* correlated) {
  return x6_getter(deviceID, &X6_1000::get_threshold_input_sel, correlated, a, thresholder);
}

X6_STATUS write_kernel(int deviceID, unsigned a, unsigned b, unsigned c, double* kernel, unsigned length) {
  complex<double>* kernel_cmplx = reinterpret_cast<std::complex<double>*>(kernel);
  vector<complex<double>> vec(kernel_cmplx, kernel_cmplx + length);
  return x6_call(deviceID, &X6_1000::write_kernel, a, b, c, vec);
}

X6_STATUS read_kernel(int deviceID, unsigned a, unsigned b, unsigned c, unsigned addr, double* val) {
  std::complex<double>* tmpVal = reinterpret_cast<std::complex<double>*>(val);
  return x6_getter(deviceID, &X6_1000::read_kernel, tmpVal, a, b, c, addr);
}

X6_STATUS set_kernel_bias(int deviceID, unsigned a, unsigned b, unsigned c, double* val) {
  std::complex<double>* tmp_val = reinterpret_cast<std::complex<double>*>(val);
  return x6_call(deviceID, &X6_1000::set_kernel_bias, a, b, c, *tmp_val);
}

X6_STATUS get_kernel_bias(int deviceID, unsigned a, unsigned b, unsigned c, double* val) {
  std::complex<double>* tmp_val = reinterpret_cast<std::complex<double>*>(val);
  return x6_getter(deviceID, &X6_1000::get_kernel_bias, tmp_val, a, b, c);
}

X6_STATUS get_correlator_size(int deviceID, int a, uint32_t* val) {
  return x6_getter(deviceID, &X6_1000::get_correlator_size, val, a);
}

X6_STATUS write_correlator_matrix(int deviceID, unsigned a, double* matrix, unsigned length) {
  vector<double> vec(matrix, matrix + length);
  return x6_call(deviceID, &X6_1000::write_correlator_matrix, a, vec);
}

X6_STATUS read_correlator_matrix(int deviceID, int a, int addr, double* val) {
  return x6_getter(deviceID, &X6_1000::read_correlator_matrix, val, a, addr);
}

X6_STATUS set_correlator_input(int deviceID, int a, int input_num, int sel) {
  return x6_call(deviceID, &X6_1000::set_correlator_input, a, input_num, sel);
}

X6_STATUS get_correlator_input(int deviceID, int a, int addr, uint32_t* val) {
  return x6_getter(deviceID, &X6_1000::get_correlator_input, val, a, addr);
}

X6_STATUS acquire(int deviceID) {
  return x6_call(deviceID, &X6_1000::acquire);
}

X6_STATUS wait_for_acquisition(int deviceID, unsigned timeOut) {
  return x6_call(deviceID, &X6_1000::wait_for_acquisition, timeOut);
}

X6_STATUS get_is_running(int deviceID, int* isRunning) {
  return x6_getter(deviceID, &X6_1000::get_is_running, isRunning);
}

X6_STATUS get_num_new_records(int deviceID, unsigned* hasNewData) {
  return x6_getter(deviceID, &X6_1000::get_num_new_records, hasNewData);
}

X6_STATUS get_data_available(int deviceID, bool* dataAvailable) {
  return x6_getter(deviceID, &X6_1000::get_data_available, dataAvailable);
}

X6_STATUS stop(int deviceID) {
  return x6_call(deviceID, &X6_1000::stop);
}

X6_STATUS register_socket(int deviceID, ChannelTuple *channel, int32_t socket) {
  QDSPStream stream(channel->a, channel->b, channel->c);
  return x6_call(deviceID, &X6_1000::register_socket, stream, socket);
}

X6_STATUS transfer_stream(int deviceID, ChannelTuple *channelTuples, unsigned numChannels, double* buffer, unsigned bufferLength) {
  // when passed a single ChannelTuple, fills buffer with the corresponding waveform data
  // when passed multple ChannelTuples, fills buffer with the corresponding correlation data
  vector<QDSPStream> channels(numChannels);
  for (unsigned i = 0; i < numChannels; i++) {
    channels[i] = QDSPStream(channelTuples[i].a, channelTuples[i].b, channelTuples[i].c);
  }
  if (numChannels == 1) {
    return x6_call(deviceID, &X6_1000::transfer_stream, channels[0], buffer, bufferLength);
  } else {
    return x6_call(deviceID, &X6_1000::transfer_correlation, channels, buffer, bufferLength);
  }
}

X6_STATUS transfer_variance(int deviceID, ChannelTuple *channelTuples, unsigned numChannels, double* buffer, unsigned bufferLength) {
  vector<QDSPStream> streams(numChannels);
  for (unsigned i = 0; i < numChannels; i++) {
    streams[i] = QDSPStream(channelTuples[i].a, channelTuples[i].b, channelTuples[i].c);
  }
  if (numChannels == 1) {
    return x6_call(deviceID, &X6_1000::transfer_variance, streams[0], buffer, bufferLength);
  } else {
    return x6_call(deviceID, &X6_1000::transfer_correlation_variance, streams, buffer, bufferLength);
  }
}

X6_STATUS get_buffer_size(int deviceID, ChannelTuple *channelTuples, unsigned numChannels, unsigned* bufferSize) {
  vector<QDSPStream> streams(numChannels);
  for (unsigned i = 0; i < numChannels; i++) {
    streams[i] = QDSPStream(channelTuples[i].a, channelTuples[i].b, channelTuples[i].c);
  }
  return x6_getter(deviceID, &X6_1000::get_buffer_size, bufferSize, streams);
}

X6_STATUS get_record_length(int deviceID, ChannelTuple * channelTuple,  unsigned * len) {
  QDSPStream stream(channelTuple->a, channelTuple->b, channelTuple->c);
  return x6_getter(deviceID, &X6_1000::get_record_length, len, stream);
}

X6_STATUS get_variance_buffer_size(int deviceID, ChannelTuple *channelTuples, unsigned numChannels, int* bufferSize) {
  vector<QDSPStream> streams(numChannels);
  for (unsigned i = 0; i < numChannels; i++) {
    streams[i] = QDSPStream(channelTuples[i].a, channelTuples[i].b, channelTuples[i].c);
  }
  return x6_getter(deviceID, &X6_1000::get_variance_buffer_size, bufferSize, streams);
}

/* Pulse generator methods */
EXPORT X6_STATUS write_pulse_waveform(int deviceID, unsigned pg, double* wf, unsigned numPoints) {
  vector<double> wfVec(wf, wf+numPoints);
  return x6_call(deviceID, &X6_1000::write_pulse_waveform, pg, wfVec);
}

EXPORT X6_STATUS read_pulse_waveform(int deviceID, unsigned pg, unsigned addr, double* val) {
  return x6_getter(deviceID, &X6_1000::read_pulse_waveform, val, pg, addr);
}

//Expects a null-terminated character array
X6_STATUS set_log(char* fileNameArr) {
  string fileName(fileNameArr);
  if (fileName.compare("stdout") == 0){
    return update_log(stdout);
  }
  else if (fileName.compare("stderr") == 0){
    return update_log(stderr);
  }
  else{

    FILE* pFile = fopen(fileName.c_str(), "a");
    if (!pFile) {
      return X6_LOGFILE_ERROR;
    }

    return update_log(pFile);
  }
}

X6_STATUS update_log(FILE* pFile) {
  if (pFile) {
    //Close the current file
    if (Output2FILE::Stream()) fclose(Output2FILE::Stream());
    //Assign the new one
    Output2FILE::Stream() = pFile;
    return X6_OK;
  } else {
    return X6_LOGFILE_ERROR;
  }
}

X6_STATUS set_logging_level(int logLevel) {
  FILELog::ReportingLevel() = TLogLevel(logLevel);
  return X6_OK;
}

X6_STATUS read_register(int deviceID, uint32_t wbAddr, uint32_t offset, uint32_t* regValue) {
  return x6_getter(deviceID, &X6_1000::read_wishbone_register, regValue, wbAddr, offset);
}

X6_STATUS write_register(int deviceID, uint32_t wbAddr, uint32_t offset, uint32_t data) {
  return x6_call(deviceID, &X6_1000::write_wishbone_register, wbAddr, offset, data);
}

X6_STATUS get_logic_temperature(int deviceID, float* temp) {
  return x6_getter(deviceID, &X6_1000::get_logic_temperature, temp);
}

#ifdef __cplusplus
}
#endif
