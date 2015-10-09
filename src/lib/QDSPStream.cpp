// QDSPStream.cpp
//
// Class to represent a data stream from the QDSP module on the II X6 card of
// either raw, demodulated or integrated results.
//
// Original authors: Colm Ryan and Blake Johnson
//
// Copyright 2015, Raytheon BBN Technologies

#include "QDSPStream.h"

QDSPStream::QDSPStream() : channelID{0,0,0}, streamID{0}, type{PHYSICAL} {};

QDSPStream::QDSPStream(unsigned a, unsigned b, unsigned c) : channelID{a,b,c} {
    streamID = (a << 8) + (b << 4) + c;
    if ((b == 0) && (c == 0)) {
        type = PHYSICAL;
    } else if (c != 0) {
        type = RESULT;
    } else {
        type = DEMOD;
    }
};
