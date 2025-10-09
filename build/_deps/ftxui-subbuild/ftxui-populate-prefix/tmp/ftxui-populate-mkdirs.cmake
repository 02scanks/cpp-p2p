# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/scanks/Documents/Projects/cpp-p2p/build/_deps/ftxui-src")
  file(MAKE_DIRECTORY "/home/scanks/Documents/Projects/cpp-p2p/build/_deps/ftxui-src")
endif()
file(MAKE_DIRECTORY
  "/home/scanks/Documents/Projects/cpp-p2p/build/_deps/ftxui-build"
  "/home/scanks/Documents/Projects/cpp-p2p/build/_deps/ftxui-subbuild/ftxui-populate-prefix"
  "/home/scanks/Documents/Projects/cpp-p2p/build/_deps/ftxui-subbuild/ftxui-populate-prefix/tmp"
  "/home/scanks/Documents/Projects/cpp-p2p/build/_deps/ftxui-subbuild/ftxui-populate-prefix/src/ftxui-populate-stamp"
  "/home/scanks/Documents/Projects/cpp-p2p/build/_deps/ftxui-subbuild/ftxui-populate-prefix/src"
  "/home/scanks/Documents/Projects/cpp-p2p/build/_deps/ftxui-subbuild/ftxui-populate-prefix/src/ftxui-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/scanks/Documents/Projects/cpp-p2p/build/_deps/ftxui-subbuild/ftxui-populate-prefix/src/ftxui-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/scanks/Documents/Projects/cpp-p2p/build/_deps/ftxui-subbuild/ftxui-populate-prefix/src/ftxui-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
