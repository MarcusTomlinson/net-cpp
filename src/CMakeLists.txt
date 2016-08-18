# Copyright © 2013 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Thomas Voss <thomas.voss@canonical.com>

find_package(Boost COMPONENTS system serialization REQUIRED)
find_package(CURL REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})

add_library(
  net-cpp SHARED

  ${NET_CPP_INTERFACE_HEADERS}

  core/location.cpp

  core/net/error.cpp
  core/net/uri.cpp

  core/net/http/client.cpp
  core/net/http/error.cpp
  core/net/http/header.cpp
  core/net/http/request.cpp
  core/net/http/status.cpp

  core/net/http/impl/curl/client.cpp
  core/net/http/impl/curl/easy.cpp
  core/net/http/impl/curl/multi.cpp
  core/net/http/impl/curl/shared.cpp
)

target_link_libraries(
  net-cpp

  ${CMAKE_THREAD_LIBS_INIT}

  ${Boost_LIBRARIES}
  ${CURL_LIBRARIES}
)

set(symbol_map "${CMAKE_SOURCE_DIR}/symbols.map")
set_target_properties(
  net-cpp

  PROPERTIES
  LINK_FLAGS "${ldflags} -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
  VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  SOVERSION ${NET_CPP_SOVERSION}
)

install(
  TARGETS net-cpp
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
