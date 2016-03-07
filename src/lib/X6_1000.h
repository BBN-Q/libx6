// X6_1000.h
//
// Provides interface to BBN's custom firmware for the II X6-1000 card
//
// Original authors: Brian Donnovan, Colm Ryan and Blake Johnson
//
// Copyright 2013-2015 Raytheon BBN Technologies

#ifndef X6_1000_H_
#define X6_1000_H_

#include <array>
#include <mutex>

#include "X6_enums.h"

#include "QDSPStream.h"
#include "Accumulator.h"
#include "Correlator.h"
#include "RecordQueue.h"

// II Malibu headers
#include <X6_1000M_Mb.h>
#include <VitaPacketStream_Mb.h>
#include <SoftwareTimer_Mb.h>
#include <Application/TriggerManager_App.h>
#include <HardwareRegister_Mb.h>
#include <BufferDatagrams_Mb.h> // for ShortDG

class X6_1000
{
public:

	X6_1000();
	~X6_1000();

	float get_logic_temperature();

	uint32_t get_firmware_version();
	uint32_t get_firmware_git_sha1();

	void set_reference_source(X6_REFERENCE_SOURCE ref = INTERNAL_REFERENCE);
	X6_REFERENCE_SOURCE get_reference_source();


	/** Set Trigger source
	 *  \param trgSrc SOFTWARE_TRIGGER || EXTERNAL_TRIGGER
	 */
	void set_trigger_source(X6_TRIGGER_SOURCE trgSrc = EXTERNAL_TRIGGER);
	X6_TRIGGER_SOURCE get_trigger_source() const;

	void set_digitizer_mode(const X6_DIGITIZER_MODE &);
	X6_DIGITIZER_MODE get_digitizer_mode() const;

	void set_trigger_delay(float delay = 0.0);

	/** Set Decimation Factor (current for both Tx and Rx)
	 * \params enabled set to true to enable
	 * \params factor Decimaton factor
	 * \returns SUCCESS
	 */
	void set_decimation(bool enabled = false, int factor = 1);
	int get_decimation();

	void set_averager_settings(const int & recordLength, const int & numSegments, const int & waveforms,  const int & roundRobins);
	void set_record_length(int recordLength);

	void enable_stream(unsigned, unsigned, unsigned);
	void disable_stream(unsigned, unsigned, unsigned);

	void set_input_channel_enable(unsigned, bool);
	bool get_input_channel_enable(unsigned);
	void set_output_channel_enable(unsigned, bool);
	bool get_output_channel_enable(unsigned);

	void set_nco_frequency(int, int, double);
	double get_nco_frequency(int, int);
	void set_threshold(int, int, double);
	double get_threshold(int, int);
	void write_kernel(int, int, int, const vector<complex<double>> &);
	complex<double> read_kernel(unsigned, unsigned, unsigned, unsigned);

	/** retrieve PLL frequnecy
	 *  \returns Actual PLL frequnecy (in MHz) returned from board
	 */
	double get_pll_frequency();

	void open(int deviceID);
	void init();
	void close();

	void acquire();
	void wait_for_acquisition(unsigned);
	void stop();
	bool get_is_running();
	size_t get_num_new_records();

	void transfer_stream(QDSPStream, double *, size_t);
	void transfer_variance(QDSPStream, double *, size_t);
	void transfer_correlation(vector<QDSPStream> &, double *, size_t);
	void transfer_correlation_variance(vector<QDSPStream> &, double *, size_t);
	int get_buffer_size(vector<QDSPStream> &);
	unsigned get_record_length(QDSPStream &);
	int get_variance_buffer_size(vector<QDSPStream> &);

	/* Pulse generator methods */
	void write_pulse_waveform(unsigned, vector<double>&);
	double read_pulse_waveform(unsigned, uint16_t);

	void write_wishbone_register(uint32_t, uint32_t, uint32_t);
	uint32_t read_wishbone_register(uint32_t, uint32_t) const;

	void write_dsp_register(unsigned, uint32_t, uint32_t);
	uint32_t read_dsp_register(unsigned, uint32_t) const;

	/**< Rx & Tx BusMaster size in MB */
	const int RxBusmasterSize = 32;
	const int TxBusmasterSize = 4;

private:
	// disable copying because some the innovative stuff it holds on to is non-copyable
	X6_1000(const X6_1000&) = delete;
	X6_1000& operator=(const X6_1000&) = delete;

	unsigned deviceID_;       /**< board ID (aka target number) */

	Innovative::X6_1000M            module_; /**< Malibu module */
	Innovative::TriggerManager      trigger_;   /**< Malibu trigger manager */
	Innovative::VitaPacketStream    stream_;
	Innovative::SoftwareTimer       timer_;
	Innovative::VeloBuffer       	outputPacket_;
	vector<Innovative::VeloMergeParser> VMPs_; /**< Utility to convert and filter Velo stream back into VITA packets*/

	X6_TRIGGER_SOURCE triggerSource_ = EXTERNAL_TRIGGER; /**< cached trigger source */
	X6_DIGITIZER_MODE digitizerMode_ = AVERAGER;

	map<uint16_t, QDSPStream> activeQDSPStreams_;

	//vector to go from VMP ordering to SID's
	vector<int> physChans_;
	vector<int> virtChans_;
	vector<int> resultChans_;
	//Some auxiliary accumlator data
	map<uint16_t, Accumulator> accumulators_;
	map<vector<uint16_t>, Correlator> correlators_;
	map<uint16_t, RecordQueue<int32_t>> queues_;
	// locks for reading/writing data to queues
	map<uint16_t, std::mutex> mutexes_;

	// State Variables
	bool isOpen_;				  /**< cached flag indicaing board was openned */
	bool isRunning_;
	bool needToInit_;
	int prefillPacketCount_;
	unsigned recordLength_ = 0;
	unsigned numRecords_ = 1;
	unsigned numSegments_;
	unsigned waveforms_;
	unsigned roundRobins_;
	unsigned recordsTaken_;

	std::array<bool, 2> activeInputChannels_;
	std::array<bool, 4> activeOutputChannels_;
	X6_REFERENCE_SOURCE refSource_;

	void set_active_channels();
	void log_card_info();
	bool check_done();

	void initialize_accumulators();
	void initialize_queues();
	void initialize_correlators();

	// Malibu Event handlers

	void HandleDisableTrigger(OpenWire::NotifyEvent & Event);
	void HandleExternalTrigger(OpenWire::NotifyEvent & Event);
	void HandleSoftwareTrigger(OpenWire::NotifyEvent & Event);

	void HandleBeforeStreamStart(OpenWire::NotifyEvent & Event);
	void HandleAfterStreamStart(OpenWire::NotifyEvent & Event);
	void HandleAfterStreamStop(OpenWire::NotifyEvent & Event);

	void HandleDataAvailable(Innovative::VitaPacketStreamDataEvent & Event);
	void VMPDataAvailable(Innovative::VeloMergeParserDataAvailable & Event, STREAM_T);
	void HandlePhysicalStream(Innovative::VeloMergeParserDataAvailable & Event) {
		VMPDataAvailable(Event, PHYSICAL);
	};
	void HandleVirtualStream(Innovative::VeloMergeParserDataAvailable & Event) {
		VMPDataAvailable(Event, DEMOD);
	};

	void HandleResultStream(Innovative::VeloMergeParserDataAvailable & Event) {
		VMPDataAvailable(Event, RESULT);
	};

	void HandleTimer(OpenWire::NotifyEvent & Event);
};

#endif
