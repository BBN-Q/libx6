#include <iostream>
#include <iomanip>
#include <vector>

#include "libx6adc.h"
#include "constants.h"
#include "X6_1000.h"

using namespace std;

// N-wide hex output with 0x
template <unsigned int N>
std::ostream& hexn(std::ostream& out) {
  return out << "0x" << std::hex << std::setw(N) << std::setfill('0');
}

int main ()
{
  cout << "BBN X6-1000 Test Executable" << endl;

  set_logging_level(8);

  unsigned numDevices;
  get_num_devices(&numDevices);

  cout << numDevices << " X6 device" << (numDevices > 1 ? "s": "")  << " found" << endl;

  if (numDevices < 1)
  	return 0;

  // char s[] = "stdout";
  // set_log(s);

  cout << "Attempting to initialize libaps" << endl;

  int rc;
  rc = connect_by_ID(0);

  cout << "connect(0) returned " << rc << endl;

  uint32_t firmwareVersion;
  read_firmware_version(0, &firmwareVersion);
  cout << "Firmware revision: " << hexn<8> << firmwareVersion << endl;

  float logicTemp;
  get_logic_temperature(0, 0, &logicTemp);
  cout << "current logic temperature method 1 = " << logicTemp << endl;
  get_logic_temperature(0, 1, &logicTemp);
  cout << "current logic temperature method 2 = " << logicTemp << endl;

  uint32_t regVal;
  read_register(0, 0x0800, 64, &regVal);
  cout << "ADC0 stream ID: " << hexn<8> << regVal << endl;
  read_register(0, 0x0800, 65, &regVal);
  cout << "ADC1 stream ID: " << hexn<8> << regVal << std::dec << endl;

  double sampleRate;
  get_sampleRate(0, &sampleRate);
  cout << "current PLL frequency = " << sampleRate/1e6 << " MHz" << endl;

  // cout << "Set sample rate to 100 MHz" << endl;

  // set_sampleRate(0,100e6);

  // cout << "current PLL frequency = " << get_sampleRate(0)/1e6 << " MHz" << endl;

  // cout << "Set sample rate back to 1000 MHz" << endl;

  // set_sampleRate(0,1e9);

  // cout << "current PLL frequency = " << get_sampleRate(0)/1e6 << " MHz" << endl;

  cout << "setting trigger source = EXTERNAL_TRIGGER" << endl;

  set_trigger_source(0, EXTERNAL_TRIGGER);

  TriggerSource triggerSource;
  get_trigger_source(0, &triggerSource);
  cout << "get trigger source returns " << ((triggerSource == SOFTWARE_TRIGGER) ? "SOFTWARE_TRIGGER" : "EXTERNAL_TRIGGER") << endl;

  cout << "Enabling physical channel 1" << endl;
  enable_stream(0, 1, 0, 0);
  enable_stream(0, 1, 1, 0);
  enable_stream(0, 1, 1, 1);
  enable_stream(0, 1, 2, 0);
  enable_stream(0, 1, 2, 1);
  enable_stream(0, 2, 0, 0);
  enable_stream(0, 2, 1, 0);
  enable_stream(0, 2, 1, 1);
  enable_stream(0, 2, 2, 0);
  enable_stream(0, 2, 2, 1);

  cout << "Writing kernel lengths" << endl;
  write_register(0, 0x2000, 24+1-1, 2);
  write_register(0, 0x2000, 24+2-1, 2);
  write_register(0, 0x2100, 24+1-1, 2);
  write_register(0, 0x2100, 24+2-1, 2);

  cout << "setting averager parameters to record 10 segments of 1024 samples" << endl;

  set_averager_settings(0, 2048, 9, 1, 2);

  cout << "Acquiring" << endl;

  acquire(0);

  cout << "Waiting for acquisition to complete" << endl;
  int success = wait_for_acquisition(0, 2);
  if (success == X6_OK) {
    cout << "Acquistion finished" << endl;
  } else if (success == X6_TIMEOUT) {
    cout << "Acquisition timed out" << endl;
  } else {
    cout << "Unknown error in wait_for_acquisition" << endl;
  }

  cout << "Transferring waveform ch1" << endl;
  vector<double> buffer;
  ChannelTuple channels[1] = {{1,0,0}};
  int bufsize;
  get_buffer_size(0, channels, 1, &bufsize);
  buffer.resize(bufsize);
  transfer_waveform(0, channels, 1, buffer.data(), buffer.size());

  cout << "Transferring variance raw ch1" << endl;
  transfer_variance(0, channels, 1, buffer.data(), buffer.size());

  cout << "Transfer correlation" << endl;
  ChannelTuple channels2[2] = {{1,1,1}, {2,1,1}};
  get_buffer_size(0, channels2, 2, &bufsize);
  buffer.resize(bufsize);
  transfer_waveform(0, channels2, 2, buffer.data(), buffer.size());

  cout << "Stopping" << endl;

  stop(0);

  rc = disconnect(0);

  cout << "disconnect(0) returned " << rc << endl;

  return 0;
}
