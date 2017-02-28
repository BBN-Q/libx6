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

#ifdef _WIN32
	#if defined(_MSC_VER)
		#include <BaseTsd.h>
		typedef SSIZE_T ssize_t;
	#endif
	#include <winsock2.h>
#else
	#include <sys/socket.h>
#endif

#include <BufferDatagrams_Mb.h>
#include "QDSPStream.h"
#include "logger.h"
#include "X6_errno.h"


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

	std::vector<double> workbuf_;
	template <class U>
	std::vector<double>& convert_to_double(const Innovative::AccessDatagram<U> &);
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
	if (socket_ != -1) {
		std::vector<double>& workbuf = convert_to_double(buffer);
		size_t buf_size = buffer.size() * sizeof(double);
		ssize_t status = send(socket_, reinterpret_cast<char *>(&buf_size), sizeof(size_t), 0);
		if (status < 0) {
			FILE_LOG(logERROR) << "Error writing buffer size to socket,"
			#ifdef _WIN32
			                   << " received error: " << WSAGetLastError();
			#else
			                   << " received error: " << std::strerror(errno);
			#endif
			throw X6_SOCKET_ERROR;
		}

		status = send(socket_, reinterpret_cast<char *>(workbuf.data()), buf_size, 0);
		if (status < 0) {
			FILE_LOG(logERROR) << "System error writing to socket: "
			#ifdef _WIN32
			                   << WSAGetLastError();
			#else
			                   << std::strerror(errno);
			#endif
			throw X6_SOCKET_ERROR;
		} else if (status != static_cast<ssize_t>(buf_size)) {
			FILE_LOG(logERROR) << "Error writing stream ID " << stream_.streamID
			                   << " buffer to socket. Tried to write "
			                   << buf_size << " bytes, actually wrote "
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

template <class T>
template <class U>
std::vector<double>& RecordQueue<T>::convert_to_double(const Innovative::AccessDatagram<U> &buffer) {
	workbuf_.resize(buffer.size());
	for (size_t ct = 0; ct < buffer.size(); ct++) {
		workbuf_[ct] = static_cast<double>(buffer[ct]) / fixed_to_float_;
	}
	return workbuf_;
}

#endif //RECORDQUEUE_H_
