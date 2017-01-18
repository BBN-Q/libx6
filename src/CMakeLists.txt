project( libx6 )

cmake_minimum_required(VERSION 3.1)

# Make sure we have C++11 support
set(CMAKE_CXX_STANDARD 11)

# find where Innovative is installed
if(UNIX)
	set(INNOVATIVE_PATH /usr/Innovative CACHE FILEPATH "location of Innovative install")
else()
	set(INNOVATIVE_PATH $ENV{InnovativeCommon} CACHE FILEPATH "location of Innovative install")
endif()
message("Using ${INNOVATIVE_PATH} as Innovative install directory")

#include Malibu headers
include_directories(${INNOVATIVE_PATH}/Malibu)

#and our own library headers
include_directories("./lib/")

#Catch unit-testing
include_directories("../deps/Catch/include")

#Innovative shipped shared libraries
if(MSVC)
	link_directories(${INNOVATIVE_PATH}/Lib/Vc14_x64)
else()
	link_directories(${INNOVATIVE_PATH}/Lib/Gcc)
	if(CMAKE_BUILD_TYPE MATCHES Debug)
		message("Building and linking Debug")
		link_directories(${INNOVATIVE_PATH}/Lib/Gcc/Debug)
	else()
		message("Building and linking Release")
		link_directories(${INNOVATIVE_PATH}/Lib/Gcc/Release)
	endif()
endif()

#Intel IPP shared libraries
if(UNIX)
	set(IPP_LIB ipps ippcore)
else()
	link_directories(${INNOVATIVE_PATH}/Lib/Ipp/Win64)
	set(IPP_LIB ipps-9.0)
endif()

# warn all for better code
add_definitions(${CMAKE_CXX_FLAGS} "-Wall")

add_custom_target( update_version
	COMMAND ${PROJECT_SOURCE_DIR}/make_version.sh
)

set ( DLL_SRC
	./lib/libx6.cpp
	./lib/QDSPStream.cpp
	./lib/Accumulator.cpp
	./lib/Correlator.cpp
	./lib/X6_1000.cpp
)

set_source_files_properties( ${DLL_SRC} PROPERTIES LANGUAGE CXX )

add_library( x6 SHARED ${DLL_SRC} )
add_dependencies( x6 update_version )

add_executable(run_tests
	../test/run_tests.cpp
	../test/test_Sanity.cpp
	../test/test_Accumulator.cpp
	../test/test_Correlator.cpp
	../test/test_libx6.cpp
	./lib/QDSPStream.cpp
	./lib/Accumulator.cpp
	./lib/Correlator.cpp
)

set ( II_LIBS
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

target_link_libraries(run_tests
	${II_LIBS}
	wdapi
	pthread
	x6
)

target_link_libraries(x6
	${II_LIBS}
	wdapi # WinDriver library
	dl
	${IPP_LIB} # in Intel IPP directory listed above
)

if(WIN32)
	target_link_libraries(x6
		iphlpapi # II's PocoFoundation needs GetAdaptersAddress on Windows
		ws2_32
	)
endif(WIN32)

install(TARGETS x6 DESTINATION lib)
install(FILES "${PROJECT_SOURCE_DIR}/lib/libx6.h" DESTINATION include)
