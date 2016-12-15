// RecordQueue.h
//
// Queue to buffer records streamed from the QDSP module.
//
// Original authors: Colm Ryan and Blake Johnson
//
// Copyright 2015, Raytheon BBN Technologies


#ifndef RECORDQUEUE_H_
#define RECORDQUEUE_H_

#include <queue>
#include <atomic>
#include <cstring>
#include <BufferDatagrams_Mb.h>

#include <unistd.h>
#ifdef WIN32
	#include <winsock2.h>
#else
	#include <sys/socket.h>
#endif

#include "QDSPStream.h"
#include "logger.h"


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
	int32_t socket_ = -1;

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

	// if we have a socket, process the data and send it immediately
	if (socket != -1) {
		size_t buf_size = buffer.size() * sizeof(T);
		if (send(socket_, reinterpret_cast<char *>(buf_size), sizeof(size_t), 0) < 0) {
			FILE_LOG(logERROR) << "Error writing buffer size to socket,"
			                   << " received error " << std::strerror(errno);
			throw X6_SOCKET_ERROR;
		}
		char *buf_ptr = reinterpret_cast<char *>(buffer.Ptr());
		ssize_t status = send(socket_, buf_ptr, buf_size, 0);
		if (status < 0) {
			FILE_LOG(logERROR) << "System error writing to socket: " << std::strerror(errno);
			throw X6_SOCKET_ERROR;
		} else if (status != static_cast<ssize_t>(buf_size)) {
			FILE_LOG(logERROR) << "Error writing stream ID " << stream_.streamID
			                   << " buffer to socket. Tried to write "
			                   << buf_size " bytes, actually wrote "
			                   << status << " bytes";
			throw X6_SOCKET_ERROR;
		}
	} else {
		// otherwise, store for later retrieval
		for (auto val : buffer) {
			queue_.push(val);
		}
		availableRecords++;
	}

	recordsTaken++;
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
