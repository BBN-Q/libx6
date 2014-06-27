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

// initialize the library --contructor hook in header
void init() {
	//Open the logging file
	FILE* pFile = fopen("libaps.log", "a");
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

bool check_device_open(int deviceID){
	//First check that we have it in the X6s_ map
	if (X6s_.find(deviceID) == X6s_.end()){
		FILE_LOG(logERROR) << "Device " << deviceID << " not yet connected.";
		return false;
	}
	return true;
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

int initX6(int deviceID) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->init();
}

int read_firmware_version(int deviceID) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->read_firmware_version();
}

int set_digitizer_mode(int deviceID, int mode) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->set_digitizer_mode(DIGITIZER_MODE(mode));
}

int get_digitizer_mode(int deviceID) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return int(X6s_[deviceID]->get_digitizer_mode());
}

int set_sampleRate(int deviceID, double freq){
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->set_clock(X6_1000::INTERNAL_CLOCK, freq); //assume for now we'll use the internal clock
}

double get_sampleRate(int deviceID) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->get_pll_frequency();
}

int set_trigger_source(int deviceID, int triggerSource) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->set_trigger_source(X6_1000::TriggerSource(triggerSource));
}

int get_trigger_source(int deviceID) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return int(X6s_[deviceID]->get_trigger_source());
}

int set_reference(int deviceID, int referenceSource) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->set_reference(X6_1000::ReferenceSource(referenceSource));
}

int get_reference(int deviceID) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return int(X6s_[deviceID]->get_reference());
}

int enable_stream(int deviceID, int physChan, int demodChan) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->enable_stream(physChan, demodChan);
}

int disable_stream(int deviceID, int physChan, int demodChan) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->disable_stream(physChan, demodChan);
}

int set_averager_settings(int deviceID, int recordLength, int numSegments, int waveforms, int roundRobins) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->set_averager_settings(recordLength, numSegments, waveforms, roundRobins);
}

int acquire(int deviceID) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->acquire();
}

int wait_for_acquisition(int deviceID, int timeOut) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->wait_for_acquisition(timeOut);
}

int stop(int deviceID) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->stop();
}

int transfer_waveform(int deviceID, int channel, double *buffer, unsigned bufferLength) {
	if (!check_device_open(deviceID)) return X6_1000::DEVICE_NOT_CONNECTED;
	return X6s_[deviceID]->transfer_waveform(channel, buffer, bufferLength);
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

int read_register(int deviceID, int wbAddr, int offset){
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

