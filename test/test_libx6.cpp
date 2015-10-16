#include "catch.hpp"

#include <complex>
using std::complex;
#include <vector>
using std::vector;
#include <random>
#include <algorithm>

#include "libx6.h"
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
		REQUIRE( ver >= 0x0009);
		get_firmware_version(0, BBN_QDSP, &ver);
		REQUIRE( ver >= 0x0101);
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

void check_kernel(unsigned a, unsigned b, unsigned c, vector<complex<double>> & kernel, size_t numChecks) {

	write_kernel(0, 1, 0, 1, reinterpret_cast<_Complex double*>(kernel.data()), kernel.size());

	//Check first/last and random selection in between (too slow to do all)
	vector<unsigned> checkIdx(numChecks);
	checkIdx[0] = 0;
	checkIdx[1] = kernel.size()-1;
	std::uniform_int_distribution<unsigned> dist(0, kernel.size()-1);
	std::mt19937 engine; // Mersenne twister MT19937
	auto gen_rand_idx = std::bind(dist, engine);
	std::generate_n(checkIdx.begin()+2, numChecks-2, gen_rand_idx);

	for (size_t ct = 0; ct < checkIdx.size(); ct++) {
		complex<double> val;
		read_kernel(0, 1, 0, 1, checkIdx[ct], reinterpret_cast<_Complex double*>(&val));
		INFO( "Read back: " << val << " ; expected: " << kernel[checkIdx[ct]] );
		CHECK( std::real(val) == Approx( std::real(kernel[checkIdx[ct]])).epsilon( 2.0 / (1 << 15)) );
		CHECK( std::imag(val) == Approx( std::imag(kernel[checkIdx[ct]])).epsilon( 2.0 / (1 << 15)) );
	}

}

TEST_CASE("kernels") {

	connect_x6(0);

	std::uniform_real_distribution<float> dist(MIN_KERNEL_VALUE, MAX_KERNEL_VALUE);
	std::mt19937 engine; // Mersenne twister MT19937
	auto gen_rand_real = std::bind(dist, engine);

	auto gen_rand_complex = [&]() {
		return complex<double>(gen_rand_real(), gen_rand_real());
	};

	vector<complex<double>> kernel;

	SECTION("kernel validators") {

		//raw kernel less than 4096
		kernel.resize(4097);
		CHECK( write_kernel(0, 1, 0, 1, reinterpret_cast<_Complex double*>(kernel.data()), kernel.size()) == X6_INVALID_KERNEL_LENGTH );

		//demod kernel less than 512
		kernel.resize(513);
		CHECK( write_kernel(0, 1, 1, 1, reinterpret_cast<_Complex double*>(kernel.data()), kernel.size()) == X6_INVALID_KERNEL_LENGTH );

		//Kernel overrange
		kernel.resize(512);
		std::generate_n(kernel.begin(), kernel.size(), gen_rand_complex);
		kernel[81] = 1.1;
		CHECK( write_kernel(0, 1, 1, 1, reinterpret_cast<_Complex double*>(kernel.data()), kernel.size()) == X6_KERNEL_OUT_OF_RANGE );

	}

	SECTION("raw kernel write read") {

		kernel.resize(4096);
		std::generate_n(kernel.begin(), kernel.size(), gen_rand_complex);
		check_kernel(1, 0, 1, kernel, 100);
	}

	SECTION("demod kernel write read") {

		kernel.resize(512);
		std::generate_n(kernel.begin(), kernel.size(), gen_rand_complex);
		check_kernel(1, 1, 1, kernel, 100);
	}

	disconnect_x6(0);

}

TEST_CASE("streams", "[streams]") {

	connect_x6(0);

	SECTION("digitizer mode") {

		enable_stream(0, 1, 0, 0);
		enable_stream(0, 1, 1, 0);

		set_nco_frequency(0, 1, 1, 11e6);

		set_averager_settings(0, 5120, 64, 1, 2);

		set_digitizer_mode(0, DIGITIZER);

		acquire(0);

		wait_for_acquisition(0, 1);

		stop(0);
	}

	disconnect_x6(0);

}
