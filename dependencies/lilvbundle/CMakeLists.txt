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

SET(LILV_VERSION "0.5.0")
##
## Expand Qt macros in source files
##
QT4_WRAP_CPP (lilv_bundle_mocs
      )

##
## List of source files to compile
##
file (GLOB lilv_bundle_source_files
    lilv-${LILV_VERSION}/src/collections.c
    lilv-${LILV_VERSION}/src/instance.c
    lilv-${LILV_VERSION}/src/node.c
    lilv-${LILV_VERSION}/src/plugin.c
    lilv-${LILV_VERSION}/src/pluginclass.c
    lilv-${LILV_VERSION}/src/port.c
    lilv-${LILV_VERSION}/src/query.c
    lilv-${LILV_VERSION}/src/scalepoint.c
    lilv-${LILV_VERSION}/src/ui.c
    lilv-${LILV_VERSION}/src/util.c
    lilv-${LILV_VERSION}/src/world.c
	lilv-${LILV_VERSION}/src/zix/tree.c
	)

include_directories(
	lilv-${LILV_VERSION}
	lilv-${LILV_VERSION}/src
	)

#
# produce lilv-config.h file
#
configure_file (
      ${PROJECT_SOURCE_DIR}/dependencies/lilv-config.h.in
      ${PROJECT_BINARY_DIR}/lilv-config.h
      )

##
## Define target
##
add_library ( lilv_bundle STATIC
      ${lilv_bundle_source_files}
	  #      ${lilv_bundle_mocs}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${lilv_bundle_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties( lilv_bundle
      PROPERTIES COMPILE_FLAGS "-fPIC -include ${PROJECT_BINARY_DIR}/all.h"
      OUTPUT_NAME oom_lilv_bundle
      )

##
## Linkage
##
target_link_libraries ( lilv_bundle
        sord_bundle
    dl
    )

##
## Install location
##
#install(TARGETS lilv_bundle
#     DESTINATION ${OOMidi_MODULES_DIR}
#     )

