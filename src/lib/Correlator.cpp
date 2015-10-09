// Correlator.cpp
//
// Correlator to correlate data from two or more streams from the QDSP module.
// For now the assumes we correlate only result streams.
//
// Original authors: Colm Ryan and Blake Johnson
//
// Copyright 2015, Raytheon BBN Technologies

#include "Correlator.h"

#include <algorithm> //std::transform

Correlator::Correlator() :
    recordsTaken{0}, wfmCt_{0}, recordLength_{2}, numSegments_{0}, numWaveforms_{0} {};

Correlator::Correlator(const vector<QDSPStream> & streams, const size_t & numSegments, const size_t & numWaveforms) :
                        recordsTaken{0}, wfmCt_{0}, numSegments_{numSegments}, numWaveforms_{numWaveforms} {
    recordLength_ = 2; // assume a RESULT channel
    buffers_.resize(streams.size());
    data_.assign(recordLength_*numSegments, 0);
    idx_ = data_.begin();
    data2_.assign(recordLength_*numSegments*3/2, 0);
    idx2_ = data2_.begin();
    // set up mapping of SIDs to an index into buffers_
    fixed_to_float_ = 1;
    for (size_t i = 0; i < streams.size(); i++) {
        bufferSID_[streams[i].streamID] = i;
        fixed_to_float_ *= 1 << 19; // assumes a RESULT channel, grows with the number of terms in the correlation
    }
};

void Correlator::reset() {
    for (size_t i = 0; i < buffers_.size(); i++)
        buffers_[i].clear();
    data_.assign(recordLength_*numSegments_, 0);
    idx_ = data_.begin();
    data2_.assign(recordLength_*numSegments_*3/2, 0);
    idx2_ = data2_.begin();
    wfmCt_ = 0;
    recordsTaken = 0;
}

void Correlator::correlate() {
    vector<size_t> bufsizes(buffers_.size());
    std::transform(buffers_.begin(), buffers_.end(), bufsizes.begin(), [](vector<int> b) {
        return b.size();
    });
    size_t minsize = *std::min_element(bufsizes.begin(), bufsizes.end());
    if (minsize == 0)
        return;

    // correlate
    // data is real/imag interleaved, so process a pair of points at a time from each channel
    for (size_t i = 0; i < minsize; i += 2) {
        std::complex<double> c = 1;
        for (size_t j = 0; j < buffers_.size(); j++) {
            c *= std::complex<double>(buffers_[j][i], buffers_[j][i+1]);
        }
        c /= fixed_to_float_;
        idx_[0] += c.real();
        idx_[1] += c.imag();
        idx2_[0] += c.real()*c.real();
        idx2_[1] += c.imag()*c.imag();
        idx2_[2] += c.real()*c.imag();

        if (++wfmCt_ == numWaveforms_) {
            wfmCt_ = 0;
            std::advance(idx_, 2);
            std::advance(idx2_, 3);
            if (idx_ == data_.end()) {
                idx_ = data_.begin();
                idx2_ = data2_.begin();
            }
        }
    }
    for (size_t j = 0; j < buffers_.size(); j++)
        buffers_[j].erase(buffers_[j].begin(), buffers_[j].begin()+minsize);

    recordsTaken += minsize/2;
}

size_t Correlator::get_buffer_size() {
    return data_.size();
}

size_t Correlator::get_variance_buffer_size() {
    return data2_.size();
}

void Correlator::snapshot(double * buf) {
    /* Copies current data into a *preallocated* buffer*/
    double N = max(static_cast<int>(recordsTaken / numSegments_), 1);
    for(size_t ct=0; ct < data_.size(); ct++){
        buf[ct] = data_[ct] / N;
    }
}

void Correlator::snapshot_variance(double * buf) {
    int64_t N = max(static_cast<int>(recordsTaken / numSegments_), 1);

    if (N < 2) {
        for(size_t ct=0; ct < data2_.size(); ct++){
            buf[ct] = 0.0;
        }
    } else {
        // construct complex vector of data
        std::complex<double>* cvec = reinterpret_cast<std::complex<double> *>(data_.data());
        // calculate 3 components of variance
        for(size_t ct=0; ct < data_.size()/2; ct++) {
            buf[3*ct] = (data2_[3*ct] - cvec[ct].real()*cvec[ct].real()/N) / (N-1);
            buf[3*ct+1] = (data2_[3*ct+1] - cvec[ct].imag()*cvec[ct].imag()/N) / (N-1);
            buf[3*ct+2] = (data2_[3*ct+2] - cvec[ct].real()*cvec[ct].imag()/N) / (N-1);
        }
    }
}

vector<vector<int>> combinations(int n, int r) {
    /*
     * Returns all combinations of r choices from the list of integers 0,1,...,n-1.
     * Based upon code in the Julia standard library.
     */
    vector<vector<int>> c;
    vector<int> s(r);
    int i;
    if (n < r) return c;
    for (i = 0; i < r; i++)
        s[i] = i;
    c.push_back(s);
    while (s[0] < n - r) {
        for (i = r-1; i >= 0; i--) {
            s[i] += 1;
            if (s[i] > n - r + i)
                continue;
            for (int j = i+1; j < r; j++)
                s[j] = s[j-1] + 1;
            break;
        }
        c.push_back(s);
    }
    return c;
}
