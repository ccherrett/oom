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
QT4_WRAP_CPP ( studio_mocs
      )

if(LSCP_SUPPORT)
QT4_WRAP_CPP ( studio_mocs
	OOStudio.h
	${studio_mocs}
      )
endif(LSCP_SUPPORT)

##
## List of source files to compile
##
file (GLOB studio_source_files
      OOStudio.cpp
	  main.cpp
      )

if(LSCP_SUPPORT)
file (GLOB studio_source_files
	${studio_source_files}
)
endif(LSCP_SUPPORT)

##
## Define target
##
add_executable ( oostudio
      ${studio_source_files}
      ${studio_mocs}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${studio_source_files}
      CACHE INTERNAL ""
      )

##
## Linkage
##
target_link_libraries ( oostudio
      ${QT_LIBRARIES}
	  ${QT_QTNETWORK_LIBRARY}
      icons
      )

if(LSCP_SUPPORT)
      target_link_libraries(oostudio ${LSCP_LIBRARIES})
endif(LSCP_SUPPORT)

##
## Install location
##
install(TARGETS oostudio
      DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
      )
