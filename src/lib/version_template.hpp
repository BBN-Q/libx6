#ifndef VERSION_H
#define VERSION_H

#include <string>
using std::string;

const string LIBX6_VERSION = "@GIT_DESCRIBE@";
string get_driver_version() {
	return LIBX6_VERSION;
}

#endif
