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
            %Use one of the DSP registers
            val = randi(2^32, 'uint32');
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 56, val);
            checkVal = read_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 56);
            assertEqual(testCase, val, checkVal);
        end

        function test_stream_enable(testCase)
            %Enable a stream and then peak at the register to make sure bit is set
            stream = struct('a', randi(2), 'b', randi(4), 'c', randi([0,1]));
            enable_stream(testCase.x6, stream.a, stream.b, stream.c);

            checkVal = read_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(stream.a), hex2dec('0f'));
            bit = stream.b + 15*stream.c;
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
            bit = stream.b + 15*stream.c;
            verifyFalse(testCase, logical(bitget(checkVal, bit+1))); %bitget is 1 indexed

            %Also check that enabledStreams doesn't have stream any more
            verifyTrue(testCase, isempty(find(cellfun(@(x) isequal(x, [stream.a, stream.b, stream.c]), testCase.x6.enabledStreams), 1) ) );
        end

        function test_recordLength_length(testCase)
            %Test record length over max throws error
            set_averager_settings(testCase.x6, 4160, 16, 1, 1);
            %Doesn't throw until we try and acquire with an active raw stream
            enable_stream(testCase.x6, 1, 0, 0);
            verifyError(testCase, @() testCase.x6.acquire(), 'X6:Fail');
        end

        function test_recordLenth_granularity(testCase)
            %Record length should be multiple of 4
            verifyError(testCase, @() set_averager_settings(x6, 123, 16, 1, 1), 'X6:Fail');
        end

        function test_recordLength_register(testCase)
            %Test record length register is set in both DSP modules
            val = 4*randi(1024);
            set_averager_settings(testCase.x6, val, 1, 1, 1);
            checkVal = read_register(testCase.x6, testCase.DSP_WB_OFFSET(1), 63);
            verifyEqual(testCase, val, checkVal);
            checkVal = read_register(testCase.x6, testCase.DSP_WB_OFFSET(2), 63);
            verifyEqual(testCase, val, checkVal);
        end

        function test_nco_freq(testCase)
            %Test that the NCO frequency is set correctly
            freq = randi(125e6);
            a = randi(2);
            b = 0; %just a single demod channel for now
            set_nco_frequency(testCase.x6, a, b);
            checkVal = get_nco_frequency(testCase.x6, a, b);
            verfiyEqual(testCase, freq, checkVal);
        end
    end

end
