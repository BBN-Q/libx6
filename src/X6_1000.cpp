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
    write_wishbone_register(WB_FRAME_SIZE_OFFSET, recordLength/samplesPerWord);
    write_wishbone_register(WB_FRAME_SIZE_OFFSET+1, 2*recordLength/DECIMATION_FACTOR/samplesPerWord);
    write_wishbone_register(WB_FRAME_SIZE_OFFSET+2, 2*recordLength/DECIMATION_FACTOR/samplesPerWord);
    write_wishbone_register(WB_FRAME_SIZE_OFFSET+3, 2*recordLength/DECIMATION_FACTOR/samplesPerWord);
    write_wishbone_register(WB_FRAME_SIZE_OFFSET+4, 2*recordLength/DECIMATION_FACTOR/samplesPerWord);

    return SUCCESS;
}

X6_1000::ErrorCodes X6_1000::set_averager_settings(const int & recordLength, const int & numSegments, const int & waveforms,  const int & roundRobins) {
    set_frame(recordLength);
    numSegments_ = numSegments;
    waveforms_ = waveforms;
    roundRobins_ = roundRobins;
    numRecords_ = numSegments * waveforms * roundRobins;

    //Setup the accumulators
    for (auto & kv : accumulators_) {
        ssize_t recLength = recordLength;
        if (kv.first >= 10) {
            recLength /= DECIMATION_FACTOR;
        }
        kv.second.init(kv.first < 10 ? recordLength : recordLength/DECIMATION_FACTOR, numSegments, waveforms);
    }

    return SUCCESS;
}

X6_1000::ErrorCodes X6_1000::enable_stream(unsigned phys, unsigned demod) {
    FILE_LOG(logINFO) << "Enable stream " << phys << "." << demod;
    Channel chan = Channel(phys, demod);
    FILE_LOG(logDEBUG2) << "Assigned stream " << phys << "." << demod << " to streamID " << chan.streamID;
    activeChannels_[chan.streamID] = chan;
    accumulators_[chan.streamID] = Accumulator(chan.isPhys()  ? recordLength_ : 2*recordLength_/DECIMATION_FACTOR, numSegments_, waveforms_);
    return SUCCESS;
}

