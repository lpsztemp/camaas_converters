cmake_minimum_required(VERSION 2.8.3)

project(xml2bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-as-needed -Wall -Wno-unused-function -Wno-unused-local-typedefs")

SET(SOURCES ../src/binary_streams.cpp ../src/face.cpp ../src/text_streams.cpp ../src/xml_exceptions.cpp ../src/xml_parser.cpp arch_ac_domain_xml2bin.cpp domain_converter.cpp entrypoint.cpp hgt_optimizer.cpp radio_hf_domain_xml2bin.cpp xml2bin.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

#target_link_libraries(${PROJECT_NAME} chsvlib read_ini lb_dll domain_shared)

install(TARGETS ${PROJECT_NAME} DESTINATION ${BIN})

message(${BIN})
