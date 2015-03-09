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
channel 2). The second index represents the demodulated virtual channel (`b = 0`
is the raw stream, `b = 1` and higher for the demodulated channels). The last
index selects between the demodulated data and the integrated data. Some example channel tuples:

* (1,0,0) - raw data from physical channel 1
* (1,2,0) - demodulated data from virtual channel 2 on physical channel 1
* (1,2,1) - result data from virtual channel 2 on physical channel 1

The current firmware provides two DDCs per physical channel. Therefore, there
are 10 available streams (2 raw physical, 4 demod, and 4 result).

Do these things to acquire data with the card:

* Set the averager settings just like you would with an Acquiris or Alazar
  digitizer
* Enable the streams that you want to capture with the host PC
* Set the NCO frequency (IF frequency) for each enabled virtual stream
* Provide an integration kernel for each *result* stream
* Optionally set integration thresholds for hard-decisions on fast digital lines


## Gotchas, limitations, etc.

* For record lengths > 4000, you must disable the raw streams. These packets are
  too large for the VITA mover to handle, so after sending the first packet or
  two, no data will be sent to the computer unless you disable raw streams.

* Result streams require a kernel with a length shorter than the demod stream.
  The BBN firmware runs with a fixed decimation factor of 16. So, for a record
  length of 1024, you must provide a kernel with length < 64.


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
	x6.enable_stream(phys, 0, 0); % the raw stream
	for demod = 1:2
		for result = 0:1
			x6.enable_stream(phys, demod, result);
		end
	end
end

% set NCO frequencies
x6.set_nco_frequency(1, 1, 10e6); % for channels (1,1,0) and (1,1,1)
x6.set_nco_frequency(1, 2, 30e6);
x6.set_nco_frequency(2, 1, 20e6);
x6.set_nco_frequency(2, 2, 40e6);

% write integration kernels
x6.write_kernel(1, 1, ones(100,1)); % for channel (1,1,1)
x6.write_kernel(1, 2, ones(100,1));
x6.write_kernel(2, 1, ones(100,1));
x6.write_kernel(2, 2, ones(100,1));

% write decision thresholds
x6.set_threshold(1, 1, 0.5);
x6.set_threshold(1, 2, 0.5);
x6.set_threshold(2, 1, 0.5);
x6.set_threshold(2, 2, 0.5);

% set averager settings (2048 record length x 10 segments x 1 waveform x 1000 round robins)
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
			'enableResultStream', true,
			'kernel', ones(100,1), % may alternatively supply a base64 encoded string of kernel
			'threshold', 0.5
		),
		's21', struct(
			'IFfreq', 10e6,
			'enableDemodStream', true,
			'enableResultStream', true,
			'kernel', ones(100,1),
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

`connect(int ID)`

Connect to X6 with the given device ID. For a single X6 card, ID = 0.

`disconnect(int ID)`

Disconnect from the X6 card.

`set_reference(int ID, int referenceSource)`

Set the reference clock source to `EXTERNAL = 0` or `INTERNAL = 1`.

`enable_stream(int ID, int a, int b, int c)`

Enable stream (a,b,c).

`disable_stream(int ID, int a, int b, int c)`

Disable stream (a,b,c).

`set_averager_settings(int ID, int recordLength, int segments, int waveforms, int roundrobins)`

`set_nco_frequency(int ID, int a, int b, double frequency)`

Set the NCO/IF frequency for channels (a,b,0) and (a,b,1).

`write_kernel(int ID, int a, int b, double *kernel, unsigned bufsize)`

Transfer integration kernel for channel (a,b,1) to the X6. `kernel` is expected
to have interleaved real and imaginary data in the range [-1, 1].

`set_threshold(int ID, int a, int b, double threshold)`

Sets the decision engine threshold for channel (a,b,1).

`acquire(int ID)`

`wait_for_acquisition(int ID, int timeout)`

Blocks execution in the caller until finished acquiring data, or `timeout`
seconds have elapsed.

`stop(int ID)`

`transfer_waveform(int ID, struct ChannelTuple *channels, int numChannels, double *buffer, unsigned bufsize)`

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

Like `transfer_waveform` but returns the variance of the corresponding
stream(s). For non-physical streams, the variance has three components: real,
imaginary, and the real-imaginary product. `buffer` is filled in triples, e.g.:
`[d1_r, d1_i, d1_p, d2_r, d2_i, d2_p, ...]`.