X6_1000::ErrorCodes X6_1000::disable_stream(unsigned phys, unsigned demod){
    //Find the channel
    uint16_t streamID = Channel::calc_streamID(phys, demod);
    if (activeChannels_.count(streamID)){
        accumulators_.erase(streamID);
        activeChannels_.erase(streamID);
        FILE_LOG(logINFO) << "Disabling stream " << phys << "." << demod;
        return SUCCESS;
    } 
    else{
        FILE_LOG(logERROR) << "Tried to disable stream " << phys << "." << demod << " which was not enabled.";
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

int X6_1000::num_active_channels() {
    return 2;
}

void X6_1000::set_defaults() {
    set_routes();
    set_reference();
    set_clock();
    set_trigger_source();
    set_decimation();
    set_active_channels();

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
    VMPs_.resize(2);

    physChans_.clear();
    virtChans_.clear();

    for (auto kv : activeChannels_){
        if (kv.second.isPhys()){
            physChans_.push_back(kv.first);
            FILE_LOG(logDEBUG) << "ADC physical stream ID: " << myhex << kv.first;
        } else{
            virtChans_.push_back(kv.first);
            FILE_LOG(logDEBUG) << "ADC virtual stream ID: " << myhex << kv.first;
        }
    }
    VMPs_[0].Init(physChans_);
    VMPs_[0].OnDataAvailable.SetEvent(this, &X6_1000::HandlePhysicalStream);

    VMPs_[1].Init(virtChans_);
    VMPs_[1].OnDataAvailable.SetEvent(this, &X6_1000::HandleVirtualStream);

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

    // reset the accumulators
    for (auto & kv : accumulators_) {
        kv.second.reset();
    }

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

X6_1000::ErrorCodes X6_1000::transfer_waveform(int channel, double * buffer, size_t length) {
    //Don't copy more than we have
    if (length < accumulators_[channel].data_.size() ) FILE_LOG(logERROR) << "Not enough memory allocated in buffer to transfer waveform.";
    accumulators_[channel].snapshot(buffer);
    return SUCCESS;
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

void X6_1000::VMPDataAvailable(Innovative::VeloMergeParserDataAvailable & Event, ChannelType chanType) {
    FILE_LOG(logDEBUG4) << "X6_1000::VMPDataAvailable";
    if (!isRunning_) {
        return;
    }
    // StreamID is now encoded in the PeripheralID of the VMP Vita buffer
    // PeripheralID is just the order of the streamID in the filter   
    PacketBufferHeader header(Event.Data);
    uint16_t sid;
    if (chanType == PHYS_CHAN){
        sid = physChans_[header.PeripheralId()];
    } else{
        sid = virtChans_[header.PeripheralId()];
    }

    // interpret the data as integers
    ShortDG bufferDG(Event.Data);
    FILE_LOG(logDEBUG3) << "[VMPDataAvailable] buffer SID = " << sid << "; buffer.size = " << bufferDG.size() << " samples";
    // accumulate the data in the appropriate channel
    accumulators_[sid].accumulate(bufferDG);
}

bool X6_1000::check_done() {
    int records;
    for (auto & kv : accumulators_) {
        FILE_LOG(logDEBUG2) << "Channel " << kv.first << " has taken " << kv.second.recordsTaken << " records.";
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

X6_1000::ErrorCodes X6_1000::write_wishbone_register(uint32_t offset, uint32_t data) {
    return write_wishbone_register(BASE_DSP, offset, data);
}

uint32_t X6_1000::read_wishbone_register(uint32_t baseAddr, uint32_t offset) const {
    Innovative::AddressingSpace & logicMemory = Innovative::LogicMemorySpace(const_cast<X6_1000M&>(module_));
    Innovative::WishboneBusSpace WB_X6 = Innovative::WishboneBusSpace(logicMemory, baseAddr);
    Innovative::Register reg = Register(WB_X6, offset);
    return reg.Value();
}

uint32_t X6_1000::read_wishbone_register(uint32_t offset) const {
    return read_wishbone_register(BASE_DSP, offset);
}

Accumulator::Accumulator() : 
    wfmCt_{0}, recordLength_{0}, numSegments_{0}, numWaveforms_{0}, recordsTaken{0} {}; 

Accumulator::Accumulator(const size_t & recordLength, const size_t & numSegments, const size_t & numWaveforms) : 
    wfmCt_{0}, recordLength_{recordLength}, numSegments_{numSegments}, numWaveforms_{numWaveforms}, recordsTaken{0} {
        data_.assign(recordLength*numSegments, 0);
        idx_ = data_.begin();
    }; 

void Accumulator::init(const size_t & recordLength, const size_t & numSegments, const size_t & numWaveforms){
    data_.assign(recordLength*numSegments, 0);
    idx_ = data_.begin();
    wfmCt_ = 0;
    recordLength_  = recordLength;
    numSegments_ = numSegments;
    numWaveforms_ = numWaveforms;
    recordsTaken = 0;
}

void Accumulator::reset(){
    data_.assign(recordLength_*numSegments_, 0);
    idx_ = data_.begin();
    wfmCt_ = 0;
    recordsTaken = 0;
}

void Accumulator::snapshot(double * buf){
    /* Copies current data into a *preallocated* buffer*/
    double scale = recordsTaken / (numSegments_*numWaveforms_) * 2048;
    for(size_t ct=0; ct < data_.size(); ct++){
        buf[ct] = static_cast<double>(data_[ct]) / scale;
    }
}

void Accumulator::accumulate(const ShortDG & buffer){
    //TODO: worry about performance, cache-friendly etc.
    FILE_LOG(logDEBUG4) << "Accumulating data for unknown channel...";
    FILE_LOG(logDEBUG4) << "recordLength_ = " << recordLength_ << "; idx_ = " << std::distance(data_.begin(), idx_) << "; recordsTaken = " << recordsTaken;
    FILE_LOG(logDEBUG4) << "New buffer size is " << buffer.size();
    FILE_LOG(logDEBUG4) << "Accumulator buffer size is " << data_.size();

    //The assumption is that this will be called with a full record size
    std::transform(idx_, idx_+recordLength_, buffer.begin(), idx_, std::plus<int>());
    recordsTaken++;

    //If we've filled up the number of waveforms move onto the next segment, otherwise jump back to the beginning of the record
    if (++wfmCt_ == numWaveforms_){
        wfmCt_ = 0;
        std::advance(idx_, recordLength_);
    }

    //Final check if we're at the end
    if (idx_ == data_.end()){
        idx_ = data_.begin();
    }

}

Channel::Channel() : physChan{0}, demodChan{0}, streamID{0} {};

Channel::Channel(unsigned phys, unsigned demod) : physChan{phys}, demodChan{demod} {
    streamID = calc_streamID(phys, demod);
};

uint16_t Channel::calc_streamID(unsigned phys, unsigned demod){
    return (phys << 8) + (demod << 4); 
}

bool Channel::isPhys(){
    return demodChan == 0;
}
