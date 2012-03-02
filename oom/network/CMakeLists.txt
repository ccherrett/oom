#=============================================================================
#  OOMidi
#  OpenOctave Midi and Audio Editor
#  $Id:$
#
#  Copyright (C) 2002-2006 by Werner Schweer and others
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

##
## Expand Qt macros in source files
##
QT4_WRAP_CPP ( network_mocs
      )

if(LSCP_SUPPORT)
QT4_WRAP_CPP ( network_mocs
	lsclient.h
	LSThread.h
	${network_mocs}
      )
endif(LSCP_SUPPORT)

##
## List of source files to compile
##
file (GLOB network_source_files
      )

if(LSCP_SUPPORT)
file (GLOB network_source_files
	lsclient.cpp
	LSThread.cpp
	${network_source_files}
)
endif(LSCP_SUPPORT)

##
## Define target
##
add_library ( network STATIC
      ${network_source_files}
      ${network_mocs}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${network_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties( network
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h -fPIC"
      OUTPUT_NAME oom_network
      )

##
## Linkage
##
target_link_libraries ( network
      ${QT_LIBRARIES}
	  ${QT_QTNETWORK_LIBRARY}
      awl
      icons
      )

if(LSCP_SUPPORT)
      target_link_libraries(network ${LSCP_LIBRARIES})
endif(LSCP_SUPPORT)

##
## Install location
##
#install(TARGETS network
#     DESTINATION ${OOMidi_MODULES_DIR}
#     )
