#include "X6_1000.h"

#include <IppMemoryUtils_Mb.h>  // for Init::UsePerformanceMemoryFunctions
#include <BufferDatagrams_Mb.h> // for ShortDG
#include <algorithm>            // std::max
#include <cstdlib>              // for rand

using namespace Innovative;

// constructor
X6_1000::X6_1000() :
    isOpen_{false}, isRunning_{false} {

    // Use IPP performance memory functions.    
    Init::UsePerformanceMemoryFunctions();
}

X6_1000::~X6_1000() {
	if (isOpen_) close();   
}

unsigned int X6_1000::get_num_channels() {
    return module_.Input().Channels();
}

void X6_1000::setHandler(OpenWire::EventHandler<OpenWire::NotifyEvent> & event, 
    void (X6_1000:: *CallBackFunction)(OpenWire::NotifyEvent & Event)) {

    event.SetEvent(this, CallBackFunction );
    event.Unsynchronize();
}

X6_1000::ErrorCodes X6_1000::open(int deviceID) {
    /* Connects to the II module with the given device ID returns MODULE_ERROR
     * if the device cannot be found
     */

    if (isOpen_) return SUCCESS;
    deviceID_ = deviceID;

    // Timer event handlers
    setHandler(timer_.OnElapsed, &X6_1000::HandleTimer);

    // Trigger event handlers
    trigger_.OnDisableTrigger.SetEvent(this, &X6_1000::HandleDisableTrigger);
    trigger_.OnExternalTrigger.SetEvent(this, &X6_1000::HandleExternalTrigger);
    trigger_.OnSoftwareTrigger.SetEvent(this, &X6_1000::HandleSoftwareTrigger);

    // Module event handlers
    setHandler(module_.OnBeforeStreamStart, &X6_1000::HandleBeforeStreamStart);
    setHandler(module_.OnAfterStreamStart, &X6_1000::HandleAfterStreamStart);
    setHandler(module_.OnAfterStreamStop, &X6_1000::HandleAfterStreamStop);

    // General alerts
    module_.Alerts().OnSoftwareAlert.SetEvent(      this, &X6_1000::HandleSoftwareAlert);
    module_.Alerts().OnWarningTemperature.SetEvent( this, &X6_1000::HandleWarningTempAlert);
    module_.Alerts().OnTrigger.SetEvent(            this, &X6_1000::HandleTriggerAlert);
    // Input alerts
    module_.Alerts().OnInputOverflow.SetEvent(      this, &X6_1000::HandleInputFifoOverrunAlert);
    module_.Alerts().OnInputOverrange.SetEvent(     this, &X6_1000::HandleInputOverrangeAlert);

    // Stream Event Handlers
    stream_.DirectDataMode(false);
    stream_.OnVeloDataAvailable.SetEvent(this, &X6_1000::HandleDataAvailable);

    stream_.RxLoadBalancing(false);
    stream_.TxLoadBalancing(false);


    // Insure BM size is a multiple of four MB
    const int RxBmSize = std::max(BusmasterSize/4, 1) * 4;
    const int TxBmSize = std::max(BusmasterSize/4, 1) * 4;
    module_.IncomingBusMasterSize(RxBmSize * Meg);
    module_.OutgoingBusMasterSize(TxBmSize * Meg);
    module_.Target(deviceID);

    try {
        module_.Open();
        FILE_LOG(logINFO) << "Opened Device " << deviceID;
        FILE_LOG(logINFO) << "Bus master size: Input => " << RxBmSize << " MB" << " Output => " << TxBmSize << " MB";
    }
    catch(...) {
        FILE_LOG(logINFO) << "Module Device Open Failure!";
        return MODULE_ERROR;
    }
        
    module_.Reset();
    FILE_LOG(logINFO) << "Module Device Opened Successfully...";
    
    isOpen_ = true;

    log_card_info();

    set_defaults();
    
    //  Connect Stream
    stream_.ConnectTo(&module_);
    FILE_LOG(logINFO) << "Stream Connected...";

    prefillPacketCount_ = stream_.PrefillPacketCount();
    FILE_LOG(logDEBUG) << "Stream prefill packet count: " << prefillPacketCount_;

    return SUCCESS;
  }

X6_1000::ErrorCodes X6_1000::init() {
    /*
    TODO: some standard setup stuff here.  Maybe move some of the open code here.
    */
    return SUCCESS;
}
 
X6_1000::ErrorCodes X6_1000::close() {
    stream_.Disconnect();
    module_.Close();

    isOpen_ = false;
    FILE_LOG(logINFO) << "Closed connection to device " << deviceID_;
	return SUCCESS;
}

int X6_1000::read_firmware_version() {
    int version = module_.Info().FpgaLogicVersion();
    int subrevision = module_.Info().FpgaLogicSubrevision();
    FILE_LOG(logINFO) << "Logic version: " << myhex << version << ", " << myhex << subrevision;
    return version;
}

float X6_1000::get_logic_temperature() {
    return static_cast<float>(module_.Thermal().LogicTemperature());
}

float X6_1000::get_logic_temperature_by_reg() {
    Innovative::AddressingSpace & logicMemory = Innovative::LogicMemorySpace(module_);
    const unsigned int wbTemp_offset = 0x200;    
    const unsigned int tempControl_offset = 0;
    Innovative::WishboneBusSpace wbs = Innovative::WishboneBusSpace(logicMemory, wbTemp_offset);
    Innovative::Register reg = Innovative::Register(wbs, tempControl_offset );
    Innovative::RegisterBitGroup Temperature = Innovative::RegisterBitGroup(reg, 8, 8);

    return static_cast<float>(Temperature.Value());
}

