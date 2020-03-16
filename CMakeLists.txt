cmake_minimum_required(VERSION 3.0)
project(edgex_device_v4l2camera LANGUAGES C)

# Package support
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(Libcsdk REQUIRED)

if (NOT LIBCSDK_FOUND)
  message (WARNING "csdk library or header not found")
endif ()

find_library(EDGEX_CSDK_RELEASE_LIB NAMES csdk)

# Set additional C compiler flags
if (DEFINED CMAKE_C_COMPILER_ID)
  if (CMAKE_C_COMPILER_ID MATCHES GNU)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wstrict-prototypes -Wmissing-prototypes")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -O0")
    set (CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -ggdb -O0")
    set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
    set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O3")
   endif ()
endif ()

set(EDGEX_DEVICE_V4L2CAMERA_C_FILES
    ./device-service.c
	./v4l2camera.c
)

set(EDGEX_DEVICE_V4L2CAMERA_H_FILES
	./v4l2camera.h
)

add_executable(edgex_device_v4l2camera ${EDGEX_DEVICE_V4L2CAMERA_C_FILES} ${EDGEX_DEVICE_V4L2CAMERA_H_FILES})

target_link_libraries (edgex_device_v4l2camera PUBLIC m PRIVATE ${EDGEX_CSDK_RELEASE_LIB} v4l2 jpeg)

install (TARGETS edgex_device_v4l2camera LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
install (DIRECTORY "${CMAKE_SOURCE_DIR}/res/" DESTINATION res )
install (FILES "${CMAKE_SOURCE_DIR}/res/configuration.toml" DESTINATION res )
install (FILES "${CMAKE_SOURCE_DIR}/res/USBCameraDevice.yaml" DESTINATION res )