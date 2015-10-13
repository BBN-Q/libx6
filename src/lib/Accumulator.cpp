// Accumulator.cpp
//
// Accumulator to sum data streamed from the QDSP module.
//
// Original authors: Colm Ryan and Blake Johnson
//
// Copyright 2015, Raytheon BBN Technologies

#include "Accumulator.h"
#include "constants.h"

Accumulator::Accumulator() :
    recordsTaken{0}, wfmCt_{0}, numSegments_{0}, numWaveforms_{0}, recordLength_{0} {};

Accumulator::Accumulator(const QDSPStream & stream, const size_t & recordLength, const size_t & numSegments, const size_t & numWaveforms) :
                         recordsTaken{0}, stream_{stream}, wfmCt_{0}, numSegments_{numSegments}, numWaveforms_{numWaveforms} {
    recordLength_ = calc_record_length(stream, recordLength);
    data_.assign(recordLength_*numSegments, 0);
    idx_ = data_.begin();
    if (stream.type == PHYSICAL) {
        data2_.assign(recordLength_*numSegments_, 0);
    } else {
        // complex data, so 3-component correlations (real*real, imag*imag, real*imag)
        data2_.assign(recordLength_*numSegments*3/2, 0);
    }
    idx2_ = data2_.begin();
    fixed_to_float_ = stream.fixed_to_float();
};

void Accumulator::reset() {
    data_.assign(recordLength_*numSegments_, 0);
    idx_ = data_.begin();
    std::fill(data2_.begin(), data2_.end(), 0);
    idx2_ = data_.begin();
    wfmCt_ = 0;
    recordsTaken = 0;
}

size_t Accumulator::calc_record_length(const QDSPStream & stream, const size_t & recordLength) {
    switch (stream.type) {
        case PHYSICAL:
            return recordLength / RAW_DECIMATION_FACTOR;
            break;
        case DEMOD:
            return 2 * recordLength / DEMOD_DECIMATION_FACTOR;
            break;
        case RESULT:
            return 2;
            break;
        default:
            return 0;
    }
}

size_t Accumulator::get_buffer_size() {
    return data_.size();
}

size_t Accumulator::get_variance_buffer_size() {
    return data2_.size();
}

void Accumulator::snapshot(double * buf) {
    /* Copies current data into a *preallocated* buffer*/
    double scale = max(static_cast<int>(recordsTaken), 1) / numSegments_ * fixed_to_float_;
    for(size_t ct=0; ct < data_.size(); ct++){
        buf[ct] = static_cast<double>(data_[ct]) / scale;
    }
}

void Accumulator::snapshot_variance(double * buf) {
    int64_t N = max(static_cast<int>(recordsTaken / numSegments_), 1);
    double scale = (N-1) * fixed_to_float_ * fixed_to_float_;

    if (N < 2) {
        for(size_t ct=0; ct < data2_.size(); ct++){
            buf[ct] = 0.0;
        }
    } else if (stream_.type == PHYSICAL) {
        for (size_t ct = 0; ct < data2_.size(); ct++) {
            buf[ct] = static_cast<double>(data2_[ct] - data_[ct]*data_[ct]/N) / scale;
        }
    } else {
        // construct complex vector of data
        std::complex<int64_t>* cvec = reinterpret_cast<std::complex<int64_t> *>(data_.data());
        // calculate 3 components of variance
        for(size_t ct=0; ct < data_.size()/2; ct++) {
            buf[3*ct] = static_cast<double>(data2_[3*ct] - cvec[ct].real()*cvec[ct].real()/N) / scale;
            buf[3*ct+1] = static_cast<double>(data2_[3*ct+1] - cvec[ct].imag()*cvec[ct].imag()/N) / scale;
            buf[3*ct+2] = static_cast<double>(data2_[3*ct+2] - cvec[ct].real()*cvec[ct].imag()/N) / scale;
        }
    }
}
