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

        function test_raw_streams(testCase)
            %Check the test pattern on the raw streams
            disconnect(testCase.x6);
            connect(testCase.x6, 0);

            %Enable the two raw streams
            enable_stream(testCase.x6, 1, 0, 0);
            enable_stream(testCase.x6, 2, 0, 0);

            set_averager_settings(testCase.x6, 5120, 64, 1, 1);

            acquire(testCase.x6);

            pause(0.5);

            %enable test mode
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 1, 65536 + 25000);
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(2), 1, 65536 + 25000);

            success = wait_for_acquisition(testCase.x6, 1);
            stop(testCase.x6);
            verifyEqual(testCase, success, 0);

            %Now check
            function check_raw_vals(chan)
                wfs = transfer_stream(testCase.x6, struct('a', chan, 'b', 0, 'c', 0));

                for ct = 1:64
                    baseNCO = 1.0000020265579224e6; %from 24bit precision
                    pulse = (1 - 1/128)*(1 - 1/2048)*cos(2*pi*(ct-1)*baseNCO*1e-9*(8:4107));
                    expected = [zeros(23, 1); mean(reshape(pulse, 4, 1025), 1)'; zeros(232, 1)];
                    %catch alignment marker
                    if ct == 1
                        expected(4:7) = 1 - 1/2048;
                    end
                    verifyEqual(testCase, wfs(:,ct), expected, 'AbsTol', 2/2048);
                end

            end

            check_raw_vals(1);
            check_raw_vals(2);
        end

        function test_demod_streams(testCase)
            %Test the filtered demodulated streams
            disconnect(testCase.x6);
            connect(testCase.x6, 0);

            %Enable the raw (to feed into expected calculation) and one demod stream
            enable_stream(testCase.x6, 1, 0, 0);
            enable_stream(testCase.x6, 1, 1, 0);

            set_nco_frequency(testCase.x6, 1, 1, 11e6);

            set_averager_settings(testCase.x6, 5120, 64, 1, 1);

            acquire(testCase.x6);

            pause(0.5);

            %enable test mode
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 1, 65536 + 25000);

            success = wait_for_acquisition(testCase.x6, 1);
            stop(testCase.x6);
            verifyEqual(testCase, success, 0);

            b1 = [0.0008522172,0.0003782314,-4.1098e-5,-0.0007865892,-0.0015758075,-0.0019626337,-0.0015259535,-0.0001191829,0.0019387868,0.003855921,0.0046109846,0.0034010297,0.0001229449,-0.004344112,-0.0082562201,-0.0095817866,-0.0068841954,-0.0001758237,0.0086706693,0.0162467395,0.0187102645,0.0133748979,0.0002089665,-0.0174035879,-0.0330903156,-0.039167587,-0.0291477354,-0.0002341847,0.0450762459,0.0989974616,0.150208834,0.1869125901,0.2002428237,0.1869125901,0.150208834,0.0989974616,0.0450762459,-0.0002341847,-0.0291477354,-0.039167587,-0.0330903156,-0.0174035879,0.0002089665,0.0133748979,0.0187102645,0.0162467395,0.0086706693,-0.0001758237,-0.0068841954,-0.0095817866,-0.0082562201,-0.004344112,0.0001229449,0.0034010297,0.0046109846,0.003855921,0.0019387868,-0.0001191829,-0.0015259535,-0.0019626337,-0.0015758075,-0.0007865892,-4.1098e-5,0.0003782314,0.0008522172];
            b2 =  [0.0001649556,-6.64166e-5,-0.0003334019,-0.0005526026,-0.0004150039,0.0002333881,0.0011328542,0.0016101256,0.0009777185,-0.0008408614,-0.0029254551,-0.0036710748,-0.0018352389,0.0022643119,0.006348953,0.007218227,0.0029284839,-0.0052129535,-0.0124661206,-0.0130465772,-0.0041165731,0.0110908225,0.0235758874,0.0231285403,0.005196213,-0.0239635977,-0.0476848278,-0.0459136476,-0.0059535755,0.0687696723,0.1572919896,0.2288082628,0.2562262832,0.2288082628,0.1572919896,0.0687696723,-0.0059535755,-0.0459136476,-0.0476848278,-0.0239635977,0.005196213,0.0231285403,0.0235758874,0.0110908225,-0.0041165731,-0.0130465772,-0.0124661206,-0.0052129535,0.0029284839,0.007218227,0.006348953,0.0022643119,-0.0018352389,-0.0036710748,-0.0029254551,-0.0008408614,0.0009777185,0.0016101256,0.0011328542,0.0002333881,-0.0004150039,-0.0005526026,-0.0003334019,-6.64166e-5,0.0001649556];
            dds = exp(-1j*2*pi*11e6*4e-9*(1:1280)).';

            rawWFs = transfer_stream(testCase.x6, struct('a', 1, 'b', 0, 'c', 0));
            demodWFs = transfer_stream(testCase.x6, struct('a', 1, 'b', 1, 'c', 0));
            for ct = 1:64
                mult = dds .* rawWFs(:,ct);
                stage1 = filter(b1, 1.0, mult);
                stage2 = filter(b2, 1.0, stage1(1:4:end));
                verifyEqual(testCase, demodWFs(4:end,ct), stage2(1:2:end-6), 'AbsTol', 1e-3);
            end
        end

        function test_raw_kernel_memory(testCase)
            %Check we can write/read to the raw kernel memory
            kernel = (-1.0 + 2*rand(4096,1)) + 1i*(-1.0 + 2*rand(4096,1));
            write_kernel(testCase.x6, 1, 0, 1, kernel);

            %Now test first/last and a random selection in between
            addresses = [1; 4096; randi(4096, 100, 1)];
            for ct = 1:length(addresses)
                %Have 16 bits of precision in real/imag
                testVal = read_kernel(testCase.x6, 1, 0, 1, addresses(ct));
                verifyEqual(testCase, testVal, kernel(addresses(ct)), 'AbsTol', 2/2^15);
            end
        end

        function test_raw_integrator(testCase)
            %Test the integrated raw stream
            disconnect(testCase.x6);
            connect(testCase.x6, 0);

            %Enable the raw (to feed into expected calculation) and one kernel integrator stream
            enable_stream(testCase.x6, 1, 0, 0);
            enable_stream(testCase.x6, 1, 0, 1);

            %Write a random kernel
            kernel = (-1.0 + 2*rand(1280,1)) + 1i*(-1.0 + 2*rand(1280,1));
            write_kernel(testCase.x6, 1, 0, 1, kernel);

            set_averager_settings(testCase.x6, 5120, 64, 1, 1);

            acquire(testCase.x6);

            pause(0.5);

            %enable test mode
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 1, 65536 + 25000);

            success = wait_for_acquisition(testCase.x6, 1);
            stop(testCase.x6);
            verifyEqual(testCase, success, 0);

            rawWFs = transfer_stream(testCase.x6, struct('a', 1, 'b', 0, 'c', 0));
            KIs = transfer_stream(testCase.x6, struct('a', 1, 'b', 0, 'c', 1));
            expected = sum(bsxfun(@times, kernel, rawWFs(1:length(kernel),:)), 1).';
            assertEqual(testCase, KIs, expected, 'AbsTol', 1/2^8);
        end

        function test_demod_integrator(testCase)
            %Test the integrated demod stream
            disconnect(testCase.x6);
            connect(testCase.x6, 0);

            %Enable the raw (to feed into expected calculation) and one kernel integrator stream
            enable_stream(testCase.x6, 1, 1, 0);
            enable_stream(testCase.x6, 1, 1, 1);

            %Write a random kernel
            kernel = (-1.0 + 2*rand(160,1)) + 1i*(-1.0 + 2*rand(160,1));
            write_kernel(testCase.x6, 1, 1, 1, kernel);

            set_averager_settings(testCase.x6, 5120, 64, 1, 1);

            acquire(testCase.x6);

            pause(0.5);

            %enable test mode
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 1, 65536 + 25000);

            success = wait_for_acquisition(testCase.x6, 1);
            stop(testCase.x6);
            verifyEqual(testCase, success, 0);

            demodWFs = transfer_stream(testCase.x6, struct('a', 1, 'b', 1, 'c', 0));
            KIs = transfer_stream(testCase.x6, struct('a', 1, 'b', 1, 'c', 1));
            expected = sum(bsxfun(@times, kernel, demodWFs(1:length(kernel),:)), 1).';
            assertEqual(testCase, KIs, expected, 'AbsTol', 1/2^10);
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
