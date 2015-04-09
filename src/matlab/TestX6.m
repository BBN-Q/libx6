class X6Test < matlab.unitest.TestCase

    properties
        x6
    end

    methods(TestMethodSetup)
        function create_x6(obj)
            x6 = X6();
        end

        function connect_x6(obj)
            connect(obj.x6, 0)
        end

    end

    methods(TestMethodTeardown)

        function disconnect_x6(obj)
            disconnect(obj.x6)
        end

        function unload_lib(obj)
            obj.x6 = []
            unloadlibrary('libx6adc')
        end

    end

    methods(Test)

    function test_temperature(testCase)
        T = obj.x6.getLogicTemperature();
        testCase.verifyTrue((T > 20) && (T < 80))
    end

end