X6_1000::ErrorCodes X6_1000::set_routes() {
    // Route external clock source from front panel (other option is cslP16)
    module_.Clock().ExternalClkSelect(IX6ClockIo::cslFrontPanel);

    // route external sync source from front panel (other option is essP16)
    module_.Output().Trigger().ExternalSyncSource( IX6IoDevice::essFrontPanel );
    module_.Input().Trigger().ExternalSyncSource( IX6IoDevice::essFrontPanel );

    return SUCCESS;
}

X6_1000::ErrorCodes X6_1000::set_reference(X6_1000::ReferenceSource ref, float frequency) {
    IX6ClockIo::IIReferenceSource x6ref; // reference source
    if (frequency < 0) return INVALID_FREQUENCY;

    x6ref = (ref == EXTERNAL_REFERENCE) ? IX6ClockIo::rsExternal : IX6ClockIo::rsInternal;
    FILE_LOG(logDEBUG1) << "Setting reference frequency to " << frequency;

    module_.Clock().Reference(x6ref);
    module_.Clock().ReferenceFrequency(frequency);
    return SUCCESS;
}

X6_1000::ReferenceSource X6_1000::get_reference() {
    auto iiref = module_.Clock().Reference();
    return (iiref == IX6ClockIo::rsExternal) ? EXTERNAL_REFERENCE : INTERNAL_REFERENCE;
}

X6_1000::ErrorCodes X6_1000::set_clock(X6_1000::ClockSource src , float frequency, ExtSource extSrc) {

    IX6ClockIo::IIClockSource x6clksrc; // clock source
    if (frequency < 0) return INVALID_FREQUENCY;

    FILE_LOG(logDEBUG1) << "Setting clock frequency to " << frequency;
    // Route clock
    x6clksrc = (src ==  EXTERNAL_CLOCK) ? IX6ClockIo::csExternal : IX6ClockIo::csInternal;
    module_.Clock().Source(x6clksrc);
    module_.Clock().Frequency(frequency);

    return SUCCESS;
}

double X6_1000::get_pll_frequency() {
    double freq = module_.Clock().FrequencyActual();
    FILE_LOG(logINFO) << "PLL frequency for X6: " << freq;
    return freq;

}

X6_1000::ErrorCodes X6_1000::set_trigger_source(TriggerSource trgSrc) {
    // cache trigger source
    triggerSource_ = trgSrc;

    FILE_LOG(logINFO) << "Trigger Source set to " << ((trgSrc == EXTERNAL_TRIGGER) ? "External" : "Internal");

    trigger_.ExternalTrigger( (trgSrc == EXTERNAL_TRIGGER) ? true : false);
    trigger_.AtConfigure();

    return SUCCESS;
}

X6_1000::TriggerSource X6_1000::get_trigger_source() const {
    // return cached trigger source until 
    // TODO: identify method for getting source from card
    if (triggerSource_) 
        return EXTERNAL_TRIGGER;
    else
        return SOFTWARE_TRIGGER;
}

X6_1000::ErrorCodes X6_1000::set_trigger_delay(float delay) {
    // going to require a trigger engine modification to work
    // leaving as a TODO for now
    // Something like this might work:
    // trigger_.DelayedTriggerPeriod(delay);
    return SUCCESS;
}

X6_1000::ErrorCodes X6_1000::set_decimation(bool enabled, int factor) {
    module_.Input().Decimation((enabled ) ? factor : 0);
    return SUCCESS;
}

int X6_1000::get_decimation() {
    int decimation = module_.Input().Decimation();
    return (decimation > 0) ? decimation : 1;
}

X6_1000::ErrorCodes X6_1000::set_frame(int recordLength) {
    FILE_LOG(logINFO) << "Setting recordLength_ = " << recordLength;

    recordLength_ = recordLength;

    // setup the trigger window size
    int frameGranularity = module_.Input().Info().TriggerFrameGranularity();
    if (recordLength % frameGranularity != 0) {
        FILE_LOG(logERROR) << "Invalid frame size: " << recordLength;
        return INVALID_FRAMESIZE;
    }
    module_.Input().Trigger().FramedMode(true);
    module_.Input().Trigger().Edge(true);
    module_.Input().Trigger().FrameSize(recordLength);

    // (some of?) the following seems to be necessary to get external triggering to work
    module_.Input().Pulse().Reset();
    module_.Input().Pulse().Enabled(false);

    // set frame sizes (2 samples per word), virtual channels are complex so 2*
    int samplesPerWord = module_.Input().Info().SamplesPerWord();

    for (int inst = 0; inst <= 1; ++inst) {
        write_dsp_register(inst, WB_FRAME_SIZE_OFFSET, recordLength/samplesPerWord + 8);
        for (int vchan = 1; vchan <= 4; ++vchan) {
            write_dsp_register(inst, WB_FRAME_SIZE_OFFSET+vchan, 2*recordLength/DECIMATION_FACTOR/samplesPerWord + 8);
        }
    }

    return SUCCESS;
}

