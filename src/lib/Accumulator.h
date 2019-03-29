// Accumulator.h
//
// Accumulator to sum data streamed from the QDSP module.
//
// Original authors: Colm Ryan and Blake Johnson
//
// Copyright 2015, Raytheon BBN Technologies


#ifndef ACCUMULATOR_H_
#define ACCUMULATOR_H_

#include "QDSPStream.h"

#include <algorithm> //std::transform
#include <vector>
using std::vector;
using std::max;

#include <plog/Log.h>

#include <BufferDatagrams_Mb.h>


class Accumulator{

public:
	/* Helper class to accumulate/average data */
	Accumulator();
	Accumulator(const QDSPStream &, const size_t &, const size_t &, const size_t &);
	template <class T>
	void accumulate(const Innovative::AccessDatagram<T> &);

	void reset();
	void snapshot(double *);
	void snapshot_variance(double *);
	size_t get_buffer_size();
	size_t get_variance_buffer_size();
	size_t recordsTaken;

private:
	QDSPStream stream_;
	size_t wfmCt_;
	size_t numSegments_;
	size_t numWaveforms_;
	size_t recordLength_;
	unsigned fixed_to_float_;

	vector<int64_t> data_;
	vector<int64_t>::iterator idx_;
	// second data object to store the square of the data
	vector<int64_t> data2_;
	vector<int64_t>::iterator idx2_;
};

template <class T>
void Accumulator::accumulate(const Innovative::AccessDatagram<T> & buffer) {
    //TODO: worry about performance, cache-friendly etc.
    LOG(plog::debug) << "Accumulating data...";
    LOG(plog::debug) << "recordLength_ = " << recordLength_ << "; idx_ = " << std::distance(data_.begin(), idx_) << "; recordsTaken = " << recordsTaken;
    LOG(plog::debug) << "New buffer size is " << buffer.size();
    LOG(plog::debug) << "Accumulator buffer size is " << data_.size();

    // The assumption is that this will be called with a full record size
    // Accumulate the buffer into data_
    std::transform(idx_, idx_+recordLength_, buffer.begin(), idx_, std::plus<int64_t>());
    // record the square of the buffer as well
    if (stream_.type == PHYSICAL) {
        // data is real, just square and sum it.
        std::transform(idx2_, idx2_+recordLength_, buffer.begin(), idx2_, [](int64_t a, int64_t b) {
            return a + b*b;
        });
    } else {
        // data is complex: real/imaginary are interleaved every other point
        // form a complex vector from the input buffer
        vector<std::complex<int64_t>> cvec(recordLength_/2);
        for (size_t i = 0; i < recordLength_/2; i++) {
            cvec[i] = std::complex<int64_t>(buffer[2*i], buffer[2*i+1]);
        }
        // calculate 3-component correlations into a triple of successive points
        for (size_t i = 0; i < cvec.size(); i++) {
            idx2_[3*i] += cvec[i].real() * cvec[i].real();
            idx2_[3*i+1] += cvec[i].imag() * cvec[i].imag();
            idx2_[3*i+2] += cvec[i].real() * cvec[i].imag();
        }
    }
    recordsTaken++;

    //If we've filled up the number of waveforms move onto the next segment, otherwise jump back to the beginning of the record
    if (++wfmCt_ == numWaveforms_) {
        wfmCt_ = 0;
        std::advance(idx_, recordLength_);
        if (stream_.type == PHYSICAL)
            std::advance(idx2_, recordLength_);
        else
            std::advance(idx2_, recordLength_ * 3/2);
    }

    //Final check if we're at the end
    if (idx_ == data_.end()) {
        idx_ = data_.begin();
        idx2_ = data2_.begin();
    }
}

#endif // ACCUMULATOR_H_
