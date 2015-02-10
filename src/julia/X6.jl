module X6ADC

type X6
	id::Int
end
X6() = X6(-1)

const DSP_WB_OFFSET = [0x2000, 0x2100]
const X6_LIBRARY = "C:\\Users\\qlab\\Documents\\GitHub\\libx6\\build\\libx6adc"
const DECIMATION_FACTOR = 4

immutable Channel
	a::Cint
	b::Cint
	c::Cint
end

function connect!(dev::X6, id)
	ret = ccall((:connect_by_ID, X6_LIBRARY), Int32, (Int32,), id)
	if ret == 0
		dev.id = id
	else
		error("Unable to open connection to X6 $id")
	end
end

function disconnect!(dev::X6)
	ret = ccall((:disconnect, X6_LIBRARY), Int32, (Int32,), dev.id)
	dev.id = -1
end

function is_open(dev::X6)
	bool(ccall((:is_open, X6_LIBRARY), Int32, (Int32,), dev.id))
end

function set_logging_level(level)
	#Sets logging level library wide
	ccall((:set_logging_level, X6_LIBRARY), Int32, (Int32,), level)
end

function get_logic_temperature(dev::X6)
	ccall((:get_logic_temperature, X6_LIBRARY), Float32, (Int32, Int32), dev.id, 0)
end

function get_sampleRate(dev::X6)
	ccall((:get_sampleRate, X6_LIBRARY), Float64, (Int32,), dev.id)
end

function set_sampleRate(dev::X6, rate::Real)
	ccall((:set_sampleRate, X6_LIBRARY), Int32, (Int32, Float64), dev.id, rate)
end

function get_trigger_source(dev::X6)
	ccall((:get_trigger_source, X6_LIBRARY), Int32, (Int32,), dev.id)
end	

function set_trigger_source(dev::X6, source::Int)
	ccall((:set_trigger_source, X6_LIBRARY), Int32, (Int32, Int32), dev.id, source)
end	

function get_reference(dev::X6)
	strMap = {0 => "internal", 1=>"external"}
	ref = ccall((:get_reference, X6_LIBRARY), Int32, (Int32,), dev.id)
	strMap[ref]
end

function set_reference(dev::X6, ref::String)
	strMap = {"internal" => 0, "external"=> 1}
	ccall((:set_reference, X6_LIBRARY), Int32, (Int32, Int32), dev.id, strMap[ref])
end

function enable_stream(dev::X6, a, b, c)
	ccall((:enable_stream, X6_LIBRARY), Int32, (Int32, Int32, Int32, Int32), dev.id, a, b, c)
end

function disable_stream(dev::X6, a, b, c)
	ccall((:disable_stream, X6_LIBRARY), Int32, (Int32, Int32, Int32, Int32), dev.id, a, b, c)
end

function read_register(dev::X6, addr, offset)
	#Read an address in the wishbone register space
	ccall((:read_register, X6_LIBRARY), Int32, (Int32, Int32, Int32), dev.id, addr, offset)
end

function write_register(dev::X6, addr, offset, value)
	#Write an address in the wishbone register space
	ccall((:write_register, X6_LIBRARY), Int32, (Int32, Int32, Int32, Int32), dev.id, addr, offset, value)
end	

function set_averager_settings(dev::X6, recordLength::Int, numSegments::Int, waveforms::Int, roundRobins::Int)
	ccall((:set_averager_settings, X6_LIBRARY), Int32, (Int32, Int32, Int32, Int32, Int32),	
											dev.id, recordLength, numSegments, waveforms, roundRobins) 
end

function acquire(dev::X6)
	ccall((:acquire, X6_LIBRARY), Int32, (Int32,), dev.id)
end

function stop(dev::X6)
	ccall((:stop, X6_LIBRARY), Int32, (Int32,), dev.id)
end

function wait_for_acquisition(dev::X6, timeOut::Int)
	ccall((:wait_for_acquisition, X6_LIBRARY), Int32, (Int32, Int32), dev.id, timeOut)
end

function transfer_waveform(dev::X6, a, b, c)
	channels = [Channel(a,b,c)]
	bufSize = ccall((:get_buffer_size, X6_LIBRARY), Int32, (Int32, Ptr{Channel}, Uint32), dev.id, channels, length(channels))
	wfs = Array(Float64, bufSize)
	success = ccall((:transfer_waveform, X6_LIBRARY), Int32, (Int32, Ptr{Channel}, Uint32, Ptr{Float64}, Int32),
													dev.id, channels, length(channels), wfs, bufSize)
	@assert success == 0 "Transferring waveforms failed!"
	if (b == 0) # physical channel
		return wfs
	else
		return wfs[1:2:end] + 1im*wfs[2:2:end]
	end
end

function unittest()
	set_logging_level(6)

	x6 = X6()

	connect!(x6, 0)

	@assert is_open(x6) "Could not open connection to X6 board."

	@printf("Current logic temperature is %.1f\n", get_logic_temperature(x6))

	@printf("Current PLL frequency is %.2f GHz\n", get_sampleRate(x6)/1e9)

	set_reference(x6, "external")

	println("Enabling streams...")
	for physChan = 1:2
		# enable raw stream
		enable_stream(x6, physChan, 0, 0)
		for demodChan = 1:2
			for resultChan = 0:1
				enable_stream(x6, physChan, demodChan, resultChan)
			end
		end
	end

	println("Setting NCO phase increments...")
	phaseInc = 4/100
	write_register(x6, DSP_WB_OFFSET[1], 0x0010, int(round(1 * phaseInc * 2^18)))
	write_register(x6, DSP_WB_OFFSET[1], 0x0011, int(round(1 * phaseInc * 2^18)))
	write_register(x6, DSP_WB_OFFSET[2], 0x0010, int(round(1 * phaseInc * 2^18)))
	write_register(x6, DSP_WB_OFFSET[2], 0x0011, int(round(1 * phaseInc * 2^18)))

	println("Writing kernel lengths")
	write_register(x6, DSP_WB_OFFSET[1], 24+1-1, 2)
	write_register(x6, DSP_WB_OFFSET[1], 24+2-1, 2)
	write_register(x6, DSP_WB_OFFSET[2], 24+1-1, 2)
	write_register(x6, DSP_WB_OFFSET[2], 24+2-1, 2)

	set_averager_settings(x6, 2048, 9, 1, 1)

	println("Acquiring...")
	acquire(x6)
	success = wait_for_acquisition(x6, 1)
	println("Wait for acquisition returned $success")
	stop(x6)

	rawwfs = [transfer_waveform(x6, physChan, 0, 0) for physChan = 1:2]
	demodwfs = [transfer_waveform(x6, physChan, demodChan, 0) for physChan = 1:2, demodChan = 1:2]
	resultwfs = [transfer_waveform(x6, physChan, demodChan, 1) for physChan = 1:2, demodChan = 1:2]

	disconnect!(x6)

	return rawwfs, demodwfs, resultwfs
end

end #module