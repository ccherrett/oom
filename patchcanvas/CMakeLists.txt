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
QT4_WRAP_CPP ( patchcanvas_mocs
	canvastestapp.h
    patchscene.h
    theme.h
    canvasbox.h
    patchcanvas.h
	canvasboxshadow.h
	canvasicon.h
	canvasportglow.h
    )

##
## Resource files
##
#QT4_ADD_RESOURCES (patchcanvas_qrc_files 
#		  patchcanvas.qrc
#		  )

##
## List of source files to compile
##
file (GLOB patchcanvas_source_files
	main.cpp
    canvastestapp.cpp
    patchscene.cpp
    theme.cpp
    canvasbox.cpp
    patchcanvas.cpp
	canvasboxshadow.cpp
	canvasicon.cpp
	canvasportglow.cpp
    )

file (GLOB patchcanvas_source_files
	${patchcanvas_source_files}
)

##
## UI files
##
file (GLOB patchcanvas_ui_files
	canvastestapp.ui
	)

QT4_WRAP_UI (patchcanvas_ui_headers ${patchcanvas_ui_files})


##
## Define target
##
add_executable ( patchcanvas
      ${patchcanvas_ui_headers}
      ${patchcanvas_source_files}
      ${patchcanvas_mocs}
	  #     ${patchcanvas_qrc_files}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${patchcanvas_source_files}
      ${patchcanvas_ui_files}
      CACHE INTERNAL ""
      )

##
## Linkage
##
target_link_libraries ( patchcanvas
      ${QT_LIBRARIES}
	  ${QT_QTNETWORK_LIBRARY}
	  ${LSCP_LIBRARIES}
      ${JACK_LIBRARIES}
      )


##
## Install location
##
#install(TARGETS patchcanvas
#     DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
#     )
