#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Copyright (c) 2020 Atmark Techno, Inc.
#  MIT License
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#  THE SOFTWARE.

CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(RS485InitialFirmware_HLApp C)

azsphere_configure_tools(TOOLS_REVISION "22.11")
azsphere_configure_api(TARGET_API_SET "15")

file(GLOB DRIVERS_SRC drivers/*.c)
file(GLOB COMMON_SRC common/*.c)

set(INCLUDE_DIR ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/drivers
                ${PROJECT_SOURCE_DIR}/common ${PROJECT_SOURCE_DIR}/RS485
)
file(GLOB RS485_SRC RS485/*.c)
set(SRC_LIST main.c ${DRIVERS_SRC} ${COMMON_SRC} ${RS485_SRC})

# App Version
add_compile_definitions(APP_VERSION="22.11-v1.0.0")

CONFIGURE_FILE(
"${PROJECT_SOURCE_DIR}/config.h.in"
"${PROJECT_BINARY_DIR}/config.h"
)

# Create executable
INCLUDE_DIRECTORIES("${INCLUDE_DIR}")
ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_LIST})
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC)
TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PUBLIC)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} m applibs pthread gcc_s c)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "Hardware/mt3620" TARGET_DEFINITION "mt3620.json")

# Add MakeImage post-build command
azsphere_target_add_image_package(${PROJECT_NAME})
