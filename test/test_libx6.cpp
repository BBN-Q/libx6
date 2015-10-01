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

	disconnect_x6(0);
}

TEST_CASE("firmware version", "[get_firmware_version]") {

	connect_x6(0);

	SECTION("firmware version checks") {
		uint16_t ver;

		SECTION("BBN firmware is greater than 0.8"){
			get_firmware_version(0, BBN_X6, &ver);
			REQUIRE( ver >= 0x0008);
		}

		SECTION("BBN QDSP firmware is greater than 1.0"){
			get_firmware_version(0, BBN_QDSP, &ver);
			REQUIRE( ver >= 0x0100);
		}

		SECTION("BBN PG firmware is greater than 0.1"){
			get_firmware_version(0, BBN_PG, &ver);
			REQUIRE( ver >= 0x0001);
		}
	}

	disconnect_x6(0);
}
