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
            testCase.verifyTrue((T > 20) && (T < 80))
        end

        function test_wishbone_readwrite(testCase)
            %Check we can read/write to the wishbone bus
            %Use one of the DSP registers
            val = randi(2^32, 'uint32');
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 56, val);
            checkVal = read_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 56);
            testCase.assertEqual(val, checkVal);
        end

        function test_stream_enable(testCase)
            %Enable a stream and then peak at the register to make sure bit is set
            stream = struct('a', randi(2), 'b', randi(4), 'c', randi([0,1]));
            enable_stream(testCase.x6, stream);

            checkVal = read_register(testCase.x6, testCase.DSP_WB_OFFSET(stream.a), hex2dec('0x0f'));
            bit = stream.b + 15*stream.c;
            testCase.verifyTrue(bitget(checkVal, bit));
        end

        function test_stream_disable(testCase)
            %Disable a stream and then peak at the register to make sure bit is cleared
            stream = struct('a', randi(2), 'b', randi(4), 'c', randi([0,1]));
            disable_stream(testCase.x6, stream);

            checkVal = read_register(testCase.x6, testCase.DSP_WB_OFFSET(stream.a), hex2dec('0x0f'));
            bit = stream.b + 15*stream.c;
            testCase.verifyFalse(bitget(checkVal, bit));
        end

        function test_recordLength(testCase)
            %Test record length over max throws error
            testCase.verifyError(@() set_averager_settings(x6, 4097, 16, 1, 1), 'X6:Fail');

            %Test record length register is set in both DSP modules
            val = randi(4000);
            set_averager_settings(testCase.x6, val, 1, 1, 1);
            checkVal = read_register(testCase.x6, testCase.DSP_WB_OFFSET(1), 63);
            testCase.verifyEqual(val, checkVal);
            checkVal = read_register(testCase.x6, testCase.DSP_WB_OFFSET(2), 63);
            testCase.verifyEqual(val, checkVal);
        end

        function test_nco_freq(testCase)
            %Test that the NCO frequency is set correctly
            freq = randi(125e6);
            a = randi(2);
            b = 0; %just a single demod channel for now
            set_nco_frequency(testCase.x6, a, b);
            checkVal = get_nco_frequency(testCase.x6, a, b);
            testCase.verfiyEqual(freq, checkVal);
        end
    end

end