X6_1000::ErrorCodes X6_1000::set_averager_settings(const int & recordLength, const int & numSegments, const int & waveforms,  const int & roundRobins) {
    set_frame(recordLength);
    numSegments_ = numSegments;
    waveforms_ = waveforms;
    roundRobins_ = roundRobins;
    numRecords_ = numSegments * waveforms * roundRobins;

    //Setup the accumulators
    for (auto & kv : activeChannels_) {
        accumulators_[kv.first].init(kv.second, recordLength_, numSegments_, waveforms_);
    }

    return SUCCESS;
}

X6_1000::ErrorCodes X6_1000::enable_stream(unsigned a, unsigned b, unsigned c) {
    FILE_LOG(logINFO) << "Enable stream " << a << "." << b << "." << c;
    Channel chan = Channel(a, b, c);
    FILE_LOG(logDEBUG2) << "Assigned stream " << a << "." << b << "." << c << " to streamID " << myhex << chan.streamID;
    activeChannels_[chan.streamID] = chan;
    accumulators_[chan.streamID] = Accumulator(chan, recordLength_, numSegments_, waveforms_);
    return SUCCESS;
}

X6_1000::ErrorCodes X6_1000::disable_stream(unsigned a, unsigned b, unsigned c) {
    //Find the channel
    uint16_t streamID = Channel(a,b,c).streamID;
    if (activeChannels_.count(streamID)){
        accumulators_.erase(streamID);
        activeChannels_.erase(streamID);
        FILE_LOG(logINFO) << "Disabling stream " << a << "." << b << "." << c;
        return SUCCESS;
    } 
    else{
        FILE_LOG(logERROR) << "Tried to disable stream " << a << "." << b << "." << c << " which was not enabled.";
        return SUCCESS;
    }  
}

bool X6_1000::get_channel_enable(int channel) {
    // TODO get active channel status from board
    if (channel >= get_num_channels()) return false;
    return true;
}

X6_1000::ErrorCodes X6_1000::set_active_channels() {
    ErrorCodes status = SUCCESS;

    module_.Output().ChannelDisableAll();
    module_.Input().ChannelDisableAll();

    for (int cnt = 0; cnt < get_num_channels(); cnt++) {
        FILE_LOG(logINFO) << "Physical channel " << cnt << " enabled";
        module_.Input().ChannelEnabled(cnt, 1);
    }
    return status;
}

void X6_1000::set_dsp_stream_ids() {
    for (int cnt = 0; cnt < VIRTUAL_CH_RATIO + 1; cnt++) {
        for (int physChan = 0; physChan < get_num_channels(); physChan++) {
            write_dsp_register(physChan, WB_STREAM_ID_OFFSET + cnt, 0x20000 + ((physChan+1) << 8) + (cnt << 4));
        }
    }

}

void X6_1000::set_defaults() {
    set_routes();
    set_reference();
    set_clock();
    set_trigger_source();
    set_decimation();
    set_active_channels();
    set_dsp_stream_ids();

    // disable test mode 
    module_.Input().TestModeEnabled( false, 0);
    module_.Output().TestModeEnabled( false, 0);
}

void X6_1000::log_card_info() {

    FILE_LOG(logINFO) << std::hex << "Logic Version: " << module_.Info().FpgaLogicVersion()
        << ", Hdw Variant: " << module_.Info().FpgaHardwareVariant()
        << ", Revision: " << module_.Info().PciLogicRevision()
        << ", Subrevision: " << module_.Info().FpgaLogicSubrevision();

    FILE_LOG(logINFO)  << std::hex << "Board Family: " << module_.Info().PciLogicFamily()
        << ", Type: " << module_.Info().PciLogicType()
        << ", Board Revision: " << module_.Info().PciLogicPcb()
        << ", Chip: " << module_.Info().FpgaChipType();

    FILE_LOG(logINFO)  << "PCI Express Lanes: " << module_.Debug()->LaneCount();
}

X6_1000::ErrorCodes X6_1000::acquire() {
    set_active_channels();
    // should only need to call this once, but for now we call it every time
    stream_.Preconfigure();

    // Initialize VeloMergeParsers with stream IDs
    VMPs_.clear();
    VMPs_.resize(3);

    physChans_.clear();
    virtChans_.clear();
    resultChans_.clear();

    for (auto kv : activeChannels_){
        switch (kv.second.type) {
            case PHYSICAL:
                physChans_.push_back(kv.first);
                FILE_LOG(logDEBUG) << "ADC physical stream ID: " << myhex << kv.first;
                break;
            case DEMOD:
                virtChans_.push_back(kv.first);
                FILE_LOG(logDEBUG) << "ADC virtual stream ID: " << myhex << kv.first;
                break;
            case RESULT:
                resultChans_.push_back(kv.first);
                FILE_LOG(logDEBUG) << "ADC result stream ID: " << myhex << kv.first;
        }
    }
    initialize_correlators();

    VMPs_[0].Init(physChans_);
    VMPs_[0].OnDataAvailable.SetEvent(this, &X6_1000::HandlePhysicalStream);

    VMPs_[1].Init(virtChans_);
    VMPs_[1].OnDataAvailable.SetEvent(this, &X6_1000::HandleVirtualStream);

    VMPs_[2].Init(resultChans_);
    VMPs_[2].OnDataAvailable.SetEvent(this, &X6_1000::HandleResultStream);

    //Now set the buffers sizes to fire when a full record length is in
    int samplesPerWord = module_.Input().Info().SamplesPerWord();
    FILE_LOG(logDEBUG) << "samplesPerWord = " << samplesPerWord;
    // calcate packet size for physical and virtual channels
    int packetSize = recordLength_/samplesPerWord/get_decimation();
    FILE_LOG(logDEBUG) << "Physical channel packetSize = " << packetSize;
    VMPs_[0].Resize(packetSize);
    VMPs_[0].Clear();

    //Vitual channels are complex so they get a factor of two.
    packetSize = 2*recordLength_/samplesPerWord/get_decimation()/DECIMATION_FACTOR;
    FILE_LOG(logDEBUG) << "Virtual channel packetSize = " << packetSize;
    VMPs_[1].Resize(packetSize);
    VMPs_[1].Clear();

    //Result channels are real/imag 32bit integers
    packetSize = 2; 
    FILE_LOG(logDEBUG) << "Result channel packetSize = " << packetSize;
    VMPs_[2].Resize(packetSize);
    VMPs_[2].Clear();

    // reset the accumulators
    for (auto & kv : accumulators_) {
        kv.second.reset();
    }
    recordsTaken_ = 0;

    module_.Velo().LoadAll_VeloDataSize(0x4000);
    module_.Velo().ForceVeloPacketSize(false);

    // is this necessary??
    stream_.PrefillPacketCount(prefillPacketCount_);
    
    trigger_.AtStreamStart();

    // flag must be set before calling stream start
    isRunning_ = true; 

    //  Start Streaming
    FILE_LOG(logINFO) << "Arming acquisition";
    stream_.Start();
    
    return SUCCESS;
}

