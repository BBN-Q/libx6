# Python wrapper for the X6 driver.
#
# Usage notes: The channelizer creates multiple data streams per physical
# input channel. These are indexed by a 3-parameter label (a,b,c), where a
# is the 1-indexed physical channel, b is the 0-indexed virtual channel
# (b=0 is the raw stream, b>1 are demodulated streams, and c indicates
# demodulated (c=0) or demodulated and integrated (c = 1).

# Original author: Blake Johnson
# Date: December 1, 2016
# Copyright 2016 Raytheon BBN Technologies

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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

# enums and structs
class Channel(Structure):
    _fields_ = [("a", c_int32),
                ("b", c_int32),
                ("c", c_int32)]

# reference source
reference_dict = {0: "external", 1: "internal"}
EXTERNAL = 0
INTERNAL = 1

# digitizer mode
mode_dict = {0: "digitizer", 1: "averager"}
DIGITIZER = 0
AVERAGER = 1

# supply argument types for libx6 methods

libx6.connect_x6.argtypes              = [c_int32]
libx6.disconnect_x6.argtypes           = [c_int32]
libx6.get_num_devices.argtypes         = [POINTER(c_uint32)]
libx6.get_firmware_version.argtypes    = [c_int32] + [POINTER(c_uint32)]*3 + [c_char_p]

# these take an enum argument, which we will pretend is just a c_uint32
libx6.set_reference_source.argtypes    = [c_int32, c_uint32]
libx6.get_reference_source.argtypes    = [c_int32, POINTER(c_uint32)]
libx6.set_digitizer_mode.argtypes      = [c_int32, c_uint32]
libx6.get_digitizer_mode.argtypes      = [c_int32, POINTER(c_uint32)]

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
libx6.get_is_running.argtypes          = [c_int32, POINTER(c_bool)]
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
attributes = [a for a in dir(libx6) if not a.startswith('_')]
for a in attributes:
    getattr(libx6, a).restype = c_int32

libx6.get_error_msg.argtypes = [c_uint32]
libx6.get_error_msg.restype  = c_char_p

def get_error_msg(status_code):
    return libx6.get_error_msg(status_code).decode("utf-8")

