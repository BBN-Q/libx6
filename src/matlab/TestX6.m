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
            unloadlibrary('libx6');
        end

    end

    methods(Test)

        function test_temperature(testCase)
            %Check temperature is between 20C and 80C
            T = testCase.x6.get_logic_temperature();
            assertTrue(testCase, (T > 20) && (T < 80))
        end

        function test_wishbone_readwrite(testCase)
            %Check we can read/write to the wishbone bus
            %Use one of the QDSP registers
            val = randi(2^32, 'uint32');
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 2, val);
            checkVal = read_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 2);
            assertEqual(testCase, checkVal, val);
        end

        function test_reference(testCase)
            %Check that we can set/get the reference source
            curRef = testCase.x6.reference;
            toggle_map = containers.Map({'EXTERNAL_REFERENCE', 'INTERNAL_REFERENCE'}, {'INTERNAL_REFERENCE', 'EXTERNAL_REFERENCE'});
            testCase.x6.reference = toggle_map(curRef);
            assertEqual(testCase, testCase.x6.reference, toggle_map(curRef))
            testCase.x6.reference = curRef; %Class based tests methods have effects for subsequent tests so put it back
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
                bit = 15 + stream.b + 4*stream.c;
            end
            verifyFalse(testCase, logical(bitget(checkVal, bit+1))); %bitget is 1 indexed

            %Also check that enabledStreams doesn't have stream any more
            verifyTrue(testCase, isempty(find(cellfun(@(x) isequal(x, [stream.a, stream.b, stream.c]), testCase.x6.enabledStreams), 1) ) );
        end

        function test_recordLength_validators(testCase)
            %Min of 128
            verifyError(testCase, @() set_averager_settings(testCase.x6, 96, 16, 1, 1), 'X6:Fail');

            %Max of 16384
            verifyError(testCase, @() set_averager_settings(testCase.x6, 16448, 16, 1, 1), 'X6:Fail');

            %Record length should be multiple of 128
            verifyError(testCase, @() set_averager_settings(testCase.x6, 320, 16, 1, 1), 'X6:Fail');
        end

        function test_recordLength_register(testCase)
            %Test record length register is set in both DSP modules
            val = uint32(32*randi([4,256]));
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
            assertEqual(testCase, testVal, freq, 'AbsTol', 1e9 / 4 / 2^24);
        end

        function test_DataReady(testCase)
            %Test that the DataReady event is fired
            eventFired = false;
            function catchDataReady(~,~)
                eventFired = true;
            end
            el = addlistener(testCase.x6, 'DataReady', @catchDataReady);

            disconnect(testCase.x6);
            connect(testCase.x6, 0);

            %Enable the two raw streams
            enable_stream(testCase.x6, 1, 0, 0);

            set_averager_settings(testCase.x6, 5120, 64, 1, 1);

            acquire(testCase.x6);

            %enable test mode
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 1, 65536 + 25000);

            success = wait_for_acquisition(testCase.x6, 1);
            stop(testCase.x6);
            delete(el);

            verifyEqual(testCase, success, 0);
            assertTrue(testCase, eventFired, 'DataReady event failed to fire.');
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

            %enable test mode
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 1, 65536 + 25000);
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(2), 1, 65536 + 25000);

            success = wait_for_acquisition(testCase.x6, 1);
            stop(testCase.x6);
            assertEqual(testCase, success, 0);

            %Now check
            function check_raw_vals(chan)
                wfs = transfer_stream(testCase.x6, struct('a', chan, 'b', 0, 'c', 0));
                verifyEqual(testCase, wfs, TestX6.expected_raw_wfs(), 'AbsTol', 2/2048);

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

            %enable test mode
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 1, 65536 + 25000);

            success = wait_for_acquisition(testCase.x6, 1);
            stop(testCase.x6);
            assertEqual(testCase, success, 0);

            b1 = [-0.0002410984, 0.0004020658, 0.0006323846, 0.0007850739, 0.0006174427,...
            1.5386e-5, -0.000915533, -0.0018233843, -0.0022089995, -0.0016422228,...
            -2.26288e-5, 0.0022437205, 0.0042720443, 0.00498372, 0.0035814231, 3.31909e-5,...
            -0.0046728914, -0.0086930371, -0.0099484781, -0.0070308419, -4.38741e-5,...
            0.0090217522, 0.0166660779, 0.0190245151, 0.0134634247, 5.28584e-5,...
            -0.017716183, -0.0334095584, -0.0393558294, -0.0291375622, -5.87746e-5,...
            0.0453090784, 0.0991626875, 0.1502265763, 0.1867888057, 0.2000608978,...
            0.1867888057, 0.1502265763, 0.0991626875, 0.0453090784, -5.87746e-5,...
            -0.0291375622, -0.0393558294, -0.0334095584, -0.017716183, 5.28584e-5,...
            0.0134634247, 0.0190245151, 0.0166660779, 0.0090217522, -4.38741e-5,...
            -0.0070308419, -0.0099484781, -0.0086930371, -0.0046728914, 3.31909e-5,...
            0.0035814231, 0.00498372, 0.0042720443, 0.0022437205, -2.26288e-5,...
            -0.0016422228, -0.0022089995, -0.0018233843, -0.000915533, 1.5386e-5,...
            0.0006174427, 0.0007850739, 0.0006323846, 0.0004020658, -0.0002410984];

            b2 = [-0.0003214219, -7.71652e-5, 0.0001866883, 0.0005320205, 0.0007077541,...
            0.000467486, -0.0002288178, -0.0010876267, -0.0015584393, -0.001138229,...
            0.0002397764, 0.0019908646, 0.003054644, 0.0024676827, 6.00955e-5,...
            -0.0031704981, -0.0053630501, -0.0047708112, -0.0009485702, 0.0045823556,...
            0.0087695581, 0.0085747679, 0.0029216877, -0.0061003365, -0.0137255692,...
            -0.0147969995, -0.0068734866, 0.0075597, 0.0213222835, 0.0256433269,...
            0.0149878227, -0.0087769288, -0.0353245358, -0.0491137157, -0.0360042224,...
            0.0095852537, 0.0804636506, 0.1578026517, 0.2176341525, 0.24013138,...
            0.2176341525, 0.1578026517, 0.0804636506, 0.0095852537, -0.0360042224,...
            -0.0491137157, -0.0353245358, -0.0087769288, 0.0149878227, 0.0256433269,...
            0.0213222835, 0.0075597, -0.0068734866, -0.0147969995, -0.0137255692,...
            -0.0061003365, 0.0029216877, 0.0085747679, 0.0087695581, 0.0045823556,...
            -0.0009485702, -0.0047708112, -0.0053630501, -0.0031704981, 6.00955e-5,...
            0.0024676827, 0.003054644, 0.0019908646, 0.0002397764, -0.001138229,...
            -0.0015584393, -0.0010876267, -0.0002288178, 0.000467486, 0.0007077541,...
            0.0005320205, 0.0001866883, -7.71652e-5, -0.0003214219];

            dds = exp(-1j*2*pi*11e6*4e-9*(3:1282)).';

            rawWFs = transfer_stream(testCase.x6, struct('a', 1, 'b', 0, 'c', 0));
            demodWFs = transfer_stream(testCase.x6, struct('a', 1, 'b', 1, 'c', 0));
            for ct = 1:64
                mult = dds .* rawWFs(:,ct);
                stage1 = filter(b1, 1.0, mult);
                stage2 = filter(b2, 1.0, stage1(4:4:end));
                verifyEqual(testCase, demodWFs(:,ct), stage2(2:2:end), 'AbsTol', 1e-3);
            end
        end

        function test_raw_kernel_memory(testCase)

            baseKernel = (-1.0 + (2-1/2^15)*rand(4097,1)) + 1i*(-1.0 + (2-1/2^15)*rand(4097,1));
            %Test kernel too long throws
            verifyError(testCase, @() write_kernel(testCase.x6, 1, 0, 1, baseKernel), 'X6:Fail');

            %Test kernel over range throws
            kernel = baseKernel(1:4096);
            kernel(81) = 1.0 + 1/2^14;
            verifyError(testCase, @() write_kernel(testCase.x6, 1, 0, 1, kernel), 'X6:Fail');

            %Check we can write/read to the raw kernel memory
            kernel = baseKernel(1:4096);
            write_kernel(testCase.x6, 1, 0, 1, kernel);

            %Now test first/last and a random selection in between
            addresses = [1; 4096; randi(4096, 100, 1)];
            testVals = arrayfun(@(addr) read_kernel(testCase.x6, 1, 0, 1, addr), addresses);
            %Have 16 bits of precision in real/imag
            assertEqual(testCase, testVals, kernel(addresses), 'AbsTol', 2/2^15);
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

            %enable test mode
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 1, 65536 + 25000);

            success = wait_for_acquisition(testCase.x6, 1);
            stop(testCase.x6);
            assertEqual(testCase, success, 0);

            rawWFs = transfer_stream(testCase.x6, struct('a', 1, 'b', 0, 'c', 0));
            KIs = transfer_stream(testCase.x6, struct('a', 1, 'b', 0, 'c', 1));
            expected = sum(bsxfun(@times, kernel, rawWFs(1:length(kernel),:)), 1);
            assertEqual(testCase, KIs, expected, 'AbsTol', 1/2^8);
        end

        function test_demod_kernel_memory(testCase)

            baseKernel = (-1.0 + (2-1/2^15)*rand(513,1)) + 1i*(-1.0 + (2-1/2^15)*rand(513,1));

            %Test kernel too long throws
            verifyError(testCase, @() write_kernel(testCase.x6, 1, 1, 1, baseKernel), 'X6:Fail');

            %Test kernel over range throws
            kernel = baseKernel(1:512);
            kernel(81) = 1.0 + 1/2^14;
            verifyError(testCase, @() write_kernel(testCase.x6, 1, 1, 1, kernel), 'X6:Fail');

            %Check we can write/read to the demod kernel memory
            kernel = baseKernel(1:512);
            write_kernel(testCase.x6, 1, 1, 1, kernel);

            %Now test first/last and a random selection in between
            addresses = [1; 512; randi(512, 100, 1)];
            testVals = arrayfun(@(addr) read_kernel(testCase.x6, 1, 1, 1, addr), addresses);
            %Have 16 bits of precision in real/imag
            assertEqual(testCase, testVals, kernel(addresses), 'AbsTol', 2/2^15);
        end

        function test_demod_integrator(testCase)
            %Test the integrated demod stream
            disconnect(testCase.x6);
            connect(testCase.x6, 0);

            %Enable a demod stream (to feed into expected calculation) and one demod kernel integrator stream
            enable_stream(testCase.x6, 1, 1, 0);
            enable_stream(testCase.x6, 1, 1, 1);

            %Write a random kernel
            kernel = (-1.0 + 2*rand(160,1)) + 1i*(-1.0 + 2*rand(160,1));
            write_kernel(testCase.x6, 1, 1, 1, kernel);

            set_averager_settings(testCase.x6, 5120, 64, 1, 1);

            acquire(testCase.x6);

            %enable test mode
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 1, 65536 + 25000);

            success = wait_for_acquisition(testCase.x6, 1);
            stop(testCase.x6);
            assertEqual(testCase, success, 0);

            demodWFs = transfer_stream(testCase.x6, struct('a', 1, 'b', 1, 'c', 0));
            KIs = transfer_stream(testCase.x6, struct('a', 1, 'b', 1, 'c', 1));
            expected = sum(bsxfun(@times, kernel, demodWFs(1:length(kernel),:)), 1);
            assertEqual(testCase, KIs, expected, 'AbsTol', 1/2^10);
        end

        function test_threshold(testCase)
            %Test setting and getting the threshold register
            threshold = -1 + 2*rand();
            a = randi(2);
            c = randi(2);
            set_threshold(testCase.x6, a, c, threshold);
            assertEqual(testCase, get_threshold(testCase.x6, a, c), threshold, 'AbsTol', 2/2^15);
        end

        function test_pg_waveform_length(testCase)
            %Test maximum waveform length is 16384
            wf = -1.0 + (2-1/2^15)*rand(16388,1);
            %Maximum length should pass
            write_pulse_waveform(testCase.x6, randi([0 1]), wf(1:16384));
            %Over maximum length should throw
            assertError(testCase, @() write_pulse_waveform(testCase.x6, randi([0 1]), wf), 'X6:Fail');
        end

        function test_pg_waveform_granularity(testCase)
            wf = -1.0 + (2-1/2^15)*rand(16382,1);
            assertError(testCase, @() write_pulse_waveform(testCase.x6, randi([0 1]), wf), 'X6:Fail');
        end

        function test_pg_waveform_range(testCase)
            %Test overrange
            wf = -1.0 + (2-1/2^15)*rand(128,1);
            wf(randi(128)) = 1.0 + 1/2^14;
            verifyError(testCase, @() write_pulse_waveform(testCase.x6, randi([0 1]), wf), 'X6:Fail');

            %Test underrange
            wf = -1.0 + (2-1/2^15)*rand(128,1);
            wf(randi(128)) = -(1.0+1/2^14);
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

        function test_digitizer_mode(testCase)
            %Test the digitizer mode can stream records
            disconnect(testCase.x6);
            connect(testCase.x6, 0);

            enable_stream(testCase.x6, 1, 0, 0);
            enable_stream(testCase.x6, 1, 1, 0);
            enable_stream(testCase.x6, 1, 0, 1);

            rawWFs = [];
            demodWFs = [];
            resultWFs = [];
            function catchDataReady(src,~)
                rawWFs = cat(4, rawWFs, src.transfer_stream(struct('a', 1, 'b', 0, 'c', 0)));
                demodWFs = cat(4, demodWFs, src.transfer_stream(struct('a', 1, 'b', 1, 'c', 0)));
                resultWFs = cat(4, resultWFs, src.transfer_stream(struct('a', 1, 'b', 0, 'c', 1)));
            end
            el = addlistener(testCase.x6, 'DataReady', @catchDataReady);

            set_nco_frequency(testCase.x6, 1, 1, 11e6);
            write_kernel(testCase.x6, 1, 0, 1, ones(5120/4, 1));

            numRRs = 100;
            set_averager_settings(testCase.x6, 5120, 64, 1, numRRs);

            testCase.x6.digitizerMode = 'DIGITIZER';

            acquire(testCase.x6);

            %enable test mode
            write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 1, 65536 + 25000);

            success = wait_for_acquisition(testCase.x6, 10);
            stop(testCase.x6);
            delete(el);
            assertEqual(testCase, success, 0);

            % check the values
            assertEqual(testCase, size(rawWFs), [1280,1,64,numRRs])
            assertEqual(testCase, size(demodWFs), [160,1,64,numRRs])
            assertEqual(testCase, size(resultWFs), [1,1,64,numRRs])
            expected_raws = TestX6.expected_raw_wfs();
            for ct = 1:numRRs
                verifyEqual(testCase, squeeze(rawWFs(:,:,:,ct)), expected_raws, 'AbsTol', 2/2048);
            end
            verifyEqual(testCase, resultWFs, sum(rawWFs, 1), 'AbsTol', 0.1);
        end

        function test_multiple_acquisitions(testCase)
          %Test stopping and starting acquisition
          disconnect(testCase.x6);
          connect(testCase.x6, 0);

          enable_stream(testCase.x6, 1, 0, 0);
          enable_stream(testCase.x6, 1, 1, 0);
          enable_stream(testCase.x6, 1, 0, 1);

          rawWFs = [];
          demodWFs = [];
          resultWFs = [];
          function catchDataReady(src,~)
              rawWFs = cat(4, rawWFs, src.transfer_stream(struct('a', 1, 'b', 0, 'c', 0)));
              demodWFs = cat(4, demodWFs, src.transfer_stream(struct('a', 1, 'b', 1, 'c', 0)));
              resultWFs = cat(4, resultWFs, src.transfer_stream(struct('a', 1, 'b', 0, 'c', 1)));
          end
          el = addlistener(testCase.x6, 'DataReady', @catchDataReady);

          set_nco_frequency(testCase.x6, 1, 1, 11e6);
          write_kernel(testCase.x6, 1, 0, 1, ones(5120/4, 1));

          numRRs = 100;
          set_averager_settings(testCase.x6, 5120, 64, 1, numRRs);

          testCase.x6.digitizerMode = 'DIGITIZER';

          acquire(testCase.x6);

          %enable test mode
          write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 1, 65536 + 25000);

          success = wait_for_acquisition(testCase.x6, 10);
          stop(testCase.x6);
          delete(el);
          assertEqual(testCase, success, 0);
          assertEqual(testCase, size(rawWFs), [1280,1,64,numRRs])
          assertEqual(testCase, size(demodWFs), [160,1,64,numRRs])
          assertEqual(testCase, size(resultWFs), [1,1,64,numRRs])

          % check the values
          assertEqual(testCase, size(rawWFs), [1280,1,64,numRRs])
          expected_raws = TestX6.expected_raw_wfs();
          for ct = 1:numRRs
              verifyEqual(testCase, squeeze(rawWFs(:,:,:,ct)), expected_raws, 'AbsTol', 2/2048);
          end
          verifyEqual(testCase, resultWFs, sum(rawWFs, 1), 'AbsTol', 0.1);

          %disable test mode
          write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 1, 0);

          %restart the acquisition
          rawWFs = [];
          demodWFs = [];
          resultWFs = [];
          el = addlistener(testCase.x6, 'DataReady', @catchDataReady);

          acquire(testCase.x6);

          %enable test mode
          write_register(testCase.x6, testCase.x6.DSP_WB_OFFSET(1), 1, 65536 + 25000);

          success = wait_for_acquisition(testCase.x6, 10);
          stop(testCase.x6);
          delete(el);
          assertEqual(testCase, success, 0);
          assertEqual(testCase, size(rawWFs), [1280,1,64,numRRs])
          assertEqual(testCase, size(demodWFs), [160,1,64,numRRs])
          assertEqual(testCase, size(resultWFs), [1,1,64,numRRs])

      end

    end

    methods(Static)
        function expected = expected_raw_wfs()
            expected = zeros(1280, 64);
            for ct = 1:64
                baseNCO = 1.0000020265579224e6; %from 24bit precision
                pulse = (1 - 1/128)*(1 - 1/2048)*cos(2*pi*(mod(ct-1,64))*baseNCO*1e-9*(8:4107));
                expected(:,ct) = [zeros(23, 1); mean(reshape(pulse, 4, 1025), 1)'; zeros(232, 1)];
                %catch alignment marker
                if mod(ct, 64) == 1
                    expected(4:7,ct) = 1 - 1/2048;
                end
            end
        end
    end

end
