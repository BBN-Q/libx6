import os
import platform
import warnings
import numpy as np
import numpy.ctypeslib as npct
from ctypes import c_int32, c_uint32, c_float, c_double, c_char_p, c_bool, create_string_buffer, byref, POINTER, Structure

np_double = npct.ndpointer(dtype=np.double, ndim=1, flags='CONTIGUOUS')
np_complex = npct.ndpointer(dtype=np.complex128, ndim=1, flags='CONTIGUOUS')

# load the shared library
build_path = os.path.abspath(os.path.join(
    os.path.dirname(__file__), "..", "..", "build"))
# On Windows add build path to system path to pick up DLL mingw dependencies
if "Windows" in platform.platform():
    os.environ["PATH"] += ";" + build_path
libx6 = npct.load_library("libx6", build_path)

class Channel(Structure):
    _fields_ = [("a", c_int32),
                ("b", c_int32),
                ("c", c_int32)]

libx6.connect_x6.argtypes              = [c_int32]
libx6.disconnect_x6.argtypes           = [c_int32]
libx6.get_num_devices.argtypes         = [POINTER(c_uint32)]
libx6.get_firmware_version.argtypes    = [c_int32] + [POINTER(c_uint32)]*3 + [c_char_p]

# libx6.set_reference_source.argtypes    = [c_int32, X6_REFERENCE_SOURCE]
# libx6.get_reference_source.argtypes    = [c_int32, POINTER(X6_REFERENCE_SOURCE)]
# libx6.set_digitizer_mode.argtypes      = [c_int32, X6_DIGITIZER_MODE]
# libx6.get_digitizer_mode.argtypes      = [c_int32, POINTER(X6_DIGITIZER_MODE)]

libx6.enable_stream.argtypes           = [c_int32]*4
libx6.disable_stream.argtypes          = [c_int32]*4

libx6.set_nco_frequency.argtypes       = [c_int32]*3 + [c_double]
libx6.get_nco_frequency.argtypes       = [c_int32]*3 + [POINTER(c_double)]

libx6.set_averager_settings.argtypes   = [c_int32]*5
libx6.set_threshold.argtypes           = [c_int32]*3 + [c_double]
libx6.get_threshold.argtypes           = [c_int32]*3 + [POINTER(c_double)]
libx6.set_threshold_invert.argtypes    = [c_int32]*3 + [c_bool]
libx6.get_threshold_invert.argtypes    = [c_int32]*3 + [POINTER(c_bool)]

libx6.write_kernel.argtypes            = [c_int32] + [c_uint32]*3 + [np_complex, c_uint32]
libx6.read_kernel.argtypes             = [c_int32] + [c_uint32]*4 + [np_complex]
libx6.set_kernel_bias.argtypes         = [c_int32] + [c_uint32]*3 + [np_complex]
libx6.get_kernel_bias.argtypes         = [c_int32] + [c_uint32]*3 + [np_complex]

libx6.acquire.argtypes                 = [c_int32]
libx6.wait_for_acquisition.argtypes    = [c_int32, c_uint32]
libx6.get_is_running                   = [c_int32, POINTER(c_bool)]
libx6.get_num_new_records.argtypes     = [c_int32, POINTER(c_uint32)]
libx6.stop.argtypes                    = [c_int32]
libx6.transfer_stream.argtypes         = [c_int32, POINTER(Channel), c_uint32,
                                          np_double, c_int32]
libx6.transfer_variance.argtypes       = [c_int32, POINTER(Channel), c_uint32,
                                          np_double, c_int32]
libx6.get_buffer_size.argtypes         = [c_int32, POINTER(Channel), c_uint32, POINTER(c_uint32)]
libx6.get_record_length.argtypes       = [c_int32, POINTER(Channel), POINTER(c_uint32)]
libx6.get_variance_buffer_size.argtypes = [c_int32, POINTER(Channel), c_uint32, POINTER(c_int32)]

