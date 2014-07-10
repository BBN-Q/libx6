classdef X6 < hgsetget
    
    properties (Constant)
        library_path = '../build/';
    end
    
    properties
        samplingRate = 1000;
        triggerSource
        reference
        bufferSize = 0;
        is_open = 0;
        deviceID = 0;
    end
    
    properties(Constant)
        DECIM_FACTOR = 4;
        DSP_WB_OFFSET = [hex2dec('2000'), hex2dec('2100')];
    end
    
    methods
        function obj = X6()
            obj.load_library();
        end

        function val = connect(obj, id)
            val = calllib('libx6adc', 'connect_by_ID', id);
            if (val == 0)
                obj.is_open = 1;
                obj.deviceID = id;
            end
        end

        function val = disconnect(obj)
            val = obj.libraryCall('disconnect');
            obj.is_open = 0;
        end

        function delete(obj)
            if (obj.is_open)
                obj.disconnect();
            end
        end

        function val = num_devices(obj)
            val = calllib('libx6adc', 'get_num_devices');
        end

        function val = init(obj)
            val = obj.libraryCall('initX6');
        end

        function val = get.samplingRate(obj)
            val = obj.libraryCall('get_sampleRate');
        end

        function set.samplingRate(obj, rate)
            val = obj.libraryCall('set_sampleRate', rate);
        end

        function val = get.triggerSource(obj)
            val = obj.libraryCall('get_trigger_source');
        end

        function set.triggerSource(obj, source)
            obj.libraryCall('set_trigger_source', source);
        end
        
        function set.reference(obj, reference)
            valMap = containers.Map({'int', 'internal', 'ext', 'external'}, {0, 0, 1, 1});
            obj.libraryCall('set_reference', valMap(lower(reference)));
        end
        
        function val = get.reference(obj)
            val = obj.libraryCall('get_reference_source');
        end
        
        function val = enable_stream(obj, a, b, c)
            val = obj.libraryCall('enable_stream', a, b, c);
        end
        
        function val = disable_stream(obj, a, b, c)
            val = obj.libraryCall('disable_stream', a, b, c);
        end
        
        function val = set_averager_settings(obj, recordLength, numSegments, waveforms, roundRobins)
            val = obj.libraryCall('set_averager_settings', recordLength, numSegments, waveforms, roundRobins);
            obj.bufferSize = recordLength * numSegments;
        end

        function val = acquire(obj)
            val = obj.libraryCall('acquire');
        end

        function val = wait_for_acquisition(obj, timeout)
            val = obj.libraryCall('wait_for_acquisition', timeout);
        end

        function val = stop(obj)
            val = obj.libraryCall('stop');
        end

        function wf = transfer_waveform(obj, a, b, c)
            if b == 0 % physical
                bufSize = obj.bufferSize;
            elseif c ~= 0 % result
                bufSize = 2;
            else % demod
                bufSize = 2*obj.bufferSize/obj.DECIM_FACTOR;
            end
            wfPtr = libpointer('doublePtr', zeros(bufSize, 1, 'double'));
            success = obj.libraryCall('transfer_waveform', a, b, c, wfPtr, bufSize);
            assert(success == 0, 'transfer_waveform failed');

            if b == 0 || c ~= 0 % physical or result channel
                wf = wfPtr.Value;
            else
                wf = wfPtr.Value(1:2:end) +1i*wfPtr.Value(2:2:end);
            end
        end
        
        function val = writeRegister(obj, addr, offset, data)
            % get temprature using method one based on Malibu Objects
            val = obj.libraryCall('write_register', addr, offset, data);
        end

        function val = readRegister(obj, addr, offset)
            % get temprature using method one based on Malibu Objects
            val = obj.libraryCall('read_register', addr, offset);
        end

        function val = getLogicTemperature(obj)
            % get temprature using method one based on Malibu Objects
            val = obj.libraryCall('get_logic_temperature', 0);
        end
        
        function write_kernel(obj, phys, demod, kernel)
            obj.writeRegister(obj.DSP_WB_OFFSET(phys), 24+demod-1, length(kernel));
            kernel = int32(kernel * (2^15-1)); % scale up to integers
            for ct = 1:length(kernel)
                obj.writeRegister(obj.DSP_WB_OFFSET(phys), 48+2*(demod-1), ct-1);
                obj.writeRegister(obj.DSP_WB_OFFSET(phys), 48+2*(demod-1)+1, bitshift(real(kernel(ct)), 16) + bitand(imag(kernel(ct)), hex2dec('FFFF')));
            end
        end
    end
    
    methods (Access = protected)
        % overide APS load_library
        function load_library(obj)
            %Helper functtion to load the platform dependent library
            switch computer()
                case 'PCWIN64'
                    libfname = 'libx6adc.dll';
                    libheader = '../src/libx6adc.matlab.h';
                    %protoFile = @obj.libaps64;
                otherwise
                    error('Unsupported platform.');
            end
            % build library path and load it if necessary
            if ~libisloaded('libx6adc')
                loadlibrary(fullfile(obj.library_path, libfname), libheader );
            end
        end

        function val = libraryCall(obj,func,varargin)
            %Helper function to pass through to calllib with the X6 device ID first 
            if ~(obj.is_open)
                error('X6:libraryCall','X6 is not open');
            end
                        
            if size(varargin,2) == 0
                val = calllib('libx6adc', func, obj.deviceID);
            else
                val = calllib('libx6adc', func, obj.deviceID, varargin{:});
            end
        end
    end
    
    methods (Static)
        
        function setDebugLevel(level)
            % sets logging level in libx6.log
            % level = {logERROR=0, logWARNING, logINFO, logDEBUG, logDEBUG1, logDEBUG2, logDEBUG3, logDEBUG4}
            calllib('libx6adc', 'set_logging_level', level);
        end
        
        function UnitTest()
            
            fprintf('BBN X6-1000 Test Executable\n')
            
            x6 = X6();
            
            x6.setDebugLevel(6);
            
            x6.connect(0);
            
            if (~x6.is_open)
                error('Could not open aps')
            end

            fprintf('current logic temperature = %.1f\n', x6.getLogicTemperature());
            
            fprintf('current PLL frequency = %.2f GHz\n', x6.samplingRate/1e9);
            fprintf('Setting clock reference to external\n');
            x6.reference = 'external';
            
            fprintf('Enabling streams\n');
            for phys = 1:2
                for demod = 0:2
                    if demod > 0
                        for result = 0:1
                            x6.enable_stream(phys, demod, result);
                        end
                    else
                        x6.enable_stream(phys, demod, 0);
                    end
                end
            end
            
            fprintf('Setting NCO phase increments\n');
            phase_increment = 4/100;
            x6.writeRegister(X6.DSP_WB_OFFSET(1), 16,   round(2 * phase_increment * 2^18));
            x6.writeRegister(X6.DSP_WB_OFFSET(1), 16+1, round(2 * phase_increment * 2^18));
            x6.writeRegister(X6.DSP_WB_OFFSET(2), 16,   round(2 * phase_increment * 2^18));
            x6.writeRegister(X6.DSP_WB_OFFSET(2), 16+1, round(2 * phase_increment * 2^18));
            
            %write stream IDs
            fprintf('Setting stream IDs\n');
            for ct = 0:4
                x6.writeRegister(X6.DSP_WB_OFFSET(1), 32+ct, hex2dec('00020100') + 16*ct);
                x6.writeRegister(X6.DSP_WB_OFFSET(2), 32+ct, hex2dec('00020200') + 16*ct);
            end
            
            fprintf('Writing integration kernels\n');
            x6.write_kernel(1, 1, ones(128,1));
            x6.write_kernel(1, 2, ones(128,1));
            x6.write_kernel(2, 1, ones(128,1));
            x6.write_kernel(2, 2, ones(128,1));
            
            fprintf('Writing decision engine thresholds\n');
            x6.writeRegister(X6.DSP_WB_OFFSET(1), 56, int32(4000));
            x6.writeRegister(X6.DSP_WB_OFFSET(2), 56, int32(4000));
            x6.writeRegister(X6.DSP_WB_OFFSET(1), 56+1, int32(4000));
            x6.writeRegister(X6.DSP_WB_OFFSET(2), 56+1, int32(4000));
            
            fprintf('setting averager parameters to record 10 segments of 2048 samples\n');
            x6.set_averager_settings(2048, 9, 1, 1);

            fprintf('Acquiring\n');
            x6.acquire();

            success = x6.wait_for_acquisition(1);
            fprintf('Wait for acquisition returned %d\n', success);

            fprintf('Stopping\n');
            x6.stop();

            fprintf('Transferring waveforms\n');
            numDemodChan = 2;
            wfs = cell(numDemodChan+1,1);
            for ct = 0:numDemodChan
                wfs{ct+1} = x6.transfer_waveform(1,ct,0);
            end
            figure();
            subplot(numDemodChan+1,1,1);
            plot(wfs{1});
            title('Raw Channel 1');

            for ct = 1:numDemodChan
                subplot(numDemodChan+1,1,ct+1);
                plot(real(wfs{ct+1}), 'b');
                hold on
                plot(imag(wfs{ct+1}), 'r');
                title(sprintf('Virtual Channel %d',ct));
            end
            
            
            for ct = 0:numDemodChan
                wfs{ct+1} = x6.transfer_waveform(2,ct,0);
            end
            figure();
            subplot(numDemodChan+1,1,1);
            plot(wfs{1});
            title('Raw Channel 2');

            for ct = 1:numDemodChan
                subplot(numDemodChan+1,1,ct+1);
                plot(real(wfs{ct+1}), 'b');
                hold on
                plot(imag(wfs{ct+1}), 'r');
                title(sprintf('Virtual Channel %d',ct));
            end

            x6.disconnect();
            unloadlibrary('libx6adc')
        end
        
    end
    
end