classdef TestX6 < matlab.unittest.TestCase

    properties
        x6
    end

    methods(TestClassSetup)

        function setup_x6(testCase)
            testCase.x6 = X6();
            connect(testCase.x6, 0);
        end

    end

    methods(TestClassTeardown)

        function takedown_x6(testCase)
            disconnect(testCase.x6);
            testCase.x6 = [];
            unloadlibrary('libx6adc');
        end

    end

    methods(Test)

        function test_temperature(testCase)
            %Check temperature is between 20C and 80C
            T = testCase.x6.getLogicTemperature();
            verifyTrue(testCase, (T > 20) && (T < 80))
        end

        function test_wishbone_readwrite(testCase)
            %Check we can read/write to the wishbone bus
            %Use one of the QDSP registers
            val = randi(2^32, 'uint32');
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 2, val);
            checkVal = read_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 2);
            assertEqual(testCase, checkVal, val);
        end

        function test_stream_enable(testCase)
            %Enable a stream and then peak at the register to make sure bit is set
            stream = struct('a', randi(2), 'b', randi(4), 'c', randi([0,1]));
            enable_stream(testCase.x6, stream.a, stream.b, stream.c);

            checkVal = read_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(stream.a), 3);
            if stream.b == 0
                bit = stream.c;
            else
                bit = 15+ stream.b + 4*stream.c;
            end
            verifyTrue(testCase, logical(bitget(checkVal, bit+1))); %bitget is 1 indexed
            %Check enabledStreams got set
            verifyEqual(testCase, testCase.x6.enabledStreams{end}, [stream.a, stream.b, stream.c]);
        end

        function test_stream_disable(testCase)
            %Disable a stream and then peak at the register to make sure bit is cleared
            stream = struct('a', randi(2), 'b', randi(4), 'c', randi([0,1]));
            %First enable the stream
            enable_stream(testCase.x6, stream.a, stream.b, stream.c);
            disable_stream(testCase.x6, stream.a, stream.b, stream.c);

            checkVal = read_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(stream.a), hex2dec('0f'));
            if stream.b == 0
                bit = stream.c;
            else
                bit = 15+ stream.b + 4*stream.c;
            end
            verifyFalse(testCase, logical(bitget(checkVal, bit+1))); %bitget is 1 indexed

            %Also check that enabledStreams doesn't have stream any more
            verifyTrue(testCase, isempty(find(cellfun(@(x) isequal(x, [stream.a, stream.b, stream.c]), testCase.x6.enabledStreams), 1) ) );
        end

        function test_recordLength_length(testCase)
            %Test record length over max throws error
            set_averager_settings(testCase.x6, 16448, 16, 1, 1);
            %Doesn't throw until we try and acquire with an active raw stream
            enable_stream(testCase.x6, 1, 0, 0);
            verifyError(testCase, @() testCase.x6.acquire(), 'X6:Fail');
        end

        function test_recordLenth_granularity(testCase)
            %Record length should be multiple of 4
            verifyError(testCase, @() set_averager_settings(testCase.x6, 123, 16, 1, 1), 'X6:Fail');
        end

        function test_recordLength_register(testCase)
            %Test record length register is set in both DSP modules
            val = uint32(16*randi(256));
            set_averager_settings(testCase.x6, val, 1, 1, 1);
            testVal = read_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 2);
            verifyEqual(testCase, testVal, val);
            testVal = read_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(2), 2);
            verifyEqual(testCase, testVal, val);
        end

        function test_nco_freq(testCase)
            %Test that the NCO frequency is set correctly
            freq = randi(125e6);
            a = randi(2);
            b = randi(2);
            set_nco_frequency(testCase.x6, a, b, freq);
            testVal = get_nco_frequency(testCase.x6, a, b);
            %Don't expect more than 24 bits precision on a quarter sampling rate
            verifyEqual(testCase, testVal, freq, 'AbsTol', 1e9 / 4 / 2^24);
        end

        function test_pg_waveform_length(testCase)
            %Test maximum waveform length is 16384
            wf = -1.0 + (2-1/2^15)*rand(16388,1);
            %Maximum length should pass
            write_pulse_waveform(testCase.x6, randi([0 1]), wf(1:16384));
            %Over maximum length should throw
            verifyError(testCase, @() write_pulse_waveform(testCase.x6, randi([0 1]), wf), 'X6:Fail');
        end

        function test_pg_waveform_granularity(testCase)
            wf = -1.0 + (2-1/2^15)*rand(16382,1);
            verifyError(testCase, @() write_pulse_waveform(testCase.x6, randi([0 1]), wf), 'X6:Fail');
        end

        function test_pg_waveform_range(testCase)
            %Test overrange
            wf = -1.0 + (2-1/2^15)*rand(128,1);
            wf(randi(128)) = 1.0;
            verifyError(testCase, @() write_pulse_waveform(testCase.x6, randi([0 1]), wf), 'X6:Fail');

            %Test underrange
            wf = -1.0 + (2-1/2^15)*rand(128,1);
            wf(randi(128)) = -(1.0+1/2^15);
            verifyError(testCase, @() write_pulse_waveform(testCase.x6, randi([0 1]), wf), 'X6:Fail');
        end

        function test_pg_waveform_readwrite(testCase)
            %Write a random waveform of the full length
            wf = -1.0 + (2-1/2^15)*rand(16384,1);
            pg = randi([0,1]);
            write_pulse_waveform(testCase.x6, pg, wf);

            %Now test first/last and a random selection in between (takes 1.5min to test all values)
            addresses = [1; 16384; randi(16384, 100, 1)];
            for ct = 1:length(addresses)
                %Have 16 bits of precision on the DAC
                testVal = read_pulse_waveform(testCase.x6, pg, addresses(ct));
                verifyEqual(testCase, testVal, wf(addresses(ct)), 'AbsTol', 1/2^15);
            end
        end

    end

end