X6_1000::ErrorCodes X6_1000::wait_for_acquisition(unsigned timeOut){
    /* Blocking wait until all the records have been acquired. */

    auto start = std::chrono::system_clock::now();
    auto end = start + std::chrono::seconds(timeOut);
    while (get_is_running()) {
        if (std::chrono::system_clock::now() > end)
            return X6_1000::TIMEOUT;
        std::this_thread::sleep_for( std::chrono::milliseconds(100) );
    }
    return SUCCESS;
}

X6_1000::ErrorCodes X6_1000::stop() {
    isRunning_ = false;
    stream_.Stop();
    timer_.Enabled(false);
    trigger_.AtStreamStop();
    return SUCCESS;
}

bool X6_1000::get_is_running() {
    return isRunning_;
}

bool X6_1000::get_has_new_data() {
    // determines if new data has arrived since the last call
    size_t currentRecords = 0;
    for (auto & kv : accumulators_) {
        currentRecords = max(currentRecords, kv.second.recordsTaken);
    }

    bool result = (currentRecords > recordsTaken_);
    recordsTaken_ = currentRecords;
    return result;
}

X6_1000::ErrorCodes X6_1000::transfer_waveform(unsigned a, unsigned b, unsigned c, double * buffer, size_t length) {
    //Check we have the channel
    uint16_t sid = Channel(a,b,c).streamID;
    if(activeChannels_.find(sid) == activeChannels_.end()){
        FILE_LOG(logERROR) << "Tried to transfer waveform from disabled stream.";
        return INVALID_CHANNEL;
    }
    //Don't copy more than we have
    if (length < accumulators_[sid].get_buffer_size() ) FILE_LOG(logERROR) << "Not enough memory allocated in buffer to transfer waveform.";
    accumulators_[sid].snapshot(buffer);
    return SUCCESS;
}

X6_1000::ErrorCodes X6_1000::transfer_variance(unsigned a, unsigned b, unsigned c, double * buffer, size_t length) {
    //Check we have the channel
    uint16_t sid = Channel(a,b,c).streamID;
    if(activeChannels_.find(sid) == activeChannels_.end()){
        FILE_LOG(logERROR) << "Tried to transfer waveform variance from disabled stream.";
        return INVALID_CHANNEL;
    }
    //Don't copy more than we have
    if (length < accumulators_[sid].get_buffer_size() ) FILE_LOG(logERROR) << "Not enough memory allocated in buffer to transfer variance.";
    accumulators_[sid].snapshot_variance(buffer);
    return SUCCESS;
}

X6_1000::ErrorCodes X6_1000::transfer_correlation(vector<Channel> & channels, double *buffer, size_t length) {
    // check that we have the correlator
    vector<uint16_t> sids(channels.size());
    for (int i = 0; i < channels.size(); i++)
        sids[i] = channels[i].streamID;
    if (correlators_.find(sids) == correlators_.end()) {
        FILE_LOG(logERROR) << "Tried to transfer invalid correlator.";
        return INVALID_CHANNEL;
    }
    // Don't copy more than we have
    if (length < correlators_[sids].get_buffer_size())
        FILE_LOG(logERROR) << "Not enough memory allocated in buffer to transfer correlator.";
    correlators_[sids].snapshot(buffer);
    return SUCCESS;
}

X6_1000::ErrorCodes X6_1000::transfer_correlation_variance(vector<Channel> & channels, double *buffer, size_t length) {
    // check that we have the correlator
    vector<uint16_t> sids(channels.size());
    for (int i = 0; i < channels.size(); i++)
        sids[i] = channels[i].streamID;
    if (correlators_.find(sids) == correlators_.end()) {
        FILE_LOG(logERROR) << "Tried to transfer invalid correlator.";
        return INVALID_CHANNEL;
    }
    // Don't copy more than we have
    if (length < correlators_[sids].get_buffer_size())
        FILE_LOG(logERROR) << "Not enough memory allocated in buffer to transfer correlator.";
    correlators_[sids].snapshot_variance(buffer);
    return SUCCESS;
}

