# Usage Notes

This driver provides interfaces to BBN's custom firmware running on the II
X6-1000M. This document attempts to answer basic questions about use of this
firmware for multiplexed qubit readout.

The BBN firmware analyzes multiple channels per physical channel, and there are
three points where the user may "tap off" each data stream. These tap points
correspond to the raw physical channel data (raw stream), the demodulated data
from the digital downconverter (demod stream), and the kernel integrated data
from the decision engine prior to thresholding (result stream). We introduce a
channel tuple `(a,b,c)` to identify each of these streams. In this tuple, the
first index represents the physical channel (`a = 1` for channel 1, `a = 2` for
channel 2). The second and third index together select a stream from the
physical channel. When `b = 0` and `c = 0`, you get the "raw data" (decimated by
4) from the physical channel. When `b = 1` and `c = 0` you get the demodulated
data (decimated by 32). When `b = 1` and `c = 1`, you get demodulated and
integrated data. However, this last stream has sufficiently large latency to not
be very useful in feedback scenarios. Consequently, when `b = 0` and `c = 1` or
`c = 2` you select kernel integrators that bypass the demodulator. The complete
set of streams for physical channel 1 provided by the current firmware are:

* (1,0,0) - raw data
* (1,1,0) - demodulated data from DSP channel 1
* (1,1,1) - result data from demodulator + kernel integrator on DSP channel 1
* (1,0,1) - result data without demodulator from DSP channel 1 kernel integrator
* (1,0,2) - result data without demodulator from DSP channel 2 kernel integrator

These last two streams are connected to the digital I/O pins after thresholding
to enable fast feedback experiments. Physical channel 2 provides an equivalent
set of streams with `a = 2`. Future firmware versions will likely add more
result streams to allow for a great degree of multiplexing.

Do these things to acquire data with the card:

* Set the capture settings just like you would with an Alazar digitizer
* Enable the streams that you want to capture with the host PC
* Set the NCO frequency (IF frequency) for each enabled demodulator stream
* Provide an integration kernel for each *result* stream
* Optionally set integration thresholds for hard-decisions on fast digital lines


## Gotchas, limitations, etc.

* For record lengths > 4000, you must disable the raw streams. These packets are
  too large for the VITA mover to handle, so after sending the first packet or
  two, no data will be sent to the computer unless you disable raw streams.

* The kernel integrators with and without the demodulator operate on data with
  different decimation factors (the demodulator decimates by 8). A `(1,0,1)`
  stream kernel has a max length of `record_length / 4`, while a `(1,1,1)`
  stream kernel has a max length of `record_length / 32`.

* The `(1,0,1)` and `(1,0,2)` kernel integrators act on a real data stream.
  Therefore, the output of these channels will be purely-real if the kernel is
  purely-real. The `(1,1,1)` kernel acts on a complex data stream from the
  demodulator; consequently, this stream typically contains both real and
  imaginary components regardless of the kernel.

* The socket API only works when the card is in `digitizer` mode. In `averager`
  mode, no data will be sent over the socket.

## MATLAB usage

You can either make individual calls to the appropriate methods, or get
everything "at once" by passing a settings struct to `setAll()`.

### Individual methods example

```matlab
x6 = X6();
x6.connect(0);
x6.reference = 'external';

% enable all streams
for phys = 1:2
  % the raw stream
  x6.enable_stream(phys, 0, 0);
  % demod stream
  x6.enable_stream(phys, 1, 0);
  % demod + integrator stream
  x6.enable_stream(phys, 1, 1);
  % integrators without demod streams
  x6.enable_stream(phys, 0, 1);
  x6.enable_stream(phys, 0, 2);
end

% set NCO frequencies
x6.set_nco_frequency(1, 1, 10e6); % for channels (1,1,0) and (1,1,1)
x6.set_nco_frequency(2, 1, 20e6); % for channels (2,1,0) and (2,1,1)

% write integration kernels
x6.write_kernel(1, 1, 1, ones(64,1));  % max length of 2048 / 32 = 64
x6.write_kernel(1, 0, 1, ones(512,1)); % max length of 2048 / 4 = 512
x6.write_kernel(1, 0, 2, ones(512,1));
x6.write_kernel(2, 1, 1, ones(64,1));
x6.write_kernel(2, 0, 1, ones(512,1));
x6.write_kernel(2, 0, 2, ones(512,1));

% write decision thresholds
x6.set_threshold(1, 1, 0.5); % for stream (1,0,1)
x6.set_threshold(1, 2, 0.5); % for stream (1,0,2)
x6.set_threshold(2, 1, 0.5);
x6.set_threshold(2, 2, 0.5);

% set capture settings (2048 record length x 10 segments x 1 waveform x 1000 round robins)
x6.set_averager_settings(2048, 10, 1, 1000);

% then later...
x6.acquire();
x6.wait_for_acquisition(10);
x6.stop();

% and download data with, e.g. for (1,1,0)
wf = x6.transfer_stream(struct('a', 1, 'b', 1, 'c', 0));
```

