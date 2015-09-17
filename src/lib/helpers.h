//
// helpers.h
//
// Little helper functions like hex formatting streams
//
// Original authors: Colm Ryan and Blake Johnson
//
// Copyright 2015 Raytheon BBN Technologies

#ifndef HELPERS_H_
#define HELPERS_H_

#include <iomanip> //std::setw

// N-wide hex output with 0x
template <unsigned int N>
inline std::ostream& hexn(std::ostream& out) {
  return out << "0x" << std::hex << std::setw(N) << std::setfill('0');
}

inline int mymod(int a, int b) {
	int c = a % b;
	if (c < 0)
		c += b;
	return c;
}

#endif /* HELPERS_H_ */
