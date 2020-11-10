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
PROJECT(DI_RTApp C)

azsphere_configure_tools(TOOLS_REVISION "20.10")

# App Version
add_compile_definitions(RTAPP_VERSION="20.10-v1.0.0")

# Create executable
ADD_EXECUTABLE(${PROJECT_NAME} main.c TimerUtil.c InterCoreComm.c PulseCounter.c
mt3620-intercore.c mt3620-gpio.c mt3620-timer.c)
TARGET_LINK_LIBRARIES(${PROJECT_NAME})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/linker.ld)

# Add MakeImage post-build command
azsphere_target_add_image_package(${PROJECT_NAME})
