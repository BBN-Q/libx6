# build using
# mkdir build
# cd build
# cmake -G "MSYS Makefiles" -DINNOVATIVE_PATH="C:\\Innovative" ../src/  # for Windows
# cmake -DINNOVATIVE_PATH=/home/cryan/Downloads/II ../src   # for Linux
# make


PROJECT( libx6adc )

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

#Malibu shared libraries
if(WIN32)
	SET(INNOVATIVE_LIB_PATH ${INNOVATIVE_PATH}/Lib)
else()
	SET(INNOVATIVE_LIB_PATH ${INNOVATIVE_PATH}//usr/Innovative/Malibu-1.8/Lib)
endif()

LINK_DIRECTORIES(${INNOVATIVE_LIB_PATH}/Gcc)
LINK_DIRECTORIES( ${INNOVATIVE_LIB_PATH}/Gcc/Debug)

#Intel IPP shared libraries
if(UNIX)
	LINK_DIRECTORIES(${INNOVATIVE_PATH}/opt/intel/composer_xe_2011_sp1.9.293/ipp/lib/intel64)
	SET(IPP_LIB "ipps")
else()
	LINK_DIRECTORIES(${INNOVATIVE_PATH}/Lib/Ipp/Win64)
	SET(IPP_LIB "ipps-7.0")
endif()

if(UNIX)
	LINK_DIRECTORIES( ${INNOVATIVE_PATH}/usr/Innovative/WinDriver-11.8/lib)
endif()

#Some compile flags
ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-std=c++11 -Wall")

SET ( DLL_SRC
	./lib/libx6adc.cpp
	./lib/X6_1000.cpp
)

SET_SOURCE_FILES_PROPERTIES( ${DLL_SRC} PROPERTIES LANGUAGE CXX )

ADD_LIBRARY( x6adc SHARED ${DLL_SRC} )

ADD_EXECUTABLE(x6test ../test/main.cpp )
TARGET_LINK_LIBRARIES(x6test x6adc)

ADD_EXECUTABLE(unit
	../test/unit.cpp
	./lib/X6_1000.cpp
)
TARGET_LINK_LIBRARIES(unit x6adc)

TARGET_LINK_LIBRARIES(x6adc
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

	wdapi # WinDriver library in Innovative\lib\gcc
	${IPP_LIB} # in Intel IPP directory listed above

	# The following libraries are not currently required
	#Framework_Win
	#Vpx_Mb  ##
)

if(WIN32)
	TARGET_LINK_LIBRARIES(x6adc
		iphlpapi # II's PocoFoundation needs GetAdaptersAddress on Windows
	)
endif(WIN32)
