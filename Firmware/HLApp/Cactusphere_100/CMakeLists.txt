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
PROJECT(HLApp_Cactusphere_100 C)

azsphere_configure_tools(TOOLS_REVISION "21.04")
azsphere_configure_api(TARGET_API_SET "9")

file(GLOB DRIVERS_SRC drivers/*.c)
file(GLOB COMMON_SRC common/*.c)

add_custom_command(OUTPUT "${PROJECT_SOURCE_DIR}/app_manifest.json"
        COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/${PRODUCT}/app_manifest.json" "${PROJECT_SOURCE_DIR}/app_manifest.json"
        DEPENDS "${PROJECT_SOURCE_DIR}/${PRODUCT}/app_manifest.json")

# Product
if(PRODUCT STREQUAL "atmarktechno_DI_model")
    add_compile_definitions(APP_PRODUCT_ID=0x01)
    set(INCLUDE_DIR ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/drivers
                    ${PROJECT_SOURCE_DIR}/common ${PROJECT_SOURCE_DIR}/DI
    )
    file(GLOB DI_SRC DI/*.c)
    set(SRC_LIST main.c ${DRIVERS_SRC} ${COMMON_SRC} ${DI_SRC})
    set(APP_VERSION "21.04-v1.0.0")
endif()

if(PRODUCT STREQUAL "atmarktechno_RS485_model")
    add_compile_definitions(APP_PRODUCT_ID=0x05)
    set(INCLUDE_DIR ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/drivers
                    ${PROJECT_SOURCE_DIR}/common ${PROJECT_SOURCE_DIR}/RS485
    )
    file(GLOB RS485_SRC RS485/*.c)
    set(SRC_LIST main.c ${DRIVERS_SRC} ${COMMON_SRC} ${RS485_SRC})
    set(APP_VERSION "21.04-v1.0.0")
endif()

# App Version
add_compile_definitions(HLAPP_VERSION="${APP_VERSION}")

CONFIGURE_FILE(
"${PROJECT_SOURCE_DIR}/config.h.in"
"${PROJECT_BINARY_DIR}/config.h"
)

# Create executable
INCLUDE_DIRECTORIES("${INCLUDE_DIR}")
ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_LIST})
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC ${AZURE_SPHERE_API_SET_DIR}/usr/include/azureiot ${AZURE_SPHERE_API_SET_DIR}/usr/include/azure_prov_client)
TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} m azureiot applibs pthread gcc_s c)

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "Hardware/mt3620" TARGET_DEFINITION "mt3620.json")

find_program(POWERSHELL powershell.exe)

if (POWERSHELL)
    # Run validate_manifest script during build
    ADD_CUSTOM_TARGET(ValidateManifest ALL
        COMMAND ${POWERSHELL} -ExecutionPolicy Bypass -NoProfile -NonInteractive -File ${CMAKE_SOURCE_DIR}/script/validate_manifest.ps1
    DEPENDS ${CMAKE_SOURCE_DIR}/app_manifest.json)
else()
    # Warn users without PowerShell to update their manifest
    ADD_CUSTOM_TARGET(ValidateManifest ALL
        COMMAND echo "Please ensure that you have updated app_manifest.json as described in IoTCentral.md or IoTHub.md, as appropriate."
    DEPENDS ${CMAKE_SOURCE_DIR}/app_manifest.json)
endif()

ADD_DEPENDENCIES(ValidateManifest ${PROJECT_NAME})

# Add MakeImage post-build command
azsphere_target_add_image_package(${PROJECT_NAME})