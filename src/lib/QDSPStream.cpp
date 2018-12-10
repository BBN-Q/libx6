// QDSPStream.cpp
//
// Class to represent a data stream from the QDSP module on the II X6 card of
// either raw, demodulated or integrated results.
//
// Original authors: Colm Ryan and Blake Johnson
//
// Copyright 2015, Raytheon BBN Technologies

#include "QDSPStream.h"
#include "constants.h"

QDSPStream::QDSPStream() : channelID{0,0,0}, streamID{0}, type{PHYSICAL} {};

QDSPStream::QDSPStream(unsigned a, unsigned b, unsigned c) : channelID{a,b,c} {
    streamID = (a << 8) + (b << 4) + c;
    if ((b == 0) && (c == 0)) {
        type = PHYSICAL;
    }
    else if (c != 0) {
        if(c >= 6 && c <= 10)
        {
            type = STATE;
        }
        else if(c > 10)
        {
            type = CORRELATED;
        }
        else
        {
            type = RESULT;
        }
    }
    else {
        type = DEMOD;
    }
};

QDSPStream::QDSPStream(unsigned a, unsigned b, unsigned c, unsigned numRawInt) : channelID{a,b,c} {
    streamID = (a << 8) + (b << 4) + c;
    if ((b == 0) && (c == 0)) {
        type = PHYSICAL;
    }
    else if (c != 0) {
        if(c >= numRawInt+1 && c <= 2*numRawInt)
        {
            type = STATE;
        }
        else if(c > 2*numRawInt)
        {
            type = CORRELATED;
        }
        else
        {
            type = RESULT;
        }
    }
    else {
        type = DEMOD;
    }
};

unsigned QDSPStream::fixed_to_float() const {
    switch (type) {
        case PHYSICAL:
            return 1 << 13; // signed 12-bit integers from ADC and then four samples summed
            break;
        case DEMOD:
            return 1 << 14;
            break;
        case RESULT:
        case CORRELATED:
            if (channelID[1]) {
                return 1 << 19;
            }
            else {
                return 1 << 15;
            }
            break;
        case STATE:
            return 1;
        default:
            return 0;
    }
}

size_t QDSPStream::calc_record_length(const size_t & recordLength) const {
    switch (type) {
        case PHYSICAL:
            return recordLength / RAW_DECIMATION_FACTOR;
            break;
        case DEMOD:
            return 2 * recordLength / DEMOD_DECIMATION_FACTOR;
            break;
        case RESULT:
        case STATE:
        case CORRELATED:
            return 2;
            break;
        default:
            return 0;
    }
}
