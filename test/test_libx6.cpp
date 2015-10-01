#include "catch.hpp"

#include "libx6adc.h"


TEST_CASE("board present", "[get_num_devices]"){
	SECTION("at least one board present"){
		unsigned numDevices;
		get_num_devices(&numDevices);
		REQUIRE( numDevices >= 1);
	}
}


TEST_CASE("board temperature is sane", "[get_logic_temperature]"){

	connect_x6(0);

	SECTION("reported temperature is between 20C and 70C"){
		float logicTemp;
	    get_logic_temperature(0, &logicTemp);
		REQUIRE( logicTemp > 20 );
		REQUIRE( logicTemp < 70 );
	}
}
