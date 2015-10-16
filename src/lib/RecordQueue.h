// RecordQueue.h
//
// Queue to buffer records streamed from the QDSP module.
//
// Original authors: Colm Ryan and Blake Johnson
//
// Copyright 2015, Raytheon BBN Technologies


#ifndef RECORDQUEUE_H_
#define RECORDQUEUE_H_

#include "QDSPStream.h"

#include <queue>

#include "logger.h"

#include <BufferDatagrams_Mb.h>

template <class T>
class RecordQueue {

public:

	RecordQueue<T>();
	RecordQueue<T>(const QDSPStream &, size_t);

	template <class U>
	void push(const Innovative::AccessDatagram<U> &);
	void get(double *, size_t);
	size_t get_buffer_size();

	size_t recordsTaken = 0;
	size_t recordLength;

private:
	std::queue<T> queue_;
	QDSPStream stream_;
	unsigned fixed_to_float_;
};


template <class T>
RecordQueue<T>::RecordQueue() {}

template <class T>
RecordQueue<T>::RecordQueue(const QDSPStream & stream, size_t recordLength) {
	recordLength = stream.calc_record_length(recordLength);
	fixed_to_float_ = stream.fixed_to_float();
	stream_ = stream;
}


template <class T>
template <class U>
void RecordQueue<T>::push(const Innovative::AccessDatagram<U> & buffer) {
	//TODO: worry about performance, cache-friendly etc.
	FILE_LOG(logDEBUG4) << "Buffering data...";
	FILE_LOG(logDEBUG4) << "recordsTaken = " << recordsTaken;
	FILE_LOG(logDEBUG4) << "New buffer size is " << buffer.size();
	FILE_LOG(logDEBUG4) << "queue size is " << queue_.size();

	for (auto val : buffer) {
		queue_.push(val);
	}
    recordsTaken++;
}

template <class T>
void RecordQueue<T>::get(double * buf, size_t numPoints) {
	for(size_t ct=0; !queue_.empty() && (ct < numPoints); ct++) {
		buf[ct] = static_cast<double>(queue_.front()) / fixed_to_float_;
		queue_.pop();
	}
}

template <class T>
size_t RecordQueue<T>::get_buffer_size() {
	return queue_.size();
}
#endif //RECORDQUEUE_H_