int X6_1000::get_buffer_size(unsigned a, unsigned b, unsigned c) {
    uint16_t sid = Channel(a,b,c).streamID;
    return accumulators_[sid].get_buffer_size();
}

void X6_1000::initialize_correlators() {
    vector<uint16_t> streamIDs = {};
    vector<Channel> channels = {};

    // create all n-body correlators
    for (int n = 2; n < MAX_N_BODY_CORRELATIONS; n++) {
        streamIDs.resize(n);
        channels.resize(n);

        for (auto c : combinations(resultChans_.size(), n)) {
            for (int i = 0; i < n; i++) {
                streamIDs[i] = resultChans_[c[i]];
                channels[i] = activeChannels_[streamIDs[i]];
            }
            correlators_[streamIDs] = Correlator(channels, numSegments_, waveforms_);
        }
    }
}
/****************************************************************************
 * Event Handlers 
 ****************************************************************************/

 void  X6_1000::HandleDisableTrigger(OpenWire::NotifyEvent & /*Event*/) {
    FILE_LOG(logDEBUG) << "X6_1000::HandleDisableTrigger";
    module_.Input().Trigger().External(false);
    module_.Output().Trigger().External(false);
}


void  X6_1000::HandleExternalTrigger(OpenWire::NotifyEvent & /*Event*/) {
    FILE_LOG(logDEBUG) << "X6_1000::HandleExternalTrigger";
    module_.Input().Trigger().External(true);
    // module_.Input().Trigger().External( (triggerSource_ == EXTERNAL_TRIGGER) ? true : false );
}


void  X6_1000::HandleSoftwareTrigger(OpenWire::NotifyEvent & /*Event*/) {
    FILE_LOG(logDEBUG) << "X6_1000::HandleSoftwareTrigger";
}

void X6_1000::HandleBeforeStreamStart(OpenWire::NotifyEvent & /*Event*/) {
}

void X6_1000::HandleAfterStreamStart(OpenWire::NotifyEvent & /*Event*/) {
    FILE_LOG(logINFO) << "Analog I/O started";
    timer_.Enabled(true);
}
void
 X6_1000::HandleAfterStreamStop(OpenWire::NotifyEvent & /*Event*/) {
    FILE_LOG(logINFO) << "Analog I/O stopped";
    // Disable external triggering initially
    module_.Input().SoftwareTrigger(false);
    module_.Input().Trigger().External(false);
    VMPs_[0].Flush();
    VMPs_[1].Flush();
    VMPs_[2].Flush();
}

void X6_1000::HandleDataAvailable(Innovative::VitaPacketStreamDataEvent & Event) {
    if (!isRunning_) return;

    // create a buffer to receive the data
    VeloBuffer buffer;
    Event.Sender->Recv(buffer);


    AlignedVeloPacketExQ::Range InVelo(buffer);
    unsigned int * pos = InVelo.begin();
    VitaHeaderDatagram vh_dg(pos);
    FILE_LOG(logDEBUG3) << "[HandleDataAvailable] buffer.size() = " << buffer.SizeInInts() << "; buffer stream ID = " << myhex << vh_dg.StreamId();

    // broadcast to all VMPs
    for (auto & vmp : VMPs_) {
        vmp.Append(buffer);
        vmp.Parse();
    }

    if (check_done()) {
        FILE_LOG(logINFO) << "check_done() returned true. Stopping...";
        stop();
    }
}

void X6_1000::VMPDataAvailable(Innovative::VeloMergeParserDataAvailable & Event, channel_t chanType) {
    FILE_LOG(logDEBUG4) << "X6_1000::VMPDataAvailable";
    if (!isRunning_) {
        return;
    }
    // StreamID is now encoded in the PeripheralID of the VMP Vita buffer
    // PeripheralID is just the order of the streamID in the filter   
    PacketBufferHeader header(Event.Data);
    uint16_t sid;

    switch (chanType) {
        case PHYSICAL:
            sid = physChans_[header.PeripheralId()];
            break;
        case DEMOD:
            sid = virtChans_[header.PeripheralId()];
            break;
        case RESULT:
            sid = resultChans_[header.PeripheralId()];
            break;
    }
    
    // interpret the data as 16 or 32-bit integers depending on the channel type
    ShortDG sbufferDG(Event.Data);
    IntegerDG ibufferDG(Event.Data);
    switch (chanType) {
        case PHYSICAL:
        case DEMOD:
            FILE_LOG(logDEBUG3) << "[VMPDataAvailable] buffer SID = " << myhex << sid << "; buffer.size = " << std::dec << sbufferDG.size() << " samples";
            // accumulate the data in the appropriate channel
            if (accumulators_[sid].recordsTaken < numRecords_) {
                accumulators_[sid].accumulate(sbufferDG);
            }
            break;
        case RESULT:
            FILE_LOG(logDEBUG3) << "[VMPDataAvailable] buffer SID = " << myhex << sid << "; buffer.size = " << std::dec << ibufferDG.size() << " samples";
            // accumulate the data in the appropriate channel
            if (accumulators_[sid].recordsTaken < numRecords_) {
                accumulators_[sid].accumulate(ibufferDG);
                // correlate with other result channels
                for (auto kv : correlators_) {
                    if (std::find(kv.first.begin(), kv.first.end(), sid) != kv.first.end()) {
                        kv.second.accumulate(sid, ibufferDG);
                    }
                }
            }
            break;
    }
    
}

