# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart_echo_bot)

target_sources(app PRIVATE
    src/main.c
    src/erpc/port/erpc_port_zephyr.cpp
    src/erpc/port/erpc_threading_zephyr.cpp
    generated/ioxp_pc2emu_server.cpp
    ../erpc/erpc_c/infra/erpc_server.cpp
    ../erpc/erpc_c/infra/erpc_framed_transport.cpp
    ../erpc/erpc_c/infra/erpc_crc16.cpp
    ../erpc/erpc_c/infra/erpc_message_buffer.cpp
    ../erpc/erpc_c/infra/erpc_simple_server.cpp
    ../erpc/erpc_c/infra/erpc_basic_codec.cpp
)

target_include_directories(app PRIVATE
    src/config
    generated
    ../erpc/erpc_c/port
    ../erpc/erpc_c/infra
    ../erpc/erpc_c/transports
    ../erpc/erpc_c/setup
)
