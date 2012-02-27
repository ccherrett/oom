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

#
# Expand Qt macros
#
QT4_WRAP_CPP (composer_mocs
      Composer.h 
      ComposerCanvas.h 
	  HeaderList.h
	  CanvasNavigator.h
      )

#
# List of source files to compile
#
file (GLOB composer_source_files
      Composer.cpp
      ComposerCanvas.cpp
	  HeaderList.cpp
	  CanvasNavigator.cpp
      )

#
# Define target
#
add_library ( Composer STATIC
      ${composer_source_files}
      ${composer_mocs}
      )

#
# Append to the list of translations
#
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${composer_source_files}
      CACHE INTERNAL ""
      )

#
# Compilation flags and target name
#
set_target_properties( Composer
      #PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h -fPIC"
	  PROPERTIES COMPILE_FLAGS "${PYREM_CPP_FLAGS} -include ${PROJECT_BINARY_DIR}/all.h -fPIC -fopenmp"
	  LINK_FLAGS "-fopenmp"
      OUTPUT_NAME oom_composer
      )

#
# Linkage
#
target_link_libraries ( Composer
      ${QT_LIBRARIES}
      awl
      mixer
      widgets
	  core
      )

#
# Install location
#
#install(TARGETS Composer
#     DESTINATION ${OOMidi_MODULES_DIR}
#     )

