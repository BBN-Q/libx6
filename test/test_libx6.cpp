#include "catch.hpp"

#include "libx6adc.h"
#include "constants.h"

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

		get_firmware_version(0, BBN_X6, &ver);
		REQUIRE( ver >= 0x0008);
		get_firmware_version(0, BBN_QDSP, &ver);
		REQUIRE( ver >= 0x0100);
		get_firmware_version(0, BBN_PG, &ver);
		REQUIRE( ver >= 0x0001);
	}

	disconnect_x6(0);
}

TEST_CASE("record length") {

	connect_x6(0);

	SECTION("record length validators and register") {

		//minimum size of 128
		CHECK( set_averager_settings(0, 96, 64, 1 , 1) == X6_INVALID_RECORD_LENGTH);

		//maximum size of 16384
		CHECK( set_averager_settings(0, 16416, 64, 1 , 1) == X6_INVALID_RECORD_LENGTH);

		//multiple of 32
		CHECK( set_averager_settings(0, 144, 64, 1 , 1) == X6_INVALID_RECORD_LENGTH);

		//Check registers are set
		set_averager_settings(0, 256, 64, 1 , 1);
		for (size_t ct = 0; ct < 2; ct++) {
			uint32_t val;
			read_register(0, BASE_DSP[ct], WB_QDSP_RECORD_LENGTH, &val);
			CHECK( val = 256);
		}

	}

	disconnect_x6(0);
}
