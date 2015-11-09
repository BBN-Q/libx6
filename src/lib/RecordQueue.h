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
#include <atomic>

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

	std::atomic<size_t> recordsTaken;
	std::atomic<size_t> availableRecords;
	size_t recordLength;

private:
	std::queue<T> queue_;
	QDSPStream stream_;
	unsigned fixed_to_float_;
};


template <class T>
RecordQueue<T>::RecordQueue() : recordsTaken{0}, availableRecords{0}  {}

template <class T>
RecordQueue<T>::RecordQueue(const QDSPStream & stream, size_t recLen) : recordsTaken{0}, availableRecords{0} {
	recordLength = stream.calc_record_length(recLen);
	fixed_to_float_ = stream.fixed_to_float();
	stream_ = stream;
}


template <class T>
template <class U>
void RecordQueue<T>::push(const Innovative::AccessDatagram<U> & buffer) {
	//TODO: worry about performance, cache-friendly etc.
	FILE_LOG(logDEBUG3) << "Buffering data...";
	FILE_LOG(logDEBUG3) << "recordsTaken = " << recordsTaken;
	FILE_LOG(logDEBUG3) << "New buffer size is " << buffer.size();
	FILE_LOG(logDEBUG3) << "queue size is " << queue_.size();

	for (auto val : buffer) {
		queue_.push(val);
	}
    recordsTaken++;
    availableRecords++;
}

template <class T>
void RecordQueue<T>::get(double * buf, size_t numPoints) {
	size_t initialSize = queue_.size();
	availableRecords -= numPoints / recordLength;
	// for(size_t ct=0; !queue_.empty() && (ct < numPoints); ct++) {
	for(size_t ct=0; ct < numPoints; ct++) {
		if (queue_.empty()) {
			FILE_LOG(logERROR) << "Tried to pull " << numPoints << " from a queue of initial size " << initialSize;
			FILE_LOG(logERROR) << "Tried to pull an empty queue.";
			break;
		}
		buf[ct] = static_cast<double>(queue_.front()) / fixed_to_float_;
		queue_.pop();
	}
}

template <class T>
size_t RecordQueue<T>::get_buffer_size() {
	return availableRecords * recordLength;
}
#endif //RECORDQUEUE_H_
