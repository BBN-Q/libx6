# build using
# mkdir build
# cd build
# cmake -G "MSYS Makefiles" -DINNOVATIVE_PATH="C:\\Innovative" ../src/  # for Windows
# cmake -DINNOVATIVE_PATH=/home/cryan/Downloads/II ../src   # for Linux
# make


PROJECT( libx6 )

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET ( CMAKE_BUILD_TYPE Debug )

#include Malibu headers
if(WIN32)
	INCLUDE_DIRECTORIES(${INNOVATIVE_PATH}/Malibu)
else()
	INCLUDE_DIRECTORIES(${INNOVATIVE_PATH}/usr/Innovative/Malibu-1.8/Malibu)
endif()

#and our own library headers
INCLUDE_DIRECTORIES("./lib/")

#Catch unit-testing
INCLUDE_DIRECTORIES("../deps/Catch/include")

#Malibu shared libraries
if(WIN32)
	SET(INNOVATIVE_LIB_PATH ${INNOVATIVE_PATH}/Lib)
else()
	SET(INNOVATIVE_LIB_PATH ${INNOVATIVE_PATH}/usr/Innovative/Malibu-1.8/Lib)
endif()

LINK_DIRECTORIES(${INNOVATIVE_LIB_PATH}/Gcc)
LINK_DIRECTORIES(${INNOVATIVE_LIB_PATH}/Gcc/Debug)

#Intel IPP shared libraries
if(UNIX)
	LINK_DIRECTORIES(${INNOVATIVE_PATH}/opt/intel/composer_xe_2011_sp1.9.293/ipp/lib/intel64)
	SET(IPP_LIB "ipps")
else()
	LINK_DIRECTORIES(${INNOVATIVE_PATH}/Lib/Ipp/Win64)
	SET(IPP_LIB "ipps-9.0")
endif()

if(UNIX)
	LINK_DIRECTORIES( ${INNOVATIVE_PATH}/usr/Innovative/WinDriver-12.1/lib)
endif()

#Some compile flags
ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-std=c++11 -Wall")

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
	wdapi # WinDriver library in Innovative\lib\gcc
	${IPP_LIB} # in Intel IPP directory listed above
)

if(WIN32)
	TARGET_LINK_LIBRARIES(x6
		iphlpapi # II's PocoFoundation needs GetAdaptersAddress on Windows
		ws2_32
	)
endif(WIN32)