libx6.set_log.argtypes                 = [c_char_p]
libx6.set_logging_level.argtypes       = [c_int32]

libx6.get_logic_temperature.argtypes   = [c_int32, POINTER(c_float)]

# uniform restype = c_int32, so add it all at once
attributes = [a for a in dir(libx6) if not a.startswith('__')]
for a in attributes:
    a.restype = c_int32

# reference source
reference_dict = {0: "internal", 1: "external"}

# enum APS2_STATUS {
X6_OK                          = 0
X6_UNKNOWN_ERROR               = -1
X6_NO_DEVICE_FOUND             = -2
X6_UNCONNECTED                 = -3
X6_INVALID_FREQUENCY           = -4
X6_TIMEOUT                     = -5
X6_INVALID_CHANNEL             = -6
X6_LOGFILE_ERROR               = -7
X6_INVALID_RECORD_LENGTH       = -8
X6_MODULE_ERROR                = -9
X6_INVALID_WF_LEN              = -10
X6_WF_OUT_OF_RANGE             = -11
X6_INVALID_KERNEL_STREAM       = -12
X6_INVALID_KERNEL_LENGTH       = -13
X6_KERNEL_OUT_OF_RANGE         = -14
X6_MODULE_ERROR                = -15

status_dict = {
    0: "X6_OK",
    -1: "X6_UNKNOWN_ERROR",
    -2: "X6_NO_DEVICE_FOUND",
    -3: "X6_UNCONNECTED",
    -4: "X6_INVALID_FREQUENCY",
    -5: "X6_TIMEOUT",
    -6: "X6_INVALID_CHANNEL",
    -7: "X6_LOGFILE_ERROR",
    -8: "X6_INVALID_RECORD_LENGTH",
    -9: "X6_MODULE_ERROR",
    -10: "X6_INVALID_WF_LEN",
    -11: "X6_WF_OUT_OF_RANGE",
    -12: "X6_INVALID_KERNEL_STREAM",
    -13: "X6_INVALID_KERNEL_LENGTH",
    -14: "X6_KERNEL_OUT_OF_RANGE",
    -15: "X6_MODULE_ERROR",
}

libx6.get_error_msg.restype = c_char_p


def get_error_msg(status_code):
    return libx6.get_error_msg(status_code).decode("utf-8")

def check(status_code):
    if status_code is 0:
        return
    else:
        raise Exception("LIBX6 Error: {} - {}".format(status_dict[
            status_code], get_error_msg(status_code)))

def get_num_devices():
    num_devices = c_uint32()
    check(libx6.get_num_devices(byref(num_devices)))
    return num_devices.value

def set_log(filename):
    check(libx6.set_log(filename.encode('utf-8')))

def set_logging_level(level):
    check(libx6.set_logging_level(level))

