PROJECT( libx6 )

CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

# Make sure we have C++11 support
set (CMAKE_CXX_STANDARD 11)

# find where Innovative is installed
if(UNIX)
	set(INNOVATIVE_PATH /usr/Innovative CACHE FILEPATH "location of Innovative install")
else()
	set(INNOVATIVE_PATH $ENV{InnovativeCommon} CACHE FILEPATH "location of Innovative install")
endif()
message("Using ${INNOVATIVE_PATH} as Innovative install directory")

#include Malibu headers
INCLUDE_DIRECTORIES(${INNOVATIVE_PATH}/Malibu)

#and our own library headers
INCLUDE_DIRECTORIES("./lib/")

#Catch unit-testing
INCLUDE_DIRECTORIES("../deps/Catch/include")

#Innovative shipped shared libraries
LINK_DIRECTORIES(${INNOVATIVE_PATH}/Lib/Gcc)
if(CMAKE_BUILD_TYPE MATCHES Debug)
	message("Building and linking Debug")
	LINK_DIRECTORIES(${INNOVATIVE_PATH}/Lib/Gcc/Debug)
else()
	message("Building and linking Release")
	LINK_DIRECTORIES(${INNOVATIVE_PATH}/Lib/Gcc/Release)
endif()

#Intel IPP shared libraries
if(UNIX)
	SET(IPP_LIB ipps ippcore)
else()
	LINK_DIRECTORIES(${INNOVATIVE_PATH}/Lib/Ipp/Win64)
	SET(IPP_LIB ipps-9.0)
endif()

# warn all for better code
ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-Wall")

ADD_CUSTOM_TARGET( update_version
	COMMAND ${PROJECT_SOURCE_DIR}/make_version.sh
)

SET ( DLL_SRC
	./lib/libx6.cpp
	./lib/QDSPStream.cpp
	./lib/Accumulator.cpp
	./lib/Correlator.cpp
	./lib/X6_1000.cpp
)

SET_SOURCE_FILES_PROPERTIES( ${DLL_SRC} PROPERTIES LANGUAGE CXX )

ADD_LIBRARY( x6 SHARED ${DLL_SRC} )
ADD_DEPENDENCIES( x6 update_version )

ADD_EXECUTABLE(run_tests
	../test/run_tests.cpp
	../test/test_Sanity.cpp
	../test/test_Accumulator.cpp
	../test/test_Correlator.cpp
	../test/test_libx6.cpp
	./lib/QDSPStream.cpp
	./lib/Accumulator.cpp
	./lib/Correlator.cpp
)

SET ( II_LIBS
	-Wl,--start-group
	Os_Mb
	Application_Mb
	Analysis_Mb
	Framework_Con
	Framework_Mb
	Utility_Mb
	Hardware_Mb
	Pci_Mb
	Ficl_Mb
	Poco_Foundation_Mb
	Poco_Net_Mb
	-Wl,--end-group
)

TARGET_LINK_LIBRARIES(run_tests
	${II_LIBS}
	wdapi
	pthread
	x6
)

TARGET_LINK_LIBRARIES(x6
	${II_LIBS}
	wdapi # WinDriver library
	dl
	${IPP_LIB} # in Intel IPP directory listed above
)

if(WIN32)
	TARGET_LINK_LIBRARIES(x6
		iphlpapi # II's PocoFoundation needs GetAdaptersAddress on Windows
		ws2_32
	)
endif(WIN32)

install(TARGETS x6 DESTINATION lib)
install(FILES "${PROJECT_SOURCE_DIR}/lib/libx6.h" DESTINATION include)
