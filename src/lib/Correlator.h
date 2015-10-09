// Correlator.h
//
// Correlator to correlate data from two or more streams from the QDSP module.
// For now the assumes we correlate only result streams.
//
// Original authors: Colm Ryan and Blake Johnson
//
// Copyright 2015, Raytheon BBN Technologies

#ifndef CORRELATOR_H_
#define CORRELATOR_H_

#include <vector>
using std::vector;
#include <cstddef>

#include "QDSPStream.h"

#include <BufferDatagrams_Mb.h>

class Correlator {
public:
	Correlator();
	Correlator(const vector<QDSPStream> &, const size_t &, const size_t &);
	template <class T>
	void accumulate(const int &, const Innovative::AccessDatagram<T> &);
	void correlate();

	void reset();
	void snapshot(double *);
	void snapshot_variance(double *);
	size_t get_buffer_size();
	size_t get_variance_buffer_size();

	size_t recordsTaken;

private:
	size_t wfmCt_;
	size_t recordLength_;
	size_t numSegments_;
	size_t numWaveforms_;
	int64_t fixed_to_float_;

	// buffers for raw data from the channels
	vector<vector<int>> buffers_;
	map<uint16_t, int> bufferSID_;

	// buffer for the correlated values A*B(*C*D*...)
	vector<double> data_;
	vector<double>::iterator idx_;
	// buffer for (A*B)^2
	vector<double> data2_;
	vector<double>::iterator idx2_;
};

vector<vector<int>> combinations(int, int);

template <class T>
void Correlator::accumulate(const int & sid, const Innovative::AccessDatagram<T> & buffer) {
    // copy the data
    for (size_t i = 0; i < buffer.size(); i++)
        buffers_[bufferSID_[sid]].push_back(buffer[i]);
    correlate();
}

#endif // CORRELATOR_H_
