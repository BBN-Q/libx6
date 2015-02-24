#include "headings.h"

#include <X6_1000M_Mb.h>
#include <VitaPacketStream_Mb.h>
#include <SoftwareTimer_Mb.h>
#include <Application/TriggerManager_App.h>
#include <HardwareRegister_Mb.h>
#include <BufferDatagrams_Mb.h> // for ShortDG

#ifndef X6_1000_H_
#define X6_1000_H_

using std::vector;
using std::string;



/**
 * X6_1000 Class: Provides interface to Innovative Illustrations X6_1000 card
 *
 * The expectation is that this class will support a custom FPGA image for digitzer usage.
 *
 * This interface utilizes the II [Malibu library](www.innovative-dsp.com/products.php?product=Malibu)
 */

class Accumulator;
class Correlator;
class Channel;

enum channel_t { PHYSICAL, DEMOD, RESULT };

class X6_1000
{
public:

	enum ErrorCodes {
    	SUCCESS = 0,
    	INVALID_DEVICEID = -1,
    	DEVICE_NOT_CONNECTED = -2,
    	MODULE_ERROR = -3,
    	NOT_IMPLEMENTED = -4,
    	FILE_ERROR = -5,
    	TIMEOUT = -6,
    	INVALID_FREQUENCY = -0x10,
    	INVALID_CHANNEL = -0x11,
    	INVALID_INTERVAL = -0x12,
    	INVALID_FRAMESIZE = -0x13
	};

	enum ClockSource {
		EXTERNAL_CLOCK = 0,   /**< External Input */
		INTERNAL_CLOCK        /**< Internal Generation */
	};

	enum ReferenceSource {
		EXTERNAL_REFERENCE = 0,   /**< External Input */
		INTERNAL_REFERENCE        /**< Internal Generation */
	};

	enum ExtSource {
		FRONT_PANEL = 0, /**< Front panel input */
		P16              /**< P16 input */
	};

	enum TriggerSource {
		SOFTWARE_TRIGGER = 0,    /**< Software generated trigger */
		EXTERNAL_TRIGGER         /**< External trigger */
	};

	X6_1000();
	~X6_1000();

	float get_logic_temperature();
	float get_logic_temperature_by_reg(); // second test method to get temp using WB register

	int read_firmware_version();

	/** Set reference source and frequency
	 *  \param ref EXTERNAL || INTERNAL
	 *  \param frequency Frequency in Hz
	 *  \returns SUCCESS || INVALID_FREQUENCY
	 */
	void set_reference(ReferenceSource ref = INTERNAL_REFERENCE, float frequency = 10e6);

	ReferenceSource get_reference();

	/** Set clock source and frequency
	 *  \param src EXTERNAL || INTERNAL
	 *  \param frequency Frequency in Hz
	 *  \param extSrc FRONT_PANEL || P16
	 *  \returns SUCCESS || INVALID_FREQUENCY
	 */
	void set_clock(ClockSource src = INTERNAL_CLOCK,
		                 float frequency = 1e9,
		                 ExtSource extSrc = FRONT_PANEL);

	/** Set up clock and trigger routes
	 * \returns SUCCESS
	 */
	ErrorCodes set_routes();

	/** Set Trigger source
	 *  \param trgSrc SOFTWARE_TRIGGER || EXTERNAL_TRIGGER
	 */
	ErrorCodes set_trigger_source(TriggerSource trgSrc = EXTERNAL_TRIGGER);
	TriggerSource get_trigger_source() const;

	ErrorCodes set_trigger_delay(float delay = 0.0);

	/** Set Decimation Factor (current for both Tx and Rx)
	 * \params enabled set to true to enable
	 * \params factor Decimaton factor
	 * \returns SUCCESS
	 */
	ErrorCodes set_decimation(bool enabled = false, int factor = 1);
	int get_decimation();

	ErrorCodes set_frame(int recordLength);
	ErrorCodes set_averager_settings(const int & recordLength, const int & numSegments, const int & waveforms,  const int & roundRobins);

	ErrorCodes enable_stream(unsigned, unsigned, unsigned);
	ErrorCodes disable_stream(unsigned, unsigned, unsigned);

	bool get_channel_enable(int channel);

	void set_digitizer_mode(const DIGITIZER_MODE &);
	DIGITIZER_MODE get_digitizer_mode() const;

	ErrorCodes set_nco_frequency(int, int, double);
	ErrorCodes set_threshold(int, int, double);
	ErrorCodes write_kernel(int, int, double *, size_t);

	/** retrieve PLL frequnecy
	 *  \returns Actual PLL frequnecy (in MHz) returned from board
	 */
	double get_pll_frequency();

	unsigned int get_num_channels();

	void open(int deviceID);
	void init();
	void close();

	void acquire();
	void wait_for_acquisition(unsigned);
	void stop();
	bool get_is_running();
	bool get_has_new_data();

	void transfer_waveform(Channel, double *, size_t);
	void transfer_variance(Channel, double *, size_t);
	void transfer_correlation(vector<Channel> &, double *, size_t);
	void transfer_correlation_variance(vector<Channel> &, double *, size_t);
	int get_buffer_size(vector<Channel> &);
	int get_variance_buffer_size(vector<Channel> &);

