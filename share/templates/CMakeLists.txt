#=============================================================================
#  OOMidi
#  OpenOctave Midi and Audio Editor
#  $Id:$
#
#  Copyright (C) 2002-2006 by Werner Schweer and others
#  Copyright (C) 2011 by Andrew Williams and Christopher Cherrett
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

file (GLOB templates_files 
	default.oom
	default.lscp
	oomidi-template.oom
	oomidi-template.lscp
	oomidi-extended-template.oom
	oomidi-extended-template.lscp
	Classical_Guitar.sfz
	AcousticGuitar.sfz
	)

configure_file (
      oomidi-template.oos.in
      ${PROJECT_BINARY_DIR}/share/templates/oomidi-template.oos
      )

configure_file (
      oomidi-extended-template.oos.in
      ${PROJECT_BINARY_DIR}/share/templates/oomidi-extended-template.oos
      )


install( FILES ${templates_files} 
	${PROJECT_BINARY_DIR}/share/templates/oomidi-template.oos
    ${PROJECT_BINARY_DIR}/share/templates/oomidi-extended-template.oos
    DESTINATION ${OOMidi_SHARE_DIR}/templates
    )

