% MATLAB wrapper for the X6 driver.
%
% Usage notes: The channelizer creates multiple data streams per physical
% input channel. These are indexed by a 3-parameter label (a,b,c), where a
% is the 1-indexed physical channel, b is the 0-indexed virtual channel
% (b=0 is the raw stream, b>1 are demodulated streams, and c indicates
% demodulated (c=0) or demodulated and integrated (c = 1).

% Original authors: Blake Johnson and Colm Ryan
% Date: August 25, 2014

% Copyright 2014 Raytheon BBN Technologies
%
% Licensed under the Apache License, Version 2.0 (the "License");
% you may not use this file except in compliance with the License.
% You may obtain a copy of the License at
%
%     http://www.apache.org/licenses/LICENSE-2.0
%
% Unless required by applicable law or agreed to in writing, software
% distributed under the License is distributed on an "AS IS" BASIS,
% WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
% See the License for the specific language governing permissions and
% limitations under the License.

classdef X6 < hgsetget

    properties
        samplingRate = 1e9;
        triggerSource
        reference
        digitizerMode
        deviceID = 0;
        enabledStreams = {} %keep track of enabled streams so we can transfer them all
        dataTimer
        nbrSegments
        recordLength
        nbrWaveforms
        nbrRoundRobins
    end

    properties(Access = private)
        lastBufferTimeStamp
    end

    properties(Constant)
        LIBRARY_PATH = '../../build/';
        DSP_WB_OFFSET = [hex2dec('2000'), hex2dec('2100')];
        SPI_ADDRS = containers.Map({'adc0', 'adc1', 'dac0', 'dac1'}, {16, 18, 144, 146});
    end

    events
        DataReady
    end

    methods
        function obj = X6()
            X6.load_library();
            obj.set_debug_level(4);
        end

        function x6_call(obj, func, varargin)
            % Make void call to the library
            status = calllib('libx6', func, obj.deviceID, varargin{:});
            X6.check_status(status);
        end

        function val = x6_getter(obj, func, varargin)
            % Make a getter call to the library passing and returning a pointer
            [status, val] = calllib('libx6', func, obj.deviceID, varargin{:}, 0);
            X6.check_status(status);
        end

        function val = x6_channel_getter(obj, func, varargin)
            % Specialized getter for API's that also take a ChannelTuple pointer
            [status, ~, val] = calllib('libx6', func, obj.deviceID, varargin{:}, 0);
            X6.check_status(status);
        end

        function connect(obj, id)
            if ischar(id)
                id = str2double(id);
            end
            obj.deviceID = id;
            x6_call(obj, 'connect_x6');
            % temporary fix for stream enable register
            obj.write_register(X6.DSP_WB_OFFSET(1), 15, 0);
            obj.write_register(X6.DSP_WB_OFFSET(2), 15, 0);
        end

        function disconnect(obj)
            x6_call(obj, 'disconnect_x6');
        end

        function delete(obj)
            try
                disconnect(obj);
            catch
            end
            if ~isempty(obj.dataTimer)
                delete(obj.dataTimer);
            end
        end

        function init(obj)
            x6_call(obj, 'initX6');
        end

        function val = get.samplingRate(obj)
            val = x6_getter(obj, 'get_sampleRate');
        end

        function val = get.triggerSource(obj)
            val = x6_getter(obj, 'get_trigger_source');
        end

        function set.triggerSource(obj, source)
            x6_call(obj, 'set_trigger_source', source)
        end

        function set.reference(obj, ref)
            %If passed simple `internal` or `external` map to enum
            map = containers.Map({'external', 'internal'}, {'EXTERNAL_REFERENCE', 'INTERNAL_REFERENCE'});
            if isKey(map, ref)
                ref = map(ref);
            end
            x6_call(obj, 'set_reference_source', ref);
        end

        function val = get.reference(obj)
            val = x6_getter(obj, 'get_reference_source');
        end

        function set.digitizerMode(obj, dig_mode)
            x6_call(obj, 'set_digitizer_mode', dig_mode);
        end

        function val = get.digitizerMode(obj)
            val = x6_getter(obj, 'get_digitizer_mode');
        end

        function enable_stream(obj, a, b, c)
            x6_call(obj, 'enable_stream', a, b, c)
            obj.enabledStreams{end+1} = [a,b,c];
        end

        function disable_stream(obj, a, b, c)
            x6_call(obj, 'disable_stream', a, b, c);
            % remove the stream from the enabledStreams list
            idx = find(cellfun(@(x) isequal(x, [a,b,c]), obj.enabledStreams));
            if ~isempty(idx)
                obj.enabledStreams(idx) = [];
            end
        end

        function set_averager_settings(obj, recordLength, nbrSegments, waveforms, roundRobins)
            x6_call(obj, 'set_averager_settings', recordLength, nbrSegments, waveforms, roundRobins);
            obj.recordLength = recordLength;
            obj.nbrSegments = nbrSegments;
            obj.nbrWaveforms = waveforms;
            obj.nbrRoundRobins = roundRobins;
        end

        function acquire(obj)
            x6_call(obj, 'acquire');
            pause(0.75);   % makes sure that the digitizers are ready before starting acquisition
            % Since we cannot easily pass callbacks to the C library to fire
            % on new data arriving we resort to polling on a timer
            % We also fire on stopping to catch any last data
            recsPerRoundRobin = obj.nbrSegments*obj.nbrWaveforms;
            function do_poll(~,~)
                nRecords = x6_getter(obj, 'get_num_new_records');
                if nRecords == 0
                    return
                end
                obj.lastBufferTimeStamp = tic();
                % in digitizer mode, insist on collecting a complete round robin
                if strcmp(obj.digitizerMode, 'DIGITIZER') && nRecords < recsPerRoundRobin
                    return
                end
                notify(obj, 'DataReady');
            end
            % grab initial timestamp for calculating timeout
            obj.lastBufferTimeStamp = tic();
            obj.dataTimer = timer('TimerFcn', @do_poll, 'StopFcn', @(~,~) notify(obj, 'DataReady'), 'Period', 0.1, 'ExecutionMode', 'fixedSpacing');
            start(obj.dataTimer);
        end

        function val = wait_for_acquisition(obj, timeout)
            val = -1;
            while toc(obj.lastBufferTimeStamp) < timeout
                if ~x6_getter(obj, 'get_is_running')
                    val = 0;
                    break
                end
                pause(0.1)
            end
            stop(obj);
        end

        function stop(obj)
            x6_call(obj, 'stop');
            if ~isempty(obj.dataTimer)
                stop(obj.dataTimer);
                delete(obj.dataTimer);
                obj.dataTimer = [];
            end
        end

        function data = transfer_waveform(obj, channel)
            % returns a structure of streams associated with the given
            % channel
            data = struct();
            for stream = obj.enabledStreams
                if stream{1}(1) == channel
                    s = struct('a', stream{1}(1), 'b', stream{1}(2), 'c', stream{1}(3));
                    data.(['s' sprintf('%d',stream{1})]) = obj.transfer_stream(s);
                end
            end
        end

        function wf = transfer_stream(obj, channels)
            % expects channels to be a vector of structs of the form:
            % struct('a', X, 'b', Y, 'c', Z)
            % when passed a single channel struct, returns the corresponding waveform
            % when passed multiple channels, returns the correlation of the channels

            bufSize = x6_channel_getter(obj, 'get_buffer_size', channels, length(channels));
            % In digitizer mode we want integer number of round robins
            recLength = x6_channel_getter(obj, 'get_record_length', channels);
            samplesPerRR = recLength*obj.nbrWaveforms*obj.nbrSegments;
            if strcmp(obj.digitizerMode, 'DIGITIZER')
                bufSize = samplesPerRR * idivide(bufSize, samplesPerRR);
            end

            if bufSize == 0
                wf = [];
                return
            end
            wfPtr = libpointer('doublePtr', zeros(bufSize, 1, 'double'));
            x6_call(obj, 'transfer_stream', channels, length(channels), wfPtr, bufSize);

            if channels(1).b == 0 && channels(1).c == 0 % physical channel
                wf = wfPtr.Value;
            else
                % For complex data real/imag are interleaved
                wf = wfPtr.Value(1:2:end) + 1i*wfPtr.Value(2:2:end);
                recLength = recLength/2;
            end

            if strcmp(obj.digitizerMode, 'DIGITIZER')
                recsPerRoundRobin = obj.nbrWaveforms*obj.nbrSegments;
                rrsPerBuf = length(wf)/recLength/recsPerRoundRobin;
                wf = reshape(wf, recLength, obj.nbrWaveforms, obj.nbrSegments, rrsPerBuf);
            else
                wf = reshape(wf, recLength, obj.nbrSegments);
            end
        end

        function wf = transfer_stream_variance(obj, channels)
            % expects channels to be a vector of structs of the form:
            % struct('a', X, 'b', Y, 'c', Z)
            bufSize = x6_channel_getter(obj, 'get_variance_buffer_size', channels, length(channels));
            wfPtr = libpointer('doublePtr', zeros(bufSize, 1, 'double'));
            x6_call(obj, 'transfer_variance', channels, length(channels), wfPtr, bufSize);

            wf = struct('real', [], 'imag', [], 'prod', []);
            if channels(1).b == 0 && channels(1).c == 0 % physical channel
                wf.real = wfPtr.Value;
                wf.imag = zeros(length(wfPtr.Value), 1);
                wf.prod = zeros(length(wfPtr.Value), 1);
            else
                wf.real = wfPtr.Value(1:3:end);
                wf.imag = wfPtr.Value(2:3:end);
                wf.prod = wfPtr.Value(3:3:end);
            end
            if channels(1).c == 0 % non-results streams should be reshaped
                wf.real = reshape(wf.real, length(wf.real)/obj.nbrSegments, obj.nbrSegments);
                wf.imag = reshape(wf.imag, length(wf.imag)/obj.nbrSegments, obj.nbrSegments);
                wf.prod = reshape(wf.prod, length(wf.prod)/obj.nbrSegments, obj.nbrSegments);
            end
        end

        function write_register(obj, addr, offset, data)
            x6_call(obj, 'write_register', addr, offset, data);
        end

        function val = read_register(obj, addr, offset)
            val = x6_getter(obj, 'read_register', addr, offset);
        end

        function write_spi(obj, chip, addr, data)
            % read flag is low so just address
            val = bitshift(addr, 16) + data;
            obj.write_register(hex2dec('0800'), obj.SPI_ADDRS(chip), val);
        end

        function val = read_spi(obj, chip, addr)
            % read flag is high
            val = bitshift(1, 28) + bitshift(addr, 16);
            obj.write_register(hex2dec('0800'), obj.SPI_ADDRS(chip), val);
            val = int32(obj.read_register(hex2dec('0800'), obj.SPI_ADDRS(chip)+1));
            assert(bitget(val, 32) == 1, 'Oops! Read valid flag was not set!');
        end

        function val = get_logic_temperature(obj)
            val = x6_getter(obj, 'get_logic_temperature');
        end

        function [ver, ver_str] = get_firmware_version(obj)
            [status, ver, git_sha1] = calllib('libx6', 'get_firmware_version', obj.deviceID, 0, 0);
            X6.check_status(status);

            % Create version string
            major_ver = bitand(bitshift(ver, -8), hex2dec('ff'));
            minor_ver = bitand(ver, hex2dec('ff'));
            commits_since = bitand(bitshift(ver, -16), hex2dec('fff'));
            if bitand(bitshift(ver, -28), hex2dec('f')) == hex2dec('d')
                dirty_string = '-dirty';
            else
                dirty_string = '';
            end

            ver_str = sprintf('v%d.%d+%d-%x%s', major_ver, minor_ver, commits_since, git_sha1, dirty_string);
        end

        function set_nco_frequency(obj, a, b, freq)
            x6_call(obj, 'set_nco_frequency', a, b, freq);
        end

        function val = get_nco_frequency(obj, a, b)
            val = x6_getter(obj, 'get_nco_frequency', a, b);
        end

        function write_kernel(obj, a, b, c, kernel)
            % The C library takes a double complex* but we're faking a double* so pack the data manually
            packedKernel = [real(kernel(:))'; imag(kernel(:))'];
            x6_call(obj, 'write_kernel', a, b, c, packedKernel(:), numel(packedKernel)/2);
        end

        function val = read_kernel(obj, a, b, c, addr)
            % The C library takes a double complex* but we're faking a double*
            ptr = libpointer('doublePtr', zeros(2));
            x6_call(obj, 'read_kernel', a, b, c, addr-1, ptr);
            val = ptr.Value(1) + 1i*ptr.Value(2);
        end

        function set_kernel_bias(obj, a, b, c, bias)
            % The C library takes a double complex* but we're faking a double* so pack the data manually
            packed_bias = [real(bias); imag(bias)];
            x6_call(obj, 'set_kernel_bias', a, b, c, packed_bias);
        end

        function val = get_kernel_bias(obj, a, b, c)
            % The C library takes a double complex* but we're faking a double*
            ptr = libpointer('doublePtr', zeros(2));
            x6_call(obj, 'get_kernel_bias', a, b, c, ptr);
            val = ptr.Value(1) + 1i*ptr.Value(2);
        end

        function set_threshold(obj, a, c, threshold)
            x6_call(obj, 'set_threshold', a, c, threshold);
        end

        function val = get_threshold(obj, a, c)
            val = x6_getter(obj, 'get_threshold', a, c);
        end

        function set_threshold_invert(obj, a, c, invert)
            x6_call(obj, 'set_threshold_invert', a, c, invert);
        end

        function val = get_threshold_invert(obj, a, c)
            val = logical(x6_getter(obj, 'get_threshold_invert', a, c));
        end

        function write_pulse_waveform(obj, pg, waveform)
            x6_call(obj, 'write_pulse_waveform', pg, waveform, length(waveform));
        end

        function val = read_pulse_waveform(obj, pg, addr)
            val = x6_getter(obj, 'read_pulse_waveform', pg, addr-1);
        end

        function val = get_input_channel_enable(obj, chan)
            val = x6_getter(obj, 'get_input_channel_enable', chan-1);
        end

        function set_input_channel_enable(obj, chan, enable)
            x6_call(obj, 'set_input_channel_enable', chan-1, enable);
        end

        function val = get_output_channel_enable(obj, chan)
            val = x6_getter(obj, 'get_output_channel_enable', chan-1);
        end

        function set_output_channel_enable(obj, chan, enable)
            x6_call(obj, 'set_output_channel_enable', chan-1, enable);
        end


        % Instrument meta-setter that sets all parameters
        function setAll(obj, settings)
            fields = fieldnames(settings);
            for tmpName = fields'
                switch tmpName{1}
                    case 'horizontal'
                        % Skip for now. Eventually this is where you'd want
                        % to pass thru trigger delay
                    case 'averager'
                        obj.set_averager_settings( ...
                            settings.averager.recordLength, ...
                            settings.averager.nbrSegments, ...
                            settings.averager.nbrWaveforms, ...
                            settings.averager.nbrRoundRobins);
                    case 'channels'
                        for channel = fieldnames(settings.channels)'
                            obj.set_channel_settings( channel{1}, settings.channels.(channel{1}) );
                        end
                    case 'enableRawStreams'
                        if settings.enableRawStreams
                            obj.enable_stream(1, 0, 0);
                            obj.enable_stream(2, 0, 0);
                        end
                    otherwise
                        if ismember(tmpName{1}, methods(obj))
                            feval(['obj.' tmpName{1}], settings.(tmpName{1}));
                        elseif ismember(tmpName{1}, properties(obj))
                            obj.(tmpName{1}) = settings.(tmpName{1});
                        end
                end
            end
        end

        function set_channel_settings(obj, label, settings)
            % channel labels are of the form 'sAB'
            a = str2double(label(2));
            b = str2double(label(3));
            if settings.enableDemodStream
                obj.enable_stream(a, b, 0);
            else
                obj.disable_stream(a, b, 0);
            end
            if settings.enableDemodResultStream
                obj.enable_stream(a, b, 1);
            else
                obj.disable_stream(a, b, 1);
            end
            if settings.enableRawResultStream
                obj.enable_stream(a, 0, b)
            else
                obj.disable_stream(a, 0, b)
            end
            obj.set_nco_frequency(a, b, settings.IFfreq);
            if ~isempty(settings.demodKernel)
                %Try to decode base64 encoded kernels
                if (ischar(settings.demodKernel))
                    tmp = typecast(org.apache.commons.codec.binary.Base64.decodeBase64(uint8(settings.demodKernel)), 'uint8');
                    tmp = typecast(tmp, 'double');
                    settings.demodKernel = tmp(1:2:end) + 1j*tmp(2:2:end);
                end
                obj.write_kernel(a, b, 1, settings.demodKernel);
            end
            if ~isempty(settings.demodKernelBias)
                %Try to decode base64 encoded kernels
                if (ischar(settings.demodKernelBias))
                    tmp = typecast(org.apache.commons.codec.binary.Base64.decodeBase64(uint8(settings.demodKernelBias)), 'uint8');
                    tmp = typecast(tmp, 'double');
                    settings.demodKernelBias = tmp(1) + 1j*tmp(2);
                end
                obj.write_kernel(a, b, 1, settings.demodKernelBias);
            else
                set_kernel_bias(obj, a, b, 1, 0)
            end
            if ~isempty(settings.rawKernel)
                %Try to decode base64 encoded kernels
                if (ischar(settings.rawKernel))
                    tmp = typecast(org.apache.commons.codec.binary.Base64.decodeBase64(uint8(settings.rawKernel)), 'uint8');
                    tmp = typecast(tmp, 'double');
                    settings.rawKernel = tmp(1:2:end) + 1j*tmp(2:2:end);
                end
                obj.write_kernel(a, 0, b, settings.rawKernel);
            end
            if ~isempty(settings.rawKernelBias)
                %Try to decode base64 encoded kernels
                if (ischar(settings.rawKernelBias))
                    tmp = typecast(org.apache.commons.codec.binary.Base64.decodeBase64(uint8(settings.rawKernelBias)), 'uint8');
                    tmp = typecast(tmp, 'double');
                    settings.rawKernelBias = tmp(1) + 1j*tmp(2);
                end
                obj.write_kernel(a, 0, b, settings.rawKernelBias);
            else
                set_kernel_bias(obj, a, b, 0, 0)
            end
            obj.set_threshold(a, b, settings.threshold);
            obj.set_threshold_invert(a, b, settings.thresholdInvert);
        end
    end

    methods (Static)

        function load_library()
            % Helper functtion to load the platform dependent library
            switch computer()
                case 'PCWIN64'
                    libfname = 'libx6.dll';
                    libheader = 'libx6.matlab.h';
                    %protoFile = @obj.libx6;
                otherwise
                    error('Unsupported platform.');
            end
            % build library path and load it if necessary
            if ~libisloaded('libx6')
                myPath = fileparts(mfilename('fullpath'));
                [~,~] = loadlibrary(fullfile(myPath, X6.LIBRARY_PATH, libfname), fullfile(myPath, libheader));
            end
        end

        function check_status(status)
            X6.load_library();
            assert(strcmp(status, 'X6_OK'),...
                'X6:Fail',...
                'X6 library call failed with status: %s', calllib('libx6', 'get_error_msg', status));
        end

        function val = num_devices()
            X6.load_library();
            [status, val]  = calllib('libx6', 'get_num_devices', 0);
            X6.check_status(status);
        end


        function set_debug_level(level)
            % sets logging level in libx6.log
            % level = {logERROR=0, logWARNING, logINFO, logDEBUG, logDEBUG1, logDEBUG2, logDEBUG3, logDEBUG4}
            calllib('libx6', 'set_logging_level', level);
        end

        function UnitTest()

            fprintf('BBN X6-1000 Test Executable\n')

            x6 = X6();

            x6.set_debug_level(4);

            x6.connect(0);

            x6.init();

            fprintf('current logic temperature = %.1f\n', x6.get_logic_temperature());

            fprintf('current PLL frequency = %.2f GHz\n', x6.samplingRate/1e9);
            fprintf('Setting clock reference to external\n');
            % x6.reference = 'EXTERNAL_REFERENCE';

            fprintf('Enabling streams\n');
            numDemodChan = 1;
            numMatchFilters = 2; % 4
            for phys = 1:2
                x6.enable_stream(phys, 0, 0); % the raw stream
                x6.enable_stream(phys, 1, 0); % the demod stream
                for demod = 1:numMatchFilters
                    x6.enable_stream(phys, demod, 1);
                end
            end

            fprintf('Setting NCO phase increments\n');
            x6.set_nco_frequency(1, 1, 10e6);
            x6.set_nco_frequency(2, 1, 20e6);

            fprintf('setting averager parameters to record 16 segments of 2048 samples\n');
            x6.set_averager_settings(2048, 64, 1, 20);

            % write a waveform into transmitter memory
            x6.write_pulse_waveform(0, linspace(0,0.99,2000));
            x6.set_output_channel_enable(1, true);
            x6.set_output_channel_enable(2, true);

            %DAC trigger window
            fprintf('Acquiring\n');
            x6.acquire();

            pause(0.5);
            dec2hex(x6.read_register(hex2dec('800'), hex2dec('98')), 8)

            success = x6.wait_for_acquisition(20);
            fprintf('Wait for acquisition returned %d\n', success);

            fprintf('Stopping\n');
            x6.stop();

            x6.disconnect();

            unloadlibrary('libx6')
        end

    end

end
