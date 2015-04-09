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
        T = testCase.x6.getLogicTemperature();
        testCase.verifyTrue((T > 20) && (T < 80))
    end

end
