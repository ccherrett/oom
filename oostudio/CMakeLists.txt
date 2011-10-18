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
	OOProcess.h
	OOStudio.h
	OOClient.h
	OODownload.h
	OOExtract.h
	OOThread.h
      )

##
## Resource files
##
QT4_ADD_RESOURCES (studio_qrc_files 
		  studio.qrc
		  )

##
## List of source files to compile
##
file (GLOB studio_source_files
	OOProcess.cpp
    OOStudio.cpp
	OOClient.cpp
	OODownload.cpp
	OOExtract.cpp
	OOThread.cpp
	main.cpp
    )

file (GLOB studio_source_files
	${studio_source_files}
)

##
## UI files
##
file (GLOB studio_ui_files
	OOStudioBase.ui
	)

QT4_WRAP_UI (studio_ui_headers ${studio_ui_files})


##
## Define target
##
add_executable ( oostudio
      ${studio_ui_headers}
      ${studio_source_files}
      ${studio_mocs}
      ${studio_qrc_files}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${studio_source_files}
      ${studio_ui_files}
      CACHE INTERNAL ""
      )

##
## Linkage
##
target_link_libraries ( oostudio
      ${QT_LIBRARIES}
	  ${QT_QTNETWORK_LIBRARY}
	  ${LSCP_LIBRARIES}
      ${JACK_LIBRARIES}
      )


##
## Install location
##
install(TARGETS oostudio
      DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
      )
