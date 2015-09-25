#include "catch.hpp"

#include <vector>
using std::vector;

#include "QDSPStream.h"
#include "Accumulator.h"
#include "Correlator.h"

#include <Buffer_Mb.h>
#include <BufferDatagrams_Mb.h>

template <class T>
bool vec_equal(vector<T> a, vector<T> b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

TEST_CASE( "Accumulator mean and variance", "[accumulator]") {

	// Accumulator tests
	Innovative::Buffer buf( Innovative::Holding<int>(2) );
	Innovative::IntegerDG ibuf(buf);

	QDSPStream stream(1,1,1);
	Accumulator accumlator(stream, 1024, 2, 1);
	const int scale = 1 << 19; //see Accumulator::fixed_to_float

	ibuf[0] = 0 * scale; ibuf[1] = 10 * scale; // segment 1
	accumlator.accumulate(ibuf);
	ibuf[0] = 1 * scale; ibuf[1] = 100 * scale; // segment 2
	accumlator.accumulate(ibuf);
	ibuf[0] = 2 * scale; ibuf[1] = 20 * scale; // segment 1
	accumlator.accumulate(ibuf);
	ibuf[0] = 3 * scale; ibuf[1] = 200 * scale; // segment 2
	accumlator.accumulate(ibuf);

	vector<double> obuf(4);
	vector<double> obufvar(6);

	SECTION("mean of two round robins") {
        accumlator.snapshot(obuf.data());
		REQUIRE( vec_equal(obuf, {1, 15, 2, 150}) );
	}

	SECTION("variance of two round robins") {
		accumlator.snapshot_variance(obufvar.data());
		REQUIRE( vec_equal(obufvar, {2, 50, 10, 2, 5000, 100}) );
	}

	SECTION("add additional round robin") {
		ibuf[0] = 4 * scale; ibuf[1] = 30 * scale; // segment 1
	    accumlator.accumulate(ibuf);
	    ibuf[0] = 5 * scale; ibuf[1] = 300 * scale; // segment 2
	    accumlator.accumulate(ibuf);

		SECTION("mean of three round robins") {
	        accumlator.snapshot(obuf.data());
			REQUIRE( vec_equal(obuf, {2, 20, 3, 200}));
		}

		SECTION("variance of three round robins") {
	        accumlator.snapshot_variance(obufvar.data());
			REQUIRE( vec_equal(obufvar, {4, 100, 20, 4, 10000, 200}));
		}
	}
}
