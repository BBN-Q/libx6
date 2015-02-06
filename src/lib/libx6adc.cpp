/*
 * libx6adc.cpp
 *
 *  Created on: Oct 30, 2013
 *      Author: qlab
 */

#include "headings.h"
#include "X6_1000.h"
#include "libx6adc.h"

// globals
map<unsigned, std::unique_ptr<X6_1000>> X6s_;
unsigned numDevices_ = 0;

// initialize the library --contructor hook in header
void init() {
	//Open the logging file
	FILE* pFile = fopen("libx6.log", "a");
	Output2FILE::Stream() = pFile;

	numDevices_ = get_num_devices();
	FILE_LOG(logINFO) << "Initializing BBN libx6 with " << numDevices_ << " device" << (numDevices_ > 1 ? "s" : "") << " found.";
}

//cleanup on driver unload --destructor hook in header
void cleanup(){
	FILE_LOG(logINFO) << "Cleaning up libx6 before driver unloading." << endl;
	if (Output2FILE::Stream()) {
		fclose(Output2FILE::Stream());
	}
}

#ifdef __cplusplus
extern "C" {
#endif

unsigned get_num_devices() {
    static Innovative::X6_1000M x6;
    return static_cast<unsigned int>(x6.BoardCount());
}

int connect_by_ID(int deviceID) {
	if (deviceID >= numDevices_) return X6_1000::INVALID_DEVICEID;
	if (X6s_.find(deviceID) == X6s_.end()){
		X6s_[deviceID] = std::unique_ptr<X6_1000>(new X6_1000()); //turn-into make_unique when we can go to gcc 4.9
	}
	return X6s_[deviceID]->open(deviceID);
}

int disconnect(int deviceID) {
	if (deviceID >= numDevices_) return X6_1000::INVALID_DEVICEID;
	X6s_[deviceID]->close();
	X6s_.erase(deviceID);
}

int is_open(int deviceID){
	//First check that we have it in the X6s_ map
	if (X6s_.find(deviceID) == X6s_.end()){
		FILE_LOG(logERROR) << "Device " << deviceID << " not yet connected.";
		return 0;
	}
	return 1;
}

int initX6(int deviceID) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->init();
}

int read_firmware_version(int deviceID) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->read_firmware_version();
}

int set_digitizer_mode(int deviceID, int mode) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->set_digitizer_mode(DIGITIZER_MODE(mode));
}

int get_digitizer_mode(int deviceID) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return int(X6s_[deviceID]->get_digitizer_mode());
}

int set_sampleRate(int deviceID, double freq){
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->set_clock(X6_1000::INTERNAL_CLOCK, freq); //assume for now we'll use the internal clock
}

double get_sampleRate(int deviceID) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->get_pll_frequency();
}

int set_trigger_source(int deviceID, int triggerSource) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->set_trigger_source(X6_1000::TriggerSource(triggerSource));
}

int get_trigger_source(int deviceID) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return int(X6s_[deviceID]->get_trigger_source());
}

int set_reference(int deviceID, int referenceSource) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->set_reference(X6_1000::ReferenceSource(referenceSource));
}

int get_reference(int deviceID) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return int(X6s_[deviceID]->get_reference());
}

int enable_stream(int deviceID, int a, int b, int c) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->enable_stream(a, b, c);
}

int disable_stream(int deviceID, int a, int b, int c) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->disable_stream(a, b, c);
}

int set_averager_settings(int deviceID, int recordLength, int numSegments, int waveforms, int roundRobins) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->set_averager_settings(recordLength, numSegments, waveforms, roundRobins);
}

int acquire(int deviceID) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->acquire();
}

int wait_for_acquisition(int deviceID, int timeOut) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->wait_for_acquisition(timeOut);
}

int get_is_running(int deviceID) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->get_is_running();
}

int get_has_new_data(int deviceID) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->get_has_new_data();	
}

int stop(int deviceID) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->stop();
}

int transfer_waveform(int deviceID, ChannelTuple *channelTuples, unsigned numChannels, double *buffer, unsigned bufferLength) {
	// when passed a single ChannelTuple, fills buffer with the corresponding waveform data
	// when passed multple ChannelTuples, fills buffer with the corresponding correlation data
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	vector<Channel> channels(numChannels);
	for (int i = 0; i < numChannels; i++) {
		channels[i] = Channel(channelTuples[i].a, channelTuples[i].b, channelTuples[i].c);
	}
	if (numChannels == 1) {
		return X6s_[deviceID]->transfer_waveform(channels[0], buffer, bufferLength);
	} else {
		return X6s_[deviceID]->transfer_correlation(channels, buffer, bufferLength);
	}
}

int transfer_variance(int deviceID, ChannelTuple *channelTuples, unsigned numChannels, double *buffer, unsigned bufferLength) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	vector<Channel> channels(numChannels);
	for (int i = 0; i < numChannels; i++) {
		channels[i] = Channel(channelTuples[i].a, channelTuples[i].b, channelTuples[i].c);
	}
	if (numChannels == 1) {
		return X6s_[deviceID]->transfer_variance(channels[0], buffer, bufferLength);
	} else {
		return X6s_[deviceID]->transfer_correlation_variance(channels, buffer, bufferLength);
	}
}

int get_buffer_size(int deviceID, ChannelTuple *channelTuples, unsigned numChannels) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	vector<Channel> channels(numChannels);
	for (int i = 0; i < numChannels; i++) {
		channels[i] = Channel(channelTuples[i].a, channelTuples[i].b, channelTuples[i].c);
	}
	return X6s_[deviceID]->get_buffer_size(channels);
}

int get_variance_buffer_size(int deviceID, ChannelTuple *channelTuples, unsigned numChannels) {
	if (!is_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	vector<Channel> channels(numChannels);
	for (int i = 0; i < numChannels; i++) {
		channels[i] = Channel(channelTuples[i].a, channelTuples[i].b, channelTuples[i].c);
	}
	return X6s_[deviceID]->get_variance_buffer_size(channels);
}

//Expects a null-terminated character array
int set_log(char * fileNameArr) {
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
			return X6_1000::FILE_ERROR;
		}

		return update_log(pFile);
	}
}

int update_log(FILE * pFile) {
	if (pFile) {
		//Close the current file
		if (Output2FILE::Stream()) fclose(Output2FILE::Stream());
		//Assign the new one
		Output2FILE::Stream() = pFile;
		return X6_1000::SUCCESS;
	} else {
		return X6_1000::FILE_ERROR;
	}
}

int set_logging_level(int logLevel) {
	FILELog::ReportingLevel() = TLogLevel(logLevel);
	return 0;
}

unsigned read_register(int deviceID, int wbAddr, int offset){
	return X6s_[deviceID]->read_wishbone_register(wbAddr, offset);
}

int write_register(int deviceID, int wbAddr, int offset, int data){
	return X6s_[deviceID]->write_wishbone_register(wbAddr, offset, data);
}

float get_logic_temperature(int deviceID, int method) {
	if (method == 0)
		return X6s_[deviceID]->get_logic_temperature();
	else
		return X6s_[deviceID]->get_logic_temperature_by_reg();
}

#ifdef __cplusplus
}
#endif

