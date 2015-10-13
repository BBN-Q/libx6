// QDSPStream.h
//
// Class to represent a data stream from II X6 card.
//
// Original authors: Colm Ryan and Blake Johnson
//
// Copyright 2015, Raytheon BBN Technologies

#ifndef QDSPSTREAM_H_
#define QDSPSTREAM_H_

#include <cstdint>
using std::uint16_t;

enum STREAM_T { PHYSICAL, DEMOD, RESULT };

class QDSPStream{
public:
	QDSPStream();
	QDSPStream(unsigned, unsigned, unsigned);

	unsigned channelID[3];
	uint16_t streamID;
	STREAM_T type;

	unsigned fixed_to_float() const;
};

#endif // QDSPSTREAM_H_
