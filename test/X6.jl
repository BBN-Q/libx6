type X6
	id::Int
end
X6() = X6(-1)

const DSP_WB_OFFSET = [0x2000, 0x2100]
const X6_LIBRARY = "libx6adc"

function connect!(dev::X6, id)
	ret = ccall((:connect_by_ID, X6_LIBRARY), Int32, (Int32,), id)
	if ret == 0
		dev.id = id
	else
		error("Unable to open connection to X6 $id")
	end
end

function disconnect!(dev::X6)
	ret = ccall((:disconnect, X6_LIBRARY), Int32, (Int32,), id)
	dev.id = -1
end

function is_open(dev::X6)
	bool(ccall((:is_open, X6_LIBRARY), Int32, (Int32,), dev.id))
end

function set_logging_level(level)
	#Sets logging level library wide
	ccall((:set_logging_level, X6_LIBRARY), Int32, (Int32,), level)
end

function read_register(dev::X6, addr::Uint16, offset::Uint16)
	#Read an address in the wishbone register space
	ccall((:read_register, X6_LIBRARY), Int32, (Int32, Int32, Int32), addr, offset)
end