### SetAll example

The settings structure groups together the virtual channel information into
groups labeled by first two indices in the channel tuple (i.e. `a` and `b` in
`(a,b,c)`).

```matlab
settings = struct(
	'address', '0',
	'deviceName', 'X6',
	'reference', 'external',
	'averager', struct(
		'recordLength', 2048,
		'nbrSegments', 10,
		'nbrWaveforms', 1,
		'nbrRoundRobins', 1000)
	),
	'enableRawStreams', false,
	'channels', struct(
		's11', struct(
			'IFfreq', 10e6,
			'enableDemodStream', true,
			'enableDemodResultStream', false,
			'enableRawResultStream', true,
			'rawKernel', ones(100,1), % may alternatively supply a base64 encoded string of kernel
			'threshold', 0.5
		),
		's21', struct(
			'IFfreq', 10e6,
			'enableDemodStream', true,
      'enableDemodResultStream', false,
			'enableRawResultStream', true,
			'rawKernel', ones(100,1),
			'threshold', 0.5
		)
		% and so on for other channels...
	)
);
x6 = X6();
x6.connect(0);
x6.setAll(settings);

% then continue as above
x6.acquire();
% ...

```

## C API

Most methods take the device ID as the first argument.

`connect_x6(int ID)`

Connect to X6 with the given device ID. For a single X6 card, ID = 0.

`disconnect_x6(int ID)`

Disconnect from the X6 card.

`set_reference(int ID, int REFERENCE_SOURCE)`

Set the reference clock source to `EXTERNAL = 0` or `INTERNAL = 1`.

`enable_stream(int ID, int a, int b, int c)`

Enable stream (a,b,c).

`disable_stream(int ID, int a, int b, int c)`

Disable stream (a,b,c).

`set_averager_settings(int ID, int recordLength, int segments, int waveforms, int roundrobins)`

`set_nco_frequency(int ID, int a, int b, double frequency)`

Set the NCO/IF frequency for channels (a,b,0) and (a,b,1).

`write_kernel(int ID, int a, int b, int c, double *kernel, unsigned bufsize)`

Transfer integration kernel for channel (a,b,c) to the X6. `kernel` is expected
to have interleaved real and imaginary data in the range [-1, 1].

`set_threshold(int ID, int a, int c, double threshold)`

Sets the decision engine threshold for channel (a,0,c).

`acquire(int ID)`

`wait_for_acquisition(int ID, int timeout)`

Blocks execution in the caller until finished acquiring data, or `timeout`
seconds have elapsed.

`stop(int ID)`

`transfer_stream(int ID, struct ChannelTuple *channels, int numChannels, double *buffer, unsigned bufsize)`

The `ChannelTuple` struct is defined as follows:
```C
struct ChannelTuple {
  int a;
  int b;
  int c;
}
```

You pass an array of such structs as well a buffer, and the driver will fill
`buffer` with the data from the corresponding channel(s). When passed a single
`ChannelTuple` you get the data from that stream. When passed multiple
`ChannelTuple`s, you get the correlated values of those data streams. Currently,
correlations are only computed for result streams.

For raw streams, buffer is simply filled with the scaled physical data. For
demod or result streams, the data is interleaved real/imaginary every other
point.

`transfer_variance(int ID, ChannelTuple *channels, int numChannels, double *buffer, unsigned bufsize)`

Like `transfer_stream` but returns the variance of the corresponding
stream(s). For non-physical streams, the variance has three components: real,
imaginary, and the real-imaginary product. `buffer` is filled in triples, e.g.:
`[d1_r, d1_i, d1_p, d2_r, d2_i, d2_p, ...]`.

`register_socket(int ID, ChannelTuple *channel, int32_t socket)`

Registers file descriptor / handle `socket` to send data for the stream
indicated by `channel`.

## Transferring data

libx6 provides two different methods for transferring data off of the card. The
first is a polling mechanism via the `get_num_new_records()` API call which
returns a non-zero result when new data has arrived. The caller may subsequently
use `transfer_stream()` get fetch that new data. This approach is used in our
MATLAB wrapper to trigger a `DataReady` event when data arrives.

The second approach is to provide libx6 with one half of a locally connected
socket pair for each enabled stream. On posix systems, you can create such
sockets using the system `socketpair()` method from `sys/socket.h`. No such
native method exists on windows; however both cygwin and python gloss over this
difference, allowing users to ignore posix/windows differences. You pass one of
these sockets to libx6 with `register_socket()`. Then captured data is sent over
the socket via a simple `[msg_size data]` wire protocol, where `msg_size` is a
`uint64_t` indicating the data size in bytes.