bool X6_1000::check_done() {
    int records;
    for (auto & kv : accumulators_) {
        FILE_LOG(logDEBUG2) << "Channel " << myhex << kv.first << " has taken " << std::dec << kv.second.recordsTaken << " records.";
    }
    for (auto & kv : accumulators_) {
            if (kv.second.recordsTaken < numRecords_) {
            return false;
        }
    }
    return true;
}

void X6_1000::HandleTimer(OpenWire::NotifyEvent & /*Event*/) {
    // FILE_LOG(logDEBUG) << "X6_1000::HandleTimer";
    trigger_.AtTimerTick();
}

void X6_1000::HandleSoftwareAlert(Innovative::AlertSignalEvent & event) {
    LogHandler("HandleSoftwareAlert");
}

void X6_1000::HandleWarningTempAlert(Innovative::AlertSignalEvent & event) {
    LogHandler("HandleWarningTempAlert");
}

void X6_1000::HandleInputFifoOverrunAlert(Innovative::AlertSignalEvent & event) {
    LogHandler("HandleInputFifoOverrunAlert");
}

void X6_1000::HandleInputOverrangeAlert(Innovative::AlertSignalEvent & event) {
    LogHandler("HandleInputOverrangeAlert");
}

void X6_1000::HandleTriggerAlert(Innovative::AlertSignalEvent & event) {
    std::string triggerType;
    switch (event.Argument & 0x3) {
        case 0:  triggerType = "? "; break;
        case 1:  triggerType = "Input "; break;
        case 2:  triggerType = "Output "; break;
        case 3:  triggerType = "Input and Output "; break;
    }
    std::stringstream msg;
    msg << "Trigger 0x" << std::hex << event.Argument
        << " Type: " <<  triggerType;
    FILE_LOG(logINFO) << msg.str();
}

void X6_1000::LogHandler(string handlerName) {
    FILE_LOG(logINFO) << "Alert:" << handlerName;
}

X6_1000::ErrorCodes X6_1000::set_digitizer_mode(const DIGITIZER_MODE & mode) {
    FILE_LOG(logINFO) << "Setting digitizer mode to: " << mode;
    return write_wishbone_register(WB_ADDR_DIGITIZER_MODE, WB_OFFSET_DIGITIZER_MODE, mode);
}

DIGITIZER_MODE X6_1000::get_digitizer_mode() const {
    return DIGITIZER_MODE(read_wishbone_register(WB_ADDR_DIGITIZER_MODE, WB_OFFSET_DIGITIZER_MODE));
}

X6_1000::ErrorCodes X6_1000::write_wishbone_register(uint32_t baseAddr, uint32_t offset, uint32_t data) {
     // Initialize WishboneAddress Space for APS specific firmware
    Innovative::AddressingSpace & logicMemory = Innovative::LogicMemorySpace(const_cast<X6_1000M&>(module_));
    Innovative::WishboneBusSpace WB_X6 = Innovative::WishboneBusSpace(logicMemory, baseAddr);
    Innovative::Register reg = Register(WB_X6, offset);
    reg.Value(data);
    return SUCCESS;
}


uint32_t X6_1000::read_wishbone_register(uint32_t baseAddr, uint32_t offset) const {
    Innovative::AddressingSpace & logicMemory = Innovative::LogicMemorySpace(const_cast<X6_1000M&>(module_));
    Innovative::WishboneBusSpace WB_X6 = Innovative::WishboneBusSpace(logicMemory, baseAddr);
    Innovative::Register reg = Register(WB_X6, offset);
    return reg.Value();
}

X6_1000::ErrorCodes X6_1000::write_dsp_register(unsigned instance, uint32_t offset, uint32_t data) {
    return write_wishbone_register(BASE_DSP[instance], offset, data);
}

uint32_t X6_1000::read_dsp_register(unsigned instance, uint32_t offset) const {
    return read_wishbone_register(BASE_DSP[instance], offset);
}

Accumulator::Accumulator() : 
    wfmCt_{0}, recordLength_{0}, numSegments_{0}, numWaveforms_{0}, recordsTaken{0} {}; 

Accumulator::Accumulator(const Channel & chan, const size_t & recordLength, const size_t & numSegments, const size_t & numWaveforms) : 
                         channel_{chan}, wfmCt_{0}, numSegments_{numSegments}, numWaveforms_{numWaveforms}, recordsTaken{0} {
    recordLength_ = calc_record_length(chan, recordLength);
    data_.assign(recordLength_*numSegments, 0);
    idx_ = data_.begin();
    if (chan.type == PHYSICAL) {
        data2_.assign(recordLength_*numSegments_, 0);
    } else {
        // complex data, so 3-component correlations (real*real, imag*imag, real*imag)
        data2_.assign(recordLength_*numSegments*3/2, 0);
    }
    idx2_ = data2_.begin();
    fixed_to_float_ = fixed_to_float(chan);
};

