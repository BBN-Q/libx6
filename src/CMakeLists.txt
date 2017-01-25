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
if(CMAKE_BUILD_TYPE MATCHES Debug)
	message("Building and linking Debug")
	if(MSVC)
		link_directories(${INNOVATIVE_PATH}/Lib/Vc14_x64)
		set(II_MSVC_LIB_SUFFIX _DLL_Debug)
	else()
		link_directories(${INNOVATIVE_PATH}/Lib/Gcc)
		link_directories(${INNOVATIVE_PATH}/Lib/Gcc/Debug)
	endif()
else()
	message("Building and linking Release")
	if(MSVC)
		link_directories(${INNOVATIVE_PATH}/Lib/Vc14_x64)
		set(II_MSVC_LIB_SUFFIX _DLL)
	else()
		link_directories(${INNOVATIVE_PATH}/Lib/Gcc)
		link_directories(${INNOVATIVE_PATH}/Lib/Gcc/Release)
	endif()
endif()

#Intel IPP shared libraries
if(WIN32 AND NOT MSVC)
	set(IPP_LIB ipps-9.0)
else()
	set(IPP_LIB ippcore ipps)
endif()
if(MSVC)
	link_directories(${INNOVATIVE_PATH}/Lib/Vc_Ipp_x64)
endif()


# don't let windows.h clobber std::min and std::max
if(MSVC)
    add_definitions(-DNOMINMAX)
endif()

# warn all for better code but disable some Visual Studio ones for now
# warning C4244: 'initializing': conversion from 'unsigned int' to 'unsigned short', possible loss of data
# warning C4267: 'argument': conversion from 'size_t' to 'uint32_t', possible loss of data
# warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead.
#     however can't use "_s" safe versions with glibc
if(MSVC)
	add_definitions(${CMAKE_CXX_FLAGS} "/W3 /wd4244 /wd4267 /wd4996")
else()
add_definitions(${CMAKE_CXX_FLAGS} "-Wall")
endif()

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
# force lib prefix even on Windows
set_target_properties(x6 PROPERTIES PREFIX "lib")

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
)

# linux and mingw-64
if(NOT MSVC)
	target_link_libraries( x6
		${IPP_LIB} # in Intel IPP directory listed above
		-Wl,--start-group
		${II_LIBS}
		-Wl,--end-group
		wdapi
	)
	# on Linux wdapi needs dl to dynamically load
	if(UNIX)
		target_link_libraries(x6 dl)
	endif()
endif()

if(MSVC)
	list(APPEND II_LIBS Framework_Win)

	target_link_libraries(x6 Kernel_Mb)

	foreach(lib ${II_LIBS})
		target_link_libraries(x6 ${lib}${II_MSVC_LIB_SUFFIX})
	endforeach()

	target_link_libraries(x6
		${IPP_LIB}
		wdapi
	)
endif()

if(WIN32)
	target_link_libraries(x6
		iphlpapi # II's PocoFoundation needs GetAdaptersAddress on Windows
		ws2_32
	)
endif()

target_link_libraries(run_tests
	x6
	pthread
)

install(TARGETS x6 DESTINATION lib)
install(FILES "${PROJECT_SOURCE_DIR}/lib/libx6.h" DESTINATION include)
