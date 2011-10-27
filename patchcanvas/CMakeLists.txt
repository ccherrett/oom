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
    patchcanvas.h
    patchscene.h
    theme.h
    canvasbezierline.h
    canvasbezierlinemov.h
    canvasbox.h
    canvasboxshadow.h
    canvasfadeanimation.h
    canvasicon.h
    canvasline.h
    canvaslinemov.h
    canvasport.h
    canvasportglow.h
    )

##
## Resource files
##
#QT4_ADD_RESOURCES ( patchcanvas_qrc_files
    #patchcanvas.qrc
    #)

##
## List of source files to compile
##
file (GLOB patchcanvas_source_files
    patchcanvas.cpp
    patchscene.cpp
    theme.cpp
    canvasbezierline.cpp
    canvasbezierlinemov.cpp
    canvasbox.cpp
    canvasboxshadow.cpp
    canvasfadeanimation.cpp
    canvasicon.cpp
    canvasline.cpp
    canvaslinemov.cpp
    canvasport.cpp
    canvasportglow.cpp
    )

file (GLOB patchcanvas_source_files
    ${patchcanvas_source_files}
)

##
## Define target
##
add_library ( patchcanvas STATIC
      ${patchcanvas_ui_headers}
      ${patchcanvas_source_files}
      ${patchcanvas_mocs}
      #${patchcanvas_qrc_files}
      )

##
## Linkage
##
target_link_libraries ( patchcanvas
      ${QT_LIBRARIES}
      )