def check(status_code):
    if status_code is 0:
        return
    else:
        raise Exception("LIBX6 Error: {} - {}".format(status_code,
            get_error_msg(status_code)))

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
        super(X6, self).__init__()
        self.device_id = None
        self.record_length = 128
        self.nbr_waveforms = 1
        self.nbr_segments = 1
        self.nbr_round_robins = 1

    def __del__(self):
        try:
            self.disconnect()
        except Exception as e:
            pass

    def x6_call(self, name, *args):
        status = getattr(libx6, name)(self.device_id, *args)
        check(status)

    def x6_getter(self, name, *args):
        # the value we are fetching is always passed as a pointer as the last
        # argument to the method
        pointer_type = getattr(libx6, name).argtypes[-1]
        param = pointer_type._type_()
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
        self.x6_call("disconnect_x6")
        self.device_id = None

    def get_firmware_version(self):
        version = c_uint32()
        sha = c_uint32()
        timestamp = c_uint32()
        string = create_string_buffer(64)
        check(libx6.get_firmware_version(
            self.device_id, byref(version), byref(sha), byref(timestamp), string))
        return version.value, sha.value, timestamp.value, string.value.decode('ascii')

    def set_reference_source(self, source):
        self.x6_call("set_reference_source", int(source))

    def get_reference_source(self):
        source = self.x6_getter("get_reference_source")
        return reference_dict[source]

    reference = property(get_reference_source, set_reference_source)

    def set_acquire_mode(self, mode):
        self.x6_call("set_digitizer_mode", int(mode))

    def get_acquire_mode(self):
        mode = self.x6_getter("get_digitizer_mode")
        return mode_dict[mode]

    acquire_mode = property(get_acquire_mode, set_acquire_mode)

    def enable_stream(self, a, b, c):
        self.x6_call("enable_stream", a, b, c)

    def disable_stream(self, a, b, c):
        self.x6_call("disable_stream", a, b, c)

    def set_nco_frequency(self, a, b, frequency):
        self.x6_call("set_nco_frequency", a, b, frequency)

    def get_nco_frequency(self, a, b):
        return self.x6_getter("get_nco_frequency", a, b)

    def get_record_length(self, a, b, c):
        """
        Get the channel-specific record length (number of samples).

        Note that the channel record length differs from the physical channel
        record length because of decimation in the DSP pipeline.
        """
        ch = Channel(a, b, c)
        return self.x6_getter("get_record_length", byref(ch))

    def set_threshold(self, a, c, threshold):
        self.x6_call("set_threshold", a, c, threshold)

    def get_threshold(self, a, c):
        return self.x6_getter("get_threshold", a, c)

    def set_threshold_invert(self, a, c, invert):
        self.x6_call("set_threshold_invert", a, c, invert)

    def get_threshold_invert(self, a, c):
        return self.x6_getter("get_threshold_invert", a, c)

    def write_kernel(self, a, b, c, kernel):
        self.x6_call("write_kernel", a, b, c, kernel, len(kernel))

    def read_kernel(self, a, b, c, offset):
        # for MATLAB compatibility reasons with complex data types, the underlying
        # libx6 is designed to work with an array of length 1
        point = np.zeros(1, dtype=np.complex128)
        self.x6_call("read_kernel", a, b, c, offset, point)
        return point[0]

    def set_kernel_bias(self, a, b, c, bias):
        point = np.array([bias], dtype=np.complex128)
        self.x6_call("set_kernel_bias", a, b, c, point)

    def get_kernel_bias(self, a, b, c):
        point = np.zeros(1, dtype=np.complex128)
        self.x6_call("get_kernel_bias", a, b, c, point)
        return point[0]

    def acquire(self):
        self.x6_call("set_averager_settings",
                self.record_length,
                self.segments,
                self.waveforms,
                self.round_robins)
        self.x6_call("acquire")

    def wait_for_acquisition(self, timeout):
        self.x6_call("wait_for_acquisition", timeout)

    def stop(self):
        self.x6_call("stop")

    def get_num_new_records(self):
        return self.x6_getter("get_num_new_records")

    def get_is_running(self):
        return self.x6_getter("get_is_running")

    def transfer_stream(self, a, b, c):
        ch = Channel(a, b, c)
        buffer_size = self.x6_getter("get_buffer_size", byref(ch), 1)
        # In digitizer mode, resize the buffer to get an integer number of
        # round robins
        if self.get_digitizer_mode() == 'digitizer':
            record_length = self.get_record_length(a, b, c)
            samples_per_RR = record_length * self.waveforms * self.segments;
            buffer_size = samples_per_RR * (buffer_size // samples_per_RR)
        if buffer_size == 0:
            return None
        stream = np.zeros(buffer_size, dtype=np.double)
        self.x6_call("transfer_stream", byref(ch), 1, stream, len(stream))

        if b == 0 and c == 0:
            # physical channels should be returned directly
            return stream
        else:
            # otherwise, the data is complex and interleaved real/imag
            return stream[::2] + 1j*stream[1::2]

    def transfer_variance(self, a, b, c):
        ch = Channel(a, b, c)
        buffer_size = self.x6_getter("get_variance_buffer_size", byref(ch), 1)
        stream = np.zeros(buffer_size, dtype=np.double)
        self.x6_call("transfer_variance", byref(ch), 1, stream, len(stream))

        if b == 0 and c == 0:
            # physical channel
            return stream, np.zeros(buffer_size), np.zeros(buffer_size)
        else:
            # interleaved real/imag/prod
            return stream[::3], stream[1::3], stream[2::3]
