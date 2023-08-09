# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ioxp_emu)

target_sources(app PRIVATE
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/erpc/port/erpc_port_zephyr.cpp
    ${PROJECT_SOURCE_DIR}/src/erpc/port/erpc_threading_zephyr.cpp
    ${PROJECT_SOURCE_DIR}/src/erpc/transport/erpc_zephyr_usb_cdc_transport.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/erpcgen/ioxp_pc2emu_server.cpp
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/infra/erpc_server.cpp
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/infra/erpc_framed_transport.cpp
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/infra/erpc_crc16.cpp
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/infra/erpc_message_buffer.cpp
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/infra/erpc_simple_server.cpp
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/infra/erpc_basic_codec.cpp
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/infra/erpc_arbitrated_client_manager.cpp
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/infra/erpc_transport_arbitrator.cpp
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/infra/erpc_client_manager.cpp
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/setup/erpc_setup_mbf_dynamic.cpp
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/setup/erpc_arbitrated_client_setup.cpp
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/setup/erpc_server_setup.cpp
)

target_include_directories(app PRIVATE
    ${PROJECT_SOURCE_DIR}/src/config
    ${PROJECT_SOURCE_DIR}/src/erpc/transport
    ${PROJECT_SOURCE_DIR}/src/erpc/port
    ${CMAKE_CURRENT_BINARY_DIR}/erpcgen
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/infra
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/transports
    ${PROJECT_SOURCE_DIR}/../erpc/erpc_c/setup

)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/erpcgen/ioxp_pc2emu_server.cpp
    COMMAND erpcgen -o ${CMAKE_CURRENT_BINARY_DIR}/erpcgen ${PROJECT_SOURCE_DIR}/../../shared/interface.erpc
)
