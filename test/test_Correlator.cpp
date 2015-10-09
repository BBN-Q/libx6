#include "catch.hpp"

#include <vector>
using std::vector;

#include "QDSPStream.h"
#include "Correlator.h"

#include <Buffer_Mb.h>
#include <BufferDatagrams_Mb.h>

template <class T>
bool vec_equal(vector<T> a, vector<T> b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

TEST_CASE("Combinations function", "[combinations]") {

	vector<vector<int>> combos;

	SECTION("pairs from three choices") {
    	combos = combinations(3, 2);
		REQUIRE( combos.size() == 3 );
		REQUIRE( vec_equal(combos[0], {0,1}) );
		REQUIRE( vec_equal(combos[1], {0,2}) );
		REQUIRE( vec_equal(combos[2], {1,2}) );
	}

	SECTION("pairs from four choices") {
		combos = combinations(4,2);
		REQUIRE ( combos.size() == 6);
		REQUIRE( vec_equal(combos[0], {0,1}) );
		REQUIRE( vec_equal(combos[1], {0,2}) );
		REQUIRE( vec_equal(combos[2], {0,3}) );
		REQUIRE( vec_equal(combos[3], {1,2}) );
		REQUIRE( vec_equal(combos[4], {1,3}) );
		REQUIRE( vec_equal(combos[5], {2,3}) );
	}

	SECTION("from four choose 3") {
    	combos = combinations(4,3);
		REQUIRE( combos.size() == 4);
		REQUIRE( vec_equal(combos[0], {0,1,2}) );
		REQUIRE( vec_equal(combos[1], {0,1,3}) );
		REQUIRE( vec_equal(combos[2], {0,2,3}) );
		REQUIRE( vec_equal(combos[3], {1,2,3}) );
	}
}

TEST_CASE("stream correlator mean and variance", "[correlator]") {
	QDSPStream stream1(1,1,1), stream2(1,2,1);
	uint16_t sid1 = stream1.streamID;
	uint16_t sid2 = stream2.streamID;
	Correlator corr({stream1, stream2}, 2, 1);

	Innovative::Buffer buf( Innovative::Holding<int>(2) );
	Innovative::IntegerDG ibuf(buf);
	const int scale = 1 << 19; //see Accumulator::fixed_to_float

	ibuf[0] = 0 * scale; ibuf[1] = 7 * scale; // segment 1, stream1
    corr.accumulate(sid1, ibuf);
    ibuf[0] = 1 * scale; ibuf[1] = 6 * scale; // segment 1, stream2
    corr.accumulate(sid2, ibuf);
    ibuf[0] = 2 * scale; ibuf[1] = 5 * scale; // segment 2, stream1
    corr.accumulate(sid1, ibuf);
    ibuf[0] = 3 * scale; ibuf[1] = 4 * scale; // segment 2, stream2
    corr.accumulate(sid2, ibuf);

	vector<double> obuf(4);
	vector<double> obufvar(6);

	SECTION("two stream mean correlator from one round robin") {
		corr.snapshot(obuf.data());
		REQUIRE( vec_equal(obuf, {0*1 - 7*6, 0*6 + 1*7, 2*3 - 5*4, 2*4 + 3*5}));
	}

	SECTION("two stream zero variance from one round robin"){
		corr.snapshot_variance(obufvar.data());
		REQUIRE( vec_equal(obufvar, {0, 0, 0, 0, 0, 0}) );
	}

	SECTION("two stream add a second round robin"){
		ibuf[0] = 4 * scale; ibuf[1] = 3 * scale; // segment 1, stream1
		corr.accumulate(sid1, ibuf);
		ibuf[0] = 5 * scale; ibuf[1] = 2 * scale; // segment 1, stream2
		corr.accumulate(sid2, ibuf);
		ibuf[0] = 6 * scale; ibuf[1] = 1 * scale; // segment 2, stream1
		corr.accumulate(sid1, ibuf);
		ibuf[0] = 7 * scale; ibuf[1] = 0 * scale; // segment 2, stream2
		corr.accumulate(sid2, ibuf);

		SECTION("mean after two round robins") {
			corr.snapshot(obuf.data());
			REQUIRE( vec_equal(obuf, {(0*1 - 7*6 + 4*5 - 3*2)/2, (0*6 + 1*7 + 4*2 + 3*5)/2, (2*3 - 5*4 + 6*7 - 1*0)/2, (2*4 + 3*5 + 6*0 + 1*7)/2}) );
		}

		SECTION("variance from two round robins"){
			corr.snapshot_variance(obufvar.data());
			REQUIRE( vec_equal(obufvar, {1568, 128, 448, 1568, 128, -448}) );
		}
	}

	SECTION("three stream correlator mean") {
		QDSPStream stream3(2,1,1);
		uint16_t sid3 = stream3.streamID;
		Correlator corr2({stream1, stream2, stream3}, 1, 1);

		ibuf[0] = 0 * scale; ibuf[1] = 10 * scale; // segment 1, stream1
		corr2.accumulate(sid1, ibuf);
		ibuf[0] = 1 * scale; ibuf[1] = 20 * scale; // segment 1, stream2
		corr2.accumulate(sid2, ibuf);
		ibuf[0] = 2 * scale; ibuf[1] = 30 * scale; // segment 1, stream2
		corr2.accumulate(sid3, ibuf);

		obuf.resize(2);
		corr2.snapshot(obuf.data());
		REQUIRE( vec_equal(obuf, {0*1*2 - 0*20*30 - 10*1*30 - 10*20*2, -10*20*30 + 10*1*2 + 0*20*2 + 0*1*30}) );
	}
}