class X6(object):
    def __init__(self):
        super(APS2, self).__init__()
        self.device_id = None
        self.digitizer_mode = 'DIGITIZER'
        self.record_length = 128
        self.waveforms = 1
        self.segments = 1
        self.round_robins = 1

    def __del__(self):
        try:
            self.disconnect()
        except Exception as e:
            pass

    def x6_call(self, name, *args):
        status = getattr(libx6, name)(self.device_id)(*args)
        check(status)

    def x6_getter(self, name, ctype, *args):
        param = ctype()
        self.x6_call(name, *args, byref(param))
        return param.value

    def connect(self, device_id):
        if self.device_id:
            #Disconnect before connecting to avoid dangling connections
            warnings.warn(
                "Disconnecting from {} before connection to {}".format(
                    self.device_id, device_id))
            self.disconnect()
        check(libx6.connect_x6(device_id))
        self.device_id = device_id

    def disconnect(self):
        x6_call("disconnect_x6")
        self.device_id = None

    def get_firmware_version(self):
        version = c_uint32()
        sha = c_uint32()
        timestamp = c_uint32()
        string = create_string_buffer(64)
        check(libx6.get_firmware_version(
            self.device_id, byref(version), byref(sha), byref(timestamp), string))
        return version.value, sha.value, timestamp.value, string.value.decode('ascii')

    def enable_stream(self, a, b, c):
        x6_call("enable_stream", a, b, c)

    def disable_stream(self, a, b, c):
        x6_call("disable_stream", a, b, c)

    def set_nco_frequency(self, a, b, frequency):
        x6_call("set_nco_frequency", a, b, frequency)

    def get_nco_frequency(self, a, b):
        return x6_getter("get_nco_frequency", c_double, a, b)

    def get_record_length(self, a, b, c):
        """
        Get the channel-specific record length (number of samples).

        Note that the channel record length defers from the physical channel
        record length because of decimation in the DSP pipeline.
        """
        ch = Channel(a, b, c)
        return x6_getter("get_record_length", c_uint32, byref(ch))

    def set_threshold(self, a, c, threshold):
        x6_call("set_threshold", a, c, threshold)

    def get_threshold(self, a, c):
        return x6_getter("get_threshold", c_double, a, c)

    def set_threshold_invert(self, a, c, invert):
        x6_call("set_threshold_invert", a, c, invert)

    def get_threshold_invert(self, a, c):
        return x6_getter("get_threshold_invert", c_double, a, c)

    def write_kernel(self, a, b, c, kernel):
        x6_call("write_kernel", a, b, c, kernel, len(kernel))

    def read_kernel(self, a, b, c, offset):
        # for MATLAB compatibility reasons with complex data types, the underlying
        # libx6 is designed to work with an array of length 1
        point = np.zeros(1, dtype=np.complex128)
        x6_call("read_kernel", a, b, c, offset, point)
        return point[0]

    def set_kernel_bias(self, a, b, c, bias):
        x6_call("set_kernel_bias", a, b, c, bias)

    def get_kernel_bias(self, a, b, c):
        point = np.zeros(1, dtype=np.complex128)
        x6_call("get_kernel_bias", a, b, c, point)
        return point[0]

    def acquire(self):
        x6_call("set_averager_settings",
                self.record_length,
                self.segments,
                self.waveforms,
                self.round_robins)
        x6_call("acquire")

    def wait_for_acquisition(self):
        # TODO
        print("not implemented")

    def stop(self):
        x6_call("stop")

    def data_available(self):
        return x6_call("get_num_new_records") > 0

    def transfer_stream(self, a, b, c):
        ch = Channel(a, b, c)
        buffer_size = x6_getter("get_buffer_size", c_uint32, byref(ch), 1)
        # In digitizer mode, resize the buffer to get an integer number of
        # round robins
        if self.digitizer_mode == 'DIGITIZER':
            record_length = self.get_record_length(a, b, c)
            samples_per_RR = record_length * self.waveforms * self.segments;
            buffer_size = samples_per_RR * (buffer_size // samples_per_RR)
        if buffer_size == 0
            return None
        stream = np.zeros(buffer_size, dtype=np.double)
        x6_call("transfer_stream", byref(ch), 1, stream, len(stream))

        if b == 0 and c == 0:
            # physical channels should be returned directly
            return stream
        else:
            # otherwise, the data is complex and interleaved real/imag
            return stream[::2] + 1j*stream[1::2]

    def transfer_variance(self, a, b, c):
        ch = Channel(a, b, c)
        buffer_size = x6_getter("get_variance_buffer_size", c_uint32, byref(ch), 1)
        stream = np.zeros(buffer_size, dtype=np.double)
        x6_call("transfer_variance", byref(ch), 1, stream, len(stream))

        if b == 0 and c == 0:
            # physical channel
            return stream, np.zeros(buffer_size), np.zeros(buffer_size)
        else:
            # interleaved real/imag/prod
            return stream[::3], stream[1::3], stream[2::3]