void Accumulator::init(const Channel & chan, const size_t & recordLength, const size_t & numSegments, const size_t & numWaveforms) {
    channel_ = chan;
    recordLength_ = calc_record_length(chan, recordLength);
    data_.assign(recordLength_*numSegments, 0);
    idx_ = data_.begin();
    if (chan.type == PHYSICAL) {
        data2_.assign(recordLength_*numSegments, 0);
    } else {
        // complex data, so 3-component correlations (real*real, imag*imag, real*imag)
        data2_.assign(recordLength_*numSegments*3/2, 0);
    }
    idx2_ = data2_.begin();
    wfmCt_ = 0;
    numSegments_ = numSegments;
    numWaveforms_ = numWaveforms;
    recordsTaken = 0;
    fixed_to_float_ = fixed_to_float(chan);
}

void Accumulator::reset() {
    data_.assign(recordLength_*numSegments_, 0);
    idx_ = data_.begin();
    std::fill(data2_.begin(), data2_.end(), 0);
    idx2_ = data_.begin();
    wfmCt_ = 0;
    recordsTaken = 0;
}

size_t Accumulator::calc_record_length(const Channel & chan, const size_t & recordLength) {
    switch (chan.type) {
        case PHYSICAL:
            return recordLength;
            break;
        case DEMOD:
            return 2 * recordLength / DECIMATION_FACTOR;
            break;
        case RESULT:
            return 2;
            break;
    }
}

int Accumulator::fixed_to_float(const Channel & chan) {
    switch (chan.type) {
        case PHYSICAL:
            return 1 << 11; // signed 12-bit integers from ADC
            break;
        case DEMOD:
        case RESULT:
            return 1 << 14; // sfix16_14 from DDC
            break;
    }
}

size_t Accumulator::get_buffer_size() {
    return data_.size();
}

void Accumulator::snapshot(double * buf) {
    /* Copies current data into a *preallocated* buffer*/
    double scale = max(static_cast<int>(recordsTaken), 1) / numSegments_ * fixed_to_float_;
    for(size_t ct=0; ct < data_.size(); ct++){
        buf[ct] = static_cast<double>(data_[ct]) / scale;
    }
}

void Accumulator::snapshot_variance(double * buf) {
    int64_t N = max(static_cast<int>(recordsTaken / numSegments_), 1);
    double scale = (N-1) * fixed_to_float_ * fixed_to_float_;

    if (N < 2) {
        for(size_t ct=0; ct < data2_.size(); ct++){
            buf[ct] = 0.0;
        }
    } else {
        // construct complex vector of data
        std::complex<double>* cvec = reinterpret_cast<std::complex<double> *>(data_.data());
        // calculate 3 components of variance
        for(size_t ct=0; ct < data_.size()/2; ct++) {
            buf[3*ct] = static_cast<double>(data2_[3*ct] - cvec[ct].real()*cvec[ct].real()/N) / scale;
            buf[3*ct+1] = static_cast<double>(data2_[3*ct+1] - cvec[ct].imag()*cvec[ct].imag()/N) / scale;
            buf[3*ct+2] = static_cast<double>(data2_[3*ct+2] - cvec[ct].real()*cvec[ct].imag()/N) / scale;
        }
    }
}

template <class T>
void Accumulator::accumulate(const AccessDatagram<T> & buffer) {
    //TODO: worry about performance, cache-friendly etc.
    FILE_LOG(logDEBUG4) << "Accumulating data...";
    FILE_LOG(logDEBUG4) << "recordLength_ = " << recordLength_ << "; idx_ = " << std::distance(data_.begin(), idx_) << "; recordsTaken = " << recordsTaken;
    FILE_LOG(logDEBUG4) << "New buffer size is " << buffer.size();
    FILE_LOG(logDEBUG4) << "Accumulator buffer size is " << data_.size();

    // The assumption is that this will be called with a full record size
    // Accumulate the buffer into data_
    std::transform(idx_, idx_+recordLength_, buffer.begin(), idx_, std::plus<int64_t>());
    // record the square of the buffer as well
    if (channel_.type == PHYSICAL) {
        // data is real, just square and sum it.
        std::transform(idx2_, idx2_+recordLength_, buffer.begin(), idx2_, [](int64_t a, int64_t b) {
            return a + b*b;
        });
    } else {
        // data is complex: real/imaginary are interleaved every other point
        // form a complex vector from the input buffer
        vector<std::complex<int64_t>> cvec(recordLength_/2);
        for (int i = 0; i < recordLength_/2; i++) {
            cvec[i] = std::complex<int64_t>(buffer[2*i], buffer[2*i+1]);
        }
        // calculate 3-component correlations into a triple of successive points
        for (int i = 0; i < cvec.size(); i++) {
            idx2_[3*i] += cvec[i].real() * cvec[i].real();
            idx2_[3*i+1] += cvec[i].imag() * cvec[i].imag();
            idx2_[3*i+2] += cvec[i].real() * cvec[i].imag();
        }
    }
    recordsTaken++;

    //If we've filled up the number of waveforms move onto the next segment, otherwise jump back to the beginning of the record
    if (++wfmCt_ == numWaveforms_) {
        wfmCt_ = 0;
        std::advance(idx_, recordLength_);
        if (channel_.type == PHYSICAL)
            std::advance(idx2_, recordLength_);
        else
            std::advance(idx2_, recordLength_ * 3/2);
    }

    //Final check if we're at the end
    if (idx_ == data_.end()) {
        idx_ = data_.begin();
        idx2_ = data2_.begin();
    }
}

Correlator::Correlator() : 
    wfmCt_{0}, recordLength_{2}, numSegments_{0}, numWaveforms_{0}, recordsTaken{0} {}; 

