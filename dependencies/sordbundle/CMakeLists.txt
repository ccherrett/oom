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

SET(SERD_VERSION "0.5.0")
SET(SORD_VERSION "0.5.0")

##
## List of source files to compile
##
file (GLOB sord_bundle_source_files

    serd-${SERD_VERSION}/src/env.c
    serd-${SERD_VERSION}/src/error.c
    serd-${SERD_VERSION}/src/node.c
    serd-${SERD_VERSION}/src/reader.c
    serd-${SERD_VERSION}/src/uri.c
    serd-${SERD_VERSION}/src/writer.c
	sord-${SORD_VERSION}/src/sord.c
	sord-${SORD_VERSION}/src/syntax.c
	sord-${SORD_VERSION}/src/zix/hash.c
	sord-${SORD_VERSION}/src/zix/tree.c
	)

include_directories(
	serd-${SERD_VERSION}
	serd-${SERD_VERSION}/src
	sord-${SORD_VERSION}
	sord-${SORD_VERSION}/src
	)

#
# produce sord-config.h file
#
configure_file (
      ${PROJECT_SOURCE_DIR}/dependencies/sord-config.h.in
      ${PROJECT_BINARY_DIR}/sord-config.h
      )


##
## Define target
##
add_library ( sord_bundle STATIC
      ${sord_bundle_source_files}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${sord_bundle_source_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties( sord_bundle
      PROPERTIES COMPILE_FLAGS "-fPIC -include ${PROJECT_BINARY_DIR}/all.h"
      OUTPUT_NAME oom_sord_bundle
      )

##
## Linkage
##
target_link_libraries ( sord_bundle
     dl
     )

##
## Install location
##
#install(TARGETS sord_bundle
#     DESTINATION ${OOMidi_MODULES_DIR}
#     )