	ErrorCodes write_wishbone_register(uint32_t, uint32_t, uint32_t);
	uint32_t read_wishbone_register(uint32_t, uint32_t) const;

	ErrorCodes write_dsp_register(unsigned, uint32_t, uint32_t);
	uint32_t read_dsp_register(unsigned, uint32_t) const;

	const int BusmasterSize = 4; /**< Rx & Tx BusMaster size in MB */
	const int MHz = 1e6;         /**< Constant for converting MHz */
	const int Meg = 1024 * 1024;

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

	TriggerSource triggerSource_ = EXTERNAL_TRIGGER; /**< cached trigger source */

	map<uint16_t, Channel> activeChannels_;
	//vector to go from VMP ordering to SID's
	vector<int> physChans_;
	vector<int> virtChans_;
	vector<int> resultChans_;
	//Some auxiliary accumlator data
	map<uint16_t, Accumulator> accumulators_;
	map<vector<uint16_t>, Correlator> correlators_;

	// State Variables
	bool isOpen_;				  /**< cached flag indicaing board was openned */
	bool isRunning_;
	int prefillPacketCount_;
	unsigned recordLength_ = 0;
	unsigned numRecords_ = 1;
	unsigned numSegments_;
	unsigned waveforms_;
	unsigned roundRobins_;
	unsigned recordsTaken_;

	ErrorCodes set_active_channels();
	void set_dsp_stream_ids();
	void set_defaults();
	void log_card_info();
	bool check_done();

	void initialize_accumulators();
	void initialize_correlators();

	void setHandler(OpenWire::EventHandler<OpenWire::NotifyEvent> &event,
    				void (X6_1000:: *CallBackFunction)(OpenWire::NotifyEvent & Event));

	// Malibu Event handlers

	void HandleDisableTrigger(OpenWire::NotifyEvent & Event);
	void HandleExternalTrigger(OpenWire::NotifyEvent & Event);
	void HandleSoftwareTrigger(OpenWire::NotifyEvent & Event);

	void HandleBeforeStreamStart(OpenWire::NotifyEvent & Event);
	void HandleAfterStreamStart(OpenWire::NotifyEvent & Event);
	void HandleAfterStreamStop(OpenWire::NotifyEvent & Event);

	void HandleDataAvailable(Innovative::VitaPacketStreamDataEvent & Event);
	void VMPDataAvailable(Innovative::VeloMergeParserDataAvailable & Event, channel_t);
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

	// Module Alerts
	void HandleTimestampRolloverAlert(Innovative::AlertSignalEvent & event);
    void HandleSoftwareAlert(Innovative::AlertSignalEvent & event);
    void HandleWarningTempAlert(Innovative::AlertSignalEvent & event);
    void HandleInputFifoOverrunAlert(Innovative::AlertSignalEvent & event);
    void HandleInputOverrangeAlert(Innovative::AlertSignalEvent & event);
    void HandleTriggerAlert(Innovative::AlertSignalEvent & event);

    void LogHandler(string handlerName);
};

class Channel{
public:
	Channel();
	Channel(unsigned, unsigned, unsigned);

	unsigned channelID[3];
	uint16_t streamID;
	channel_t type;
};

class Accumulator{
friend X6_1000;

public:
	/* Helper class to accumulate/average data */
	Accumulator();
	Accumulator(const Channel &, const size_t &, const size_t &, const size_t &);
	template <class T>
	void accumulate(const Innovative::AccessDatagram<T> &);

	void reset();
	void snapshot(double *);
	void snapshot_variance(double *);
	size_t get_buffer_size();
	size_t get_variance_buffer_size();
	size_t calc_record_length(const Channel &, const size_t &);
	int fixed_to_float(const Channel &);

	size_t recordsTaken;

private:
	Channel channel_;
	size_t wfmCt_;
	size_t numSegments_;
	size_t numWaveforms_;
	size_t recordLength_;
	int fixed_to_float_;

	vector<int64_t> data_;
	vector<int64_t>::iterator idx_;
	// second data object to store the square of the data
	vector<int64_t> data2_;
	vector<int64_t>::iterator idx2_;
};

class Correlator {
public:
	Correlator();
	Correlator(const vector<Channel> &, const size_t &, const size_t &);
	template <class T>
	void accumulate(const int &, const Innovative::AccessDatagram<T> &);
	void correlate();

	void reset();
	void snapshot(double *);
	void snapshot_variance(double *);
	size_t get_buffer_size();
	size_t get_variance_buffer_size();

	size_t recordsTaken;

private:
	size_t wfmCt_;
	size_t numSegments_;
	size_t numWaveforms_;
	size_t recordLength_;
	int64_t fixed_to_float_;

	// buffers for raw data from the channels
	vector<vector<int>> buffers_;
	map<uint16_t, int> bufferSID_;

	// buffer for the correlated values A*B(*C*D*...)
	vector<double> data_;
	vector<double>::iterator idx_;
	// buffer for (A*B)^2
	vector<double> data2_;
	vector<double>::iterator idx2_;
};

vector<vector<int>> combinations(int, int);


#endif
