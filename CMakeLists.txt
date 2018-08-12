cmake_minimum_required(VERSION 2.8.3)

#project(camaas)

if (CMAKE_BUILD_TYPE MATCHES Release)

add_definitions(-O3)
add_definitions(-g)

endif()

if (CMAKE_BUILD_TYPE MATCHES Debug)

add_definitions(-g)

endif()

SET(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wno-ignored-attributes -Wno-misleading-indentation -Wno-noexcept-type")

SET(CMAKE_INSTALL_RPATH ${PROJECT_BINARY_DIR}/bin)

set(BIN ${PROJECT_BINARY_DIR}/bin)

set(INCLUDES "include")

list(REMOVE_DUPLICATES INCLUDES)

include_directories(Include)

ADD_CUSTOM_TARGET(debug_x64
  COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug -DTARGET_ARCH=x64 ${CMAKE_SOURCE_DIR}
  COMMAND make all
  COMMENT "Switch CMAKE_BUILD_TYPE to Debug"
  )

ADD_CUSTOM_TARGET(release_x64
  COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH=x64 ${CMAKE_SOURCE_DIR}
  COMMAND make all 
  COMMENT "Switch CMAKE_BUILD_TYPE to Release"
)

ADD_CUSTOM_TARGET(debug_x86
  COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug -DTARGET_ARCH=x86 ${CMAKE_SOURCE_DIR}
  COMMAND make all
  COMMENT "Switch CMAKE_BUILD_TYPE to Debug"
  )

ADD_CUSTOM_TARGET(release_x86
  COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH=x86 ${CMAKE_SOURCE_DIR}
  COMMAND make all
  COMMENT "Switch CMAKE_BUILD_TYPE to Release"
  )

if (TARGET_ARCH MATCHES x64)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")

if(UNIX)
if(APPLE)
link_directories(/opt/local/lib)
else()
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m64")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m64")
link_directories(/usr/lib/x86_64-linux-gnu)
endif()
endif()

endif()

if (TARGET_ARCH MATCHES x86)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")

if(UNIX)
if(APPLE)
message(FATAL_ERROR "This option doesn't support for target system")
else()
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
link_directories(/usr/lib/i386-linux-gnu)
endif()
endif()

endif()

add_subdirectory(bin2txt)
add_subdirectory(xml2bin)
