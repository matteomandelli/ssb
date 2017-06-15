cmake_minimum_required (VERSION 3.6)

#project
project (SSB)

# The version number.
set (METRICS_VERSION_MAJOR 0)
set (METRICS_VERSION_MINOR 1)

# Suppress ZERO_CHECK VC project
# ZERO_CHECK will rerun cmake. You can/should execute this after changing something on your CMake files.
set (CMAKE_SUPPRESS_REGENERATION true)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
set (CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11
if(MSVC)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Qpar")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_DEBUG")
	set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
	set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
else()
	set (CMAKE_CXX_FLAGS_DEBUG "-D_DEBUG -g -Wall")
endif()
SET (RES_LINK ${PROJECT_SOURCE_DIR}/src_data/ssb.res)

find_package(PythonInterp) 

# d3d10
include_directories("$(DXSDK_DIR)Include")
link_directories("$(DXSDK_DIR)Lib/x86")

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/src/Config.h.in"
  "${PROJECT_BINARY_DIR}/Config.h"
)

# imgui
include_directories("3rdParty/imgui/")
file(
	GLOB 3rdPartyImGui
		"3rdParty/imgui/imgui.cpp"
		"3rdParty/imgui/imgui_draw.cpp"
		"3rdParty/imgui/imgui_demo.cpp"
		"3rdParty/imgui/*.h"
)
source_group(3rdParty\\ImGui FILES ${3rdPartyImGui})

# SSB
file(GLOB DirSources "src/*.cpp" "src/*.hpp" "src/*.inl")

# Exe
add_executable(${PROJECT_NAME} 
					${DirSources} 
					${3rdPartyImGui}
				)

# Set subsystem to windows and entrypoint to main
if(MSVC)
	set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup ${RES_LINK}")
	set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/data")
endif()

if(MSVC)
	target_link_libraries(${PROJECT_NAME} d3d10.lib)
	target_link_libraries(${PROJECT_NAME} d3dcompiler.lib)
	target_link_libraries(${PROJECT_NAME} dxgi.lib)
	if(WIN32)
		target_link_libraries(${PROJECT_NAME} ws2_32)
	else()
		target_link_libraries(${PROJECT_NAME} ws2_64)
	endif()
endif()
