# If the Jansson library is not found, download it and build it from source.

set(JANSSON_REPO "https://github.com/akheron/jansson.git" CACHE STRING "Jansson Git repository")

# Release 2.9 of Jansson
set(JANSSON_TAG "v2.13" CACHE STRING "Jansson Git tag")

ExternalProject_Add(jansson
  GIT_REPOSITORY ${JANSSON_REPO}
  GIT_TAG ${JANSSON_TAG}
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/jansson/install -DCMAKE_C_FLAGS=-fPIC -DJANSSON_BUILD_DOCS=OFF
  BINARY_DIR ${CMAKE_BINARY_DIR}/jansson
  INSTALL_DIR ${CMAKE_BINARY_DIR}/jansson/install
  UPDATE_COMMAND ""
  LOG_DOWNLOAD 1
  LOG_UPDATE 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
  LOG_INSTALL 1)

set(JANSSON_FOUND TRUE CACHE INTERNAL "")
set(JANSSON_STATIC_FOUND TRUE CACHE INTERNAL "")
set(JANSSON_INCLUDE_DIR ${CMAKE_BINARY_DIR}/jansson/install/include CACHE INTERNAL "")
set(JANSSON_STATIC_LIBRARIES ${CMAKE_BINARY_DIR}/jansson/install/lib/libjansson.a CACHE INTERNAL "")
set(JANSSON_LIBRARIES ${JANSSON_STATIC_LIBRARIES} CACHE INTERNAL "")