Correlator::Correlator(const vector<Channel> & channels, const size_t & numSegments, const size_t & numWaveforms) : 
                         wfmCt_{0}, numSegments_{numSegments}, numWaveforms_{numWaveforms}, recordsTaken{0} {
    recordLength_ = 2; // assume a RESULT channel
    buffers_.resize(channels.size());
    data_.assign(recordLength_*numSegments, 0);
    idx_ = data_.begin();
    data2_.assign(recordLength_*numSegments*3/2, 0);
    idx2_ = data2_.begin();
    // set up mapping of SIDs to an index into buffers_
    fixed_to_float_ = 1;
    for (int i = 0; i < channels.size(); i++) {
        bufferSID_[channels[i].streamID] = i;
        fixed_to_float_ *= 1 << 14; // assumes a RESULT channel, grows with the number of terms in the correlation
    }
};

void Correlator::reset() {
    for (int i = 0; i < buffers_.size(); i++)
        buffers_[i].clear();
    data_.assign(recordLength_*numSegments_, 0);
    idx_ = data_.begin();
    data2_.assign(recordLength_*numSegments_*3/2, 0);
    idx2_ = data2_.begin();
    wfmCt_ = 0;
    recordsTaken = 0;
}

template <class T>
void Correlator::accumulate(const int & sid, const AccessDatagram<T> & buffer) {
    // copy the data
    for (int i = 0; i < buffer.size(); i++)
        buffers_[bufferSID_[sid]].push_back(buffer[i]);
    correlate();
}

void Correlator::correlate() {
    vector<size_t> bufsizes(buffers_.size());
    std::transform(buffers_.begin(), buffers_.end(), bufsizes.begin(), [](vector<int> b) {
        return b.size();
    });
    size_t minsize = *std::min_element(bufsizes.begin(), bufsizes.end());
    if (minsize == 0)
        return;
    
    // correlate
    // data is real/imag interleaved, so process a pair of points at a time from each channel
    for (int i = 0; i < minsize; i += 2) {
        std::complex<double> c = 1;
        for (int j = 0; j < buffers_.size(); j++) {
            c *= std::complex<double>(buffers_[j][i], buffers_[j][i+1]);
        }
        c /= fixed_to_float_;
        idx_[0] += c.real();
        idx_[1] += c.imag();
        idx2_[0] += c.real()*c.real();
        idx2_[1] += c.imag()*c.imag();
        idx2_[2] += c.real()*c.imag();

        if (++wfmCt_ == numWaveforms_) {
            wfmCt_ = 0;
            std::advance(idx_, 2);
            std::advance(idx2_, 3);
            if (idx_ == data_.end()) {
                idx_ = data_.begin();
                idx2_ = data2_.begin();
            }
        }
    }
    for (int j = 0; j < buffers_.size(); j++)
        buffers_[j].erase(buffers_[j].begin(), buffers_[j].begin()+minsize);

    recordsTaken += minsize/2;
}

size_t Correlator::get_buffer_size() {
    return data_.size();
}

void Correlator::snapshot(double * buf) {
    /* Copies current data into a *preallocated* buffer*/
    double N = max(static_cast<int>(recordsTaken), 1) / numSegments_;
    for(size_t ct=0; ct < data_.size(); ct++){
        buf[ct] = data_[ct] / N;
    }
}

void Correlator::snapshot_variance(double * buf) {
    int64_t N = max(static_cast<int>(recordsTaken / numSegments_), 1);

    if (N < 2) {
        for(size_t ct=0; ct < data2_.size(); ct++){
            buf[ct] = 0.0;
        }
    } else {
        // construct complex vector of data
        std::complex<double>* cvec = reinterpret_cast<std::complex<double> *>(data_.data());
        // calculate 3 components of variance
        for(size_t ct=0; ct < data_.size()/2; ct++) {
            buf[3*ct] = (data2_[3*ct] - cvec[ct].real()*cvec[ct].real()/N) / (N-1);
            buf[3*ct+1] = (data2_[3*ct+1] - cvec[ct].imag()*cvec[ct].imag()/N) / (N-1);
            buf[3*ct+2] = (data2_[3*ct+2] - cvec[ct].real()*cvec[ct].imag()/N) / (N-1);
        }
    }
}

Channel::Channel() : channelID{0,0,0}, streamID{0}, type{PHYSICAL} {};

Channel::Channel(unsigned a, unsigned b, unsigned c) : channelID{a,b,c} {
    streamID = (a << 8) + (b << 4) + c;
    if ((b == 0) && (c == 0)) {
        type = PHYSICAL;
    } else if (c != 0) {
        type = RESULT;
    } else {
        type = DEMOD;
    }
};

vector<vector<int>> combinations(int n, int r) {
    /*
     * Returns all combinations of r choices from the list of integers 0,1,...,n-1.
     * Based upon code in the Julia standard library.
     */
    vector<vector<int>> c;
    vector<int> s(r);
    int i;
    for (i = 0; i < r; i++)
        s[i] = i;
    c.push_back(s);
    while (s[0] < n - r) {
        for (i = r-1; i >= 0; i--) {
            s[i] += 1;
            if (s[i] > n - r + i)
                continue;
            for (int j = i+1; j < r; j++)
                s[j] = s[j-1] + 1;
            break;
        }
        c.push_back(s);
    }
    return c;
}
