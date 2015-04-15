/*
 * libx6adc.cpp
 *
 *  Created on: Oct 30, 2013
 *      Author: qlab
 */

#include "headings.h"
#include "libx6adc.h"
#include "X6_1000.h"

// globals
map<unsigned, std::unique_ptr<X6_1000>> X6s_;
unsigned numDevices_ = 0;

const char* get_error_msg(X6_STATUS err) {
	if (errorsMsgs.count(err)) {
		return errorsMsgs[err].c_str();
	}
	else {
		return "No error message for this status number.";
	}
}

// initialize the library --contructor hook in header
void init() {
	//Open the logging file
	FILE* pFile = fopen("libx6.log", "a");
	Output2FILE::Stream() = pFile;

	//TODO: figure out if this needs to be static
	static Innovative::X6_1000M x6;
	numDevices_ = static_cast<unsigned int>(x6.BoardCount());
	FILE_LOG(logINFO) << "Initializing BBN libx6 with " << numDevices_ << " device" << (numDevices_ > 1 ? "s" : "") << " found.";
}

//cleanup on driver unload --destructor hook in header
void cleanup(){
	FILE_LOG(logINFO) << "Cleaning up libx6 before driver unloading." << endl;
	if (Output2FILE::Stream()) {
		fclose(Output2FILE::Stream());
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
	catch (std::out_of_range e) {
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
	catch (std::out_of_range e) {
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

X6_STATUS get_num_devices(unsigned* numDevices) {
	*numDevices = numDevices_;
	return X6_OK;
}

X6_STATUS connect_x6(int deviceID) {
	if (deviceID >= numDevices_) return X6_NO_DEVICE_FOUND;
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

X6_STATUS read_firmware_version(int deviceID, uint32_t* version) {
	return x6_getter(deviceID, &X6_1000::read_firmware_version, version);
}

X6_STATUS set_digitizer_mode(int deviceID, DigitizerMode mode) {
	return x6_call(deviceID, &X6_1000::set_digitizer_mode, mode);
}

X6_STATUS get_digitizer_mode(int deviceID, DigitizerMode* digitizerMode) {
	return x6_getter(deviceID, &X6_1000::get_digitizer_mode, digitizerMode);
}

X6_STATUS set_sampleRate(int deviceID, double freq){
	//assume for now we'll use the internal clock
	//varadic pack must be last so pass default arguments here too
	return x6_call(deviceID, &X6_1000::set_clock, INTERNAL_CLOCK, freq, FRONT_PANEL);
}

X6_STATUS get_sampleRate(int deviceID, double* freq) {
	return x6_getter(deviceID, &X6_1000::get_pll_frequency, freq);
}

X6_STATUS set_trigger_source(int deviceID, TriggerSource triggerSource) {
	return x6_call(deviceID, &X6_1000::set_trigger_source, triggerSource);
}

X6_STATUS get_trigger_source(int deviceID, TriggerSource* triggerSource) {
	return x6_getter(deviceID, &X6_1000::get_trigger_source, triggerSource);
}

X6_STATUS set_reference(int deviceID, ReferenceSource src) {
	//varadic pack must be last so pass default argument here too
	return x6_call(deviceID, &X6_1000::set_reference, src, 10e6);
}

X6_STATUS get_reference(int deviceID, ReferenceSource* src) {
	return x6_getter(deviceID, &X6_1000::get_reference, src);
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

X6_STATUS set_threshold(int deviceID, int a, int b, double threshold) {
	return x6_call(deviceID, &X6_1000::set_threshold, a, b, threshold);
}

X6_STATUS write_kernel(int deviceID, int a, int b, double* kernel, unsigned length) {
	return x6_call(deviceID, &X6_1000::write_kernel, a, b, kernel, length);
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

X6_STATUS get_has_new_data(int deviceID, int* hasNewData) {
	return x6_getter(deviceID, &X6_1000::get_has_new_data, hasNewData);
}

X6_STATUS stop(int deviceID) {
	return x6_call(deviceID, &X6_1000::stop);
}

X6_STATUS transfer_waveform(int deviceID, ChannelTuple *channelTuples, unsigned numChannels, double* buffer, unsigned bufferLength) {
	// when passed a single ChannelTuple, fills buffer with the corresponding waveform data
	// when passed multple ChannelTuples, fills buffer with the corresponding correlation data
	vector<Channel> channels(numChannels);
	for (int i = 0; i < numChannels; i++) {
		channels[i] = Channel(channelTuples[i].a, channelTuples[i].b, channelTuples[i].c);
	}
	if (numChannels == 1) {
		return x6_call(deviceID, &X6_1000::transfer_waveform, channels[0], buffer, bufferLength);
	} else {
		return x6_call(deviceID, &X6_1000::transfer_correlation, channels, buffer, bufferLength);
	}
}

X6_STATUS transfer_variance(int deviceID, ChannelTuple *channelTuples, unsigned numChannels, double* buffer, unsigned bufferLength) {
	vector<Channel> channels(numChannels);
	for (int i = 0; i < numChannels; i++) {
		channels[i] = Channel(channelTuples[i].a, channelTuples[i].b, channelTuples[i].c);
	}
	if (numChannels == 1) {
		return x6_call(deviceID, &X6_1000::transfer_variance, channels[0], buffer, bufferLength);
	} else {
		return x6_call(deviceID, &X6_1000::transfer_correlation_variance, channels, buffer, bufferLength);
	}
}

X6_STATUS get_buffer_size(int deviceID, ChannelTuple *channelTuples, unsigned numChannels, int* bufferSize) {
	vector<Channel> channels(numChannels);
	for (int i = 0; i < numChannels; i++) {
		channels[i] = Channel(channelTuples[i].a, channelTuples[i].b, channelTuples[i].c);
	}
	return x6_getter(deviceID, &X6_1000::get_buffer_size, bufferSize, channels);
}

X6_STATUS get_variance_buffer_size(int deviceID, ChannelTuple *channelTuples, unsigned numChannels, int* bufferSize) {
	vector<Channel> channels(numChannels);
	for (int i = 0; i < numChannels; i++) {
		channels[i] = Channel(channelTuples[i].a, channelTuples[i].b, channelTuples[i].c);
	}
	return x6_getter(deviceID, &X6_1000::get_variance_buffer_size, bufferSize, channels);
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

X6_STATUS read_register(int deviceID, int wbAddr, int offset, uint32_t* regValue){
	return x6_getter(deviceID, &X6_1000::read_wishbone_register, regValue, wbAddr, offset);
}

X6_STATUS write_register(int deviceID, int wbAddr, int offset, int data){
	return x6_call(deviceID, &X6_1000::write_wishbone_register, wbAddr, offset, data);
}

X6_STATUS get_logic_temperature(int deviceID, int method, float* temp) {
	if (method == 0)
		return x6_getter(deviceID, &X6_1000::get_logic_temperature, temp);
	else
		return x6_getter(deviceID, &X6_1000::get_logic_temperature_by_reg, temp);
}

#ifdef __cplusplus
}
#endif
