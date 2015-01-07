################################################################################
#    HPCC SYSTEMS software Copyright (C) 2014 HPCC Systems.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
################################################################################

# - Try to find the libhiredislibrary
# Once done this will define
#
#  LIBHEDIS_FOUND - system has the libhiredis library
#  LIBHIREDIS_INCLUDE_DIR - the libhiredis include directory(s)
#  LIBHIREDIS_LIBRARY - The library needed to use hiredis

IF (NOT LIBREDIS_FOUND)
  IF (WIN32)
    SET (libhiredis "libhiredis")
    SET (libev "libev")
    SET (libevent "libevent")
  ELSE()
    SET (libhiredis "hiredis")
    SET (libev "ev")
    SET (libevent "event")
  ENDIF()

  FIND_PATH(LIBHIREDIS_INCLUDE_DIR hiredis/hiredis.h PATHS /usr/include /usr/share/include /usr/local/include PATH_SUFFIXES hiredis)
  FIND_PATH(LIBHIREDIS_ADAPTERS_INCLUDE_DIR hiredis/adapters/libev.h PATHS /usr/include /usr/share/include /usr/local/include PATH_SUFFIXES hiredis/adapters)
  FIND_LIBRARY(LIBHIREDIS_LIBRARY NAMES ${libhiredis} PATHS /usr/lib /usr/share /usr/lib64 /usr/local/lib /usr/local/lib64)
  FIND_LIBRARY(LIBEV_LIBRARY NAMES ${libev} PATHS /usr/lib /usr/share /usr/lib64 /usr/local/lib /usr/local/lib64)
  FIND_LIBRARY(LIBEVENT_LIBRARY NAMES ${libevent} PATHS /usr/lib /usr/share /usr/lib64 /usr/local/lib /usr/local/lib64)

  SET (LIBHIREDIS_INCLUDE_DIRS  ${LIBHIREDIS_INCLUDE_DIR} ${LIBHIREDIS_ADAPTERS_INCLUDE_DIR} ${libev} ${libevent})
  SET (LIBHIREDIS_LIBRARIES  ${LIBHIREDIS_LIBRARY} ${LIBEV_LIBRARY} ${LIBEVENT_LIBRARY})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(redis DEFAULT_MSG
    LIBHIREDIS_LIBRARIES
    LIBHIREDIS_INCLUDE_DIRS
  )

  MARK_AS_ADVANCED(LIBHIREDIS_INCLUDE_DIRS LIBHIREDIS_LIBRARIES)
ENDIF()

