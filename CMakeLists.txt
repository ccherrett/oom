#  OOMidi
#  OpenOctave Midi and Audio Editor
#  $Id:$
#
#  Copyright (C) 2002-2008 by Werner Schweer and others
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#=============================================================================

include(CheckIncludeFiles)
include(FindPkgConfig)
include(cmake/Summary.cmake)
include(cmake/TargetDoc.cmake)

project(oom)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.1)
if (COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# set libdir if not use -DCMAKE_INSTALL_LIBDIR
if (NOT CMAKE_INSTALL_LIBDIR)
	SET(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
endif (NOT CMAKE_INSTALL_LIBDIR)

IF(NOT DEFINED SHARE_INSTALL_PREFIX)
        SET(SHARE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/share")
ENDIF(NOT DEFINED SHARE_INSTALL_PREFIX)

#set(CMAKE_BUILD_TYPE debug)
#set(CMAKE_BUILD_TYPE release)

# If no CMAKE_BUILD_TYPE is given on the command line,
#  cmake either uses the cached value, or 'empty' (plain un-opt build). 
# And yet the only way a user can reset a cached CMAKE_BUILD_TYPE
#  is with "-DCMAKE_BUILD_TYPE=". So we cannot interfere with this.
# We should probably not attempt to do this at all.
# Installation instructions now updated to be more specific, give options.
#endif (NOT DEFINED CMAKE_BUILD_TYPE)
#if (NOT CMAKE_BUILD_TYPE)
#    message("No CMAKE_BUILD_TYPE specified. Setting to Release.")
#     set(CMAKE_BUILD_TYPE release)
#endif (NOT CMAKE_BUILD_TYPE)


# for debugging the make system uncomment next line:
# set(CMAKE_VERBOSE_MAKEFILE ON)

##
## set internal midi buffer sizes
##
if(NOT DEFINED OOM_MIDI_FIFO_SIZE)
	set(OOM_MIDI_FIFO_SIZE 2048)
endif(NOT DEFINED OOM_MIDI_FIFO_SIZE)

if(NOT DEFINED OOM_MIDI_REC_FIFO_SIZE)
	set(OOM_MIDI_REC_FIFO_SIZE 256)
endif(NOT DEFINED OOM_MIDI_REC_FIFO_SIZE)


set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
#set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
set(CMAKE_SKIP_RULE_DEPENDENCY TRUE)

# The OOMidi version number.
SET(OOMidi_VERSION_MAJOR 2012)
SET(OOMidi_VERSION_MINOR 1)
SET(OOMidi_VERSION_PATCH beta1)
SET(OOMidi_VERSION "${OOMidi_VERSION_MAJOR}.${OOMidi_VERSION_MINOR}.${OOMidi_VERSION_PATCH}")
SET(OOMidi_VERSION_FULL  "${OOMidi_VERSION}")
SET(OOMidi_INSTALL_NAME  "oom-${OOMidi_VERSION}")
SET(OOMidi_EXEC_NAME     "oomidi")
SET(OOMidi_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/bin/${OOMidi_EXEC_NAME}")
SET(OOMIDI_DEPS_DIR "${PROJECT_SOURCE_DIR}/dependencies")
SET(OOMIDI_PLUGIN_INCLUDES "${PROJECT_SOURCE_DIR}/oom/plugin_includes")

## The library where we will install the shared components:
SET(OOMidi_MODULES_DIR ${CMAKE_INSTALL_LIBDIR}/${OOMidi_INSTALL_NAME}/modules)
#SET(CMAKE_INSTALL_RPATH ${OOMidi_MODULES_DIR})

## Synthesizers directory
SET(OOMidi_SYNTHI_DIR ${CMAKE_INSTALL_LIBDIR}/${OOMidi_INSTALL_NAME}/synthi)

## Plugins directory
#SET(OOMidi_PLUGINS_DIR ${CMAKE_INSTALL_LIBDIR}/${OOMidi_INSTALL_NAME}/plugins)

## Top documentation dir
IF(NOT DEFINED OOMidi_DOC_DIR)
      SET(OOMidi_DOC_DIR ${SHARE_INSTALL_PREFIX}/doc/${OOMidi_INSTALL_NAME}/)
ENDIF(NOT DEFINED OOMidi_DOC_DIR)

## Data directory
SET(OOMidi_SHARE_DIR ${SHARE_INSTALL_PREFIX}/${OOMidi_INSTALL_NAME})

## Lib directory
SET(OOMidi_LIB_DIR ${CMAKE_INSTALL_LIBDIR}/${OOMidi_INSTALL_NAME})

#option ( ENABLE_LASH        "enable LASH Audio Session Handler"                              OFF)
#option ( ENABLE_OSC         "enable Open Sound Control (DSSI also recommended)"              OFF)   
#option ( ENABLE_DSSI        "enable Disposable Soft Synth Interface (OSC also recommended)"  OFF)
#option ( ENABLE_VST         "enable VST/win support"                                         OFF)
#option ( ENABLE_FLUID       "enable fluidsynth softsynth plugins"							 OFF)
#option ( ENABLE_EXPERIMENTAL "enable building experimental features"                         OFF)
option ( UPDATE_TRANSLATIONS "Update source translation share/locale/*.ts files 
         (WARNING: This will modify the .ts files in the source tree!!)"                     OFF)
option ( ENABLE_LSCP         "enable lscp client"                                            ON)
option ( ENABLE_LV2          "enable LV2 plugin support"                                      ON)
option ( ENABLE_LILV_STATIC  "enable static LILV linking"   								 ON)
option ( ENABLE_LV2UI        "enable LV2 plugin UI support"                                   ON)
#option ( ENABLE_JACK_SESSION "enable Jack Session support"                                   ON)

##
## Just print a notice if this is OS X
##
if (APPLE)
	message("OS X found.")
	set(OOMidi_PATH_SEPARATOR ":")
	set(OOMidi_DIR_SEPARATOR "/")
	set(OOMidi_DEFAULT_LV2_PATH
		"~/Library/Audio/Plug-Ins/LV2:~/.lv2:/usr/local/lib/lv2:/usr/lib/lv2:/Library/Audio/Plug-Ins/LV2")
else (APPLE)
	if (UNIX)
		message("Unix (probably linux) found")
		set(OOMidi_PATH_SEPARATOR ":")
		set(OOMidi_DIR_SEPARATOR "/")
		set(OOMidi_DEFAULT_LV2_PATH "~/.lv2:/usr/local/lib/lv2:/usr/lib/lv2")
	endif(UNIX)
	if(WIN32)
		set(OOMidi_PATH_SEPARATOR ";")
		set(OOMidi_DIR_SEPARATOR "/")
		set(OOMidi_DEFAULT_LV2_PATH "%APPDATA%\\\\LV2;%PROGRAMFILES%\\\\LV2")
	endif(WIN32)

endif (APPLE)

##
##  look for Qt4
##

set(QT_MIN_VERSION "4.3.0")
set(QT_USE_QTXML TRUE)
set(QT_USE_QTDESIGNER TRUE)
#set(QT_USE_QTNSPLUGIN TRUE)
SET (QT_USE_QTWEBKIT TRUE)
set(QT_USE_QTSVG TRUE)

find_package(Qt4)

if (NOT QT4_FOUND)
      message(FATAL_ERROR "Fatal error: QT (version >= 4.3.0) required.\n"
      "Cmake tries to detect QT4 by searching for 'qmake' in your PATH\n"
      "If you have QT4 installed, make sure qmake is found in your PATH."
      )
endif (NOT QT4_FOUND)

# Needed for plugins factory:
SET(QT_USE_QTUITOOLS TRUE)

# Needed for the server to work
SET (QT_USE_QTNETWORK TRUE)

include(${QT_USE_FILE})


##
## find doxygen
##    TODO

PKG_CHECK_MODULES(LINUXSAMPLER REQUIRED linuxsampler>=1.0.0.svn)
##
## alsa >= 0.9.0
##

if (APPLE)
	message("Disabling ALSA support due to OS X build.")
else (APPLE)
PKG_CHECK_MODULES(ALSA REQUIRED alsa>=0.9.0)
include_directories(${ALSA_INCLUDE_DIRS})
endif (APPLE)

##
## find sndfile >= 1.0.0
##

PKG_CHECK_MODULES(SNDFILE REQUIRED sndfile>=1.0.0)
include_directories(${SNDFILE_INCLUDE_DIRS})

##
## find libsamplerate >= 0.1.0
##

PKG_CHECK_MODULES(SAMPLERATE REQUIRED samplerate>=0.1.0)
include_directories(${SAMPLERATE_INCLUDE_DIRS})

##
## find libuuid 
##

PKG_CHECK_MODULES(UUID REQUIRED uuid>=0.0.1)
include_directories(${UUID_INCLUDE_DIRS})

##
## find jack >= 0.103.0
##

PKG_CHECK_MODULES(JACK REQUIRED jack>=0.103)
include_directories(${JACK_INCLUDE_DIRS})

##
## find if jack is jack2
##

PKG_CHECK_MODULES(JACK2 jack>=1.9.5)
if (JACK2_FOUND)
	set(JACK2_SUPPORT ON)
else (JACK2_FOUND)
	set(JACK2_SUPPORT OFF)
endif (JACK2_FOUND)

##
## jack_session
##

#if(ENABLE_JACK_SESSION)
#	set(JACK_SESSION_SUPPORT ON)
#else(ENABLE_JACK_SESSION)
#	set(JACK_SESSION_SUPPORT OFF)
#endif(ENABLE_JACK_SESSION)

##
## Set the plugin headers path
##
include_directories(${OOMIDI_PLUGIN_INCLUDES})

CHECK_INCLUDE_FILES (wordexp.h HAVE_WORDEXP)

##
## find LASH 
##

#if (ENABLE_LASH)
#      PKG_CHECK_MODULES(LASH lash-1.0>=0.2)
#      if (LASH_FOUND)
#            include_directories(${LASH_INCLUDE_DIRS})
#            set(HAVE_LASH ON)
#      endif (LASH_FOUND)           
#	  #else (ENABLE_LASH)
#	  #      message("LASH disabled")
#endif (ENABLE_LASH)
#
###
### check for liblo >= 0.23
###
#
#if (ENABLE_OSC)
#      PKG_CHECK_MODULES(LIBLO liblo>=0.23)
#      if (LIBLO_FOUND)
#            include_directories(${LIBLO_INCLUDE_DIRS})
#            set(OSC_SUPPORT ON)
#      endif (LIBLO_FOUND)
#	  #else (ENABLE_OSC)
#	  #      message("OSC disabled")
#endif (ENABLE_OSC)
#
###
### check for dssi >= 0.9.0
###
#
#if (ENABLE_DSSI)
#      PKG_CHECK_MODULES(DSSI dssi>=0.9.0)
#      if (DSSI_FOUND)
#            include_directories(${DSSI_INCLUDE_DIRS})
#            set(DSSI_SUPPORT ON)
#      endif (DSSI_FOUND)
#	  #else (ENABLE_DSSI)
#	  #      message("DSSI disabled")
#endif (ENABLE_DSSI)
#
#if (ENABLE_VST)
#      message("VST support enabled")
#      set (VST_SUPPORT TRUE)
#else (ENABLE_VST)
#	#      message("VST support disabled")
#      set (VST_SUPPORT FALSE)
#endif (ENABLE_VST)

PKG_CHECK_MODULES(LSCP REQUIRED lscp)
if (LSCP_FOUND)
    message("LSCP support enabled")
    include_directories(${LSCP_INCLUDE_DIRS})
    set (LSCP_SUPPORT TRUE)
endif (LSCP_FOUND)

if (ENABLE_LILV_STATIC)
	if (ENABLE_LV2)
	    message("LILV internal static build enabled")
		#include_directories(${OOMIDI_DEPS_DIR})
	    set (LILV_SUPPORT TRUE)
	    set (LILV_STATIC TRUE)
	    set (LILV_DEPS "dependencies")
	    set (LV2_SUPPORT TRUE)
	endif (ENABLE_LV2)
else (ENABLE_LILV_STATIC)
	if (ENABLE_LV2)
	      PKG_CHECK_MODULES(LIBLILV lilv-0>=0.4.4)
	      if (LIBLILV_FOUND)
	          message("LILV support enabled")
	          include_directories(${LIBLILV_INCLUDE_DIRS})
	          set (LILV_SUPPORT TRUE)
	          set (LV2_SUPPORT TRUE)
		  else (LIBLILV_FOUND)
	          message("LILV support disabled")
	          set (LILV_SUPPORT FALSE)
	          set (LV2_SUPPORT FALSE)
	      endif (LIBLILV_FOUND)
	endif (ENABLE_LV2)
endif (ENABLE_LILV_STATIC)

if(LV2_SUPPORT)
	PKG_CHECK_MODULES(GTK2 REQUIRED gtk+-2.0)
	if(GTK2_FOUND)
		message("Enabling lv2 gtk2 support")
		include_directories(${GTK2_INCLUDE_DIRS})
		set(GTK2UI_SUPPORT TRUE)
	endif(GTK2_FOUND)
endif(LV2_SUPPORT)

##
## TODO
##
## Optimizations
##

SET (USE_SSE false)

##
## check for fluidsynth
##

#if ( ENABLE_FLUID )
#      # Be careful with naming here since we have two shared synth libraries
#      #  to be built later, named 'fluidsynth' and 'fluid' !
#      PKG_CHECK_MODULES(FLUIDSYN fluidsynth>=0.9.0)
#      if (FLUIDSYN_FOUND)
#            include_directories(${FLUIDSYN_INCLUDE_DIRS})
#            set(HAVE_FLUIDSYNTH ON)
#            # message("FLUIDSYN_LIBRARIES " ${FLUIDSYN_LIBRARIES} )          # "fluidsynth"
#            # message("FLUIDSYN_LDFLAGS " ${FLUIDSYN_LDFLAGS} )              # "-lfluidsynth"
#            # message("FLUIDSYN_LDFLAGS_OTHER " ${FLUIDSYN_LDFLAGS_OTHER} )  # empty
#            # message("FLUIDSYN_INCLUDE_DIRS " ${FLUIDSYN_INCLUDE_DIRS} )    # empty
#            # message("FLUIDSYN_CFLAGS " ${FLUIDSYN_CFLAGS} )                # empty
#            # message("FLUIDSYN_CFLAGS_OTHER " ${FLUIDSYN_CFLAGS_OTHER} )    # empty
#      endif (FLUIDSYN_FOUND)
#else ( ENABLE_FLUID )
#      message("Fluidsynth disabled")
#endif ( ENABLE_FLUID )

#if ( ENABLE_EXPERIMENTAL )
#      set(CMAKE_CXX_FLAGS -DBUILD_EXPERIMENTAL ${CMAKE_CXX_FLAGS})
#endif ( ENABLE_EXPERIMENTAL )

#
# produce config.h file
#
configure_file (
      ${PROJECT_SOURCE_DIR}/config.h.in
      ${PROJECT_BINARY_DIR}/config.h
      )

add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/all.h
      COMMAND cp ${PROJECT_SOURCE_DIR}/all.h ${PROJECT_BINARY_DIR}/all.h
      DEPENDS ${PROJECT_SOURCE_DIR}/all.h
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      )

add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/all-pic.h
      COMMAND cp ${PROJECT_SOURCE_DIR}/all.h ${PROJECT_BINARY_DIR}/all-pic.h
      DEPENDS ${PROJECT_SOURCE_DIR}/all.h
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      )

add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/all-pic-debug.h
      COMMAND cp ${PROJECT_SOURCE_DIR}/all.h ${PROJECT_BINARY_DIR}/all-pic-debug.h
      DEPENDS ${PROJECT_SOURCE_DIR}/all.h
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      )

include_directories(
   .
   ${PROJECT_SOURCE_DIR}
   ${PROJECT_SOURCE_DIR}/oom
   ${PROJECT_SOURCE_DIR}/oom/widgets
   ${PROJECT_BINARY_DIR}
   ${PROJECT_BINARY_DIR}/oom
   ${PROJECT_BINARY_DIR}/oom/widgets
   ${PROJECT_BINARY_DIR}/oom/instruments
   ${PROJECT_BINARY_DIR}/oom/ctrl
   )

# include (pch.txt)

#
# If the cmake version includes cpack, use it
#
IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

  IF(EXISTS "${CMAKE_ROOT}/Modules/InstallRequiredSystemLibraries.cmake")
      SET(CMAKE_INSTALL_MFC_LIBRARIES 1)
      INCLUDE(InstallRequiredSystemLibraries)
  ENDIF(EXISTS "${CMAKE_ROOT}/Modules/InstallRequiredSystemLibraries.cmake")

  SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OOMidi is a virtual audio studio environment")
  SET(CPACK_PACKAGE_VENDOR "The Open Octave Project and others")
  SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
  SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
  SET(CPACK_PACKAGE_VERSION_MAJOR "${OOMidi_VERSION_MAJOR}")
  SET(CPACK_PACKAGE_VERSION_MINOR "${OOMidi_VERSION_MINOR}")
  SET(CPACK_PACKAGE_VERSION_PATCH "${OOMidi_VERSION_PATCH}")
  SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
  SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${OOMidi_INSTALL_NAME}")

  # Don't pack the binary tree and the subversion directories
  SET(CPACK_SOURCE_IGNORE_FILES "/\\\\.git/;/\\\\.gitignore;${CMAKE_BINARY_DIR}/*")

  SET(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})

  SET(CPACK_PACKAGE_FILE_NAME "${CPACK_SOURCE_PACKAGE_FILE_NAME}-${CPACK_SYSTEM_NAME}")
  #SET(CPACK_STRIP_FILES "bin/oomidi;bin/grepmidi;bin/oostudio")
  SET(CPACK_STRIP_FILES "bin/oomidi;bin/oostudio")
  #SET(CPACK_PACKAGE_EXECUTABLES "oomidi" "OOMidi" "grepmidi" "grepmidi" "oostudio" "oostudio")
  SET(CPACK_PACKAGE_EXECUTABLES "oomidi" "OOMidi" "oostudio" "oostudio")
  INCLUDE(CPack)
ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")

set(CMAKE_CXX_FLAGS         "-Wall -Wextra -Winvalid-pch -fexceptions ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -fomit-frame-pointer -ffast-math -fstrength-reduce ${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -DQT_DEBUG ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS         "${CMAKE_CXX_FLAGS} -std=c99 -fvisibility=hidden")

# NOTE: share/ directory needs to be at the end so that the translations
#       are scanned before coming to share/locale
#subdirs(al awl grepmidi man plugins oom synti packaging utils share oostudio)

subdirs(al awl man ${LILV_DEPS} oom synti packaging utils share oostudio patchcanvas)

## Install doc files
file (GLOB doc_files
      AUTHORS
      COPYING
      README
      )
INSTALL( FILES ${doc_files} DESTINATION ${OOMidi_DOC_DIR})

## Uninstall support
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake")

## Show a summary of what we got
#summary_add("Lash support" HAVE_LASH)
#summary_add("OSC (Liblo) support" OSC_SUPPORT)
#summary_add("DSSI support" DSSI_SUPPORT)
#summary_add("VST support" VST_SUPPORT)
summary_add("LV2 support" LV2_SUPPORT)
summary_add("LILV support" LILV_SUPPORT)
summary_add("LILV static" ENABLE_LILV_STATIC)
summary_add("GTK2 GUI support" GTK2UI_SUPPORT)
summary_add("LSCP support" LSCP_SUPPORT)
#summary_add("JACK_SESSION support" JACK_SESSION_SUPPORT)
#summary_add("Fluidsynth support" HAVE_FLUIDSYNTH)
#summary_add("Experimental features" ENABLE_EXPERIMENTAL)
summary_show()

## Make the user aware of what type they're building.
if (NOT CMAKE_BUILD_TYPE)
    message(" Build type: CMAKE_BUILD_TYPE is empty. Plain un-optimized build.")
else (NOT CMAKE_BUILD_TYPE)
    message(" Build type: " ${CMAKE_BUILD_TYPE} )          
endif (NOT CMAKE_BUILD_TYPE)
message("")

