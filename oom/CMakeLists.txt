#=============================================================================
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

include(${PROJECT_SOURCE_DIR}/pch.txt)

include_directories(
      ${CMAKE_CURRENT_BINARY_DIR}
      )

set (SubDirs 
      arranger 
      cliplist 
      ctrl
      driver
      instruments
      liste
      marker
      master
      midiedit
      mixer
      mplugins      
      waveedit
      widgets
      )

if (ENABLE_PYTHON)
      set ( PYREM_CPP_FLAGS "-DENABLE_PYTHON" )
      set ( PYLIBS ${PYTHON_LIB} remote) 
      set ( SubDirs ${SubDirs} remote )
endif (ENABLE_PYTHON)

subdirs (${SubDirs})

##
## Expand Qt macros in source files
##
QT4_WRAP_CPP ( oom_moc_headers
      app.h 
      appearance.h
      cobject.h 
      conf.h 
      confmport.h 
      midieditor.h 
      miditransform.h 
      plugin.h 
      song.h 
      transport.h 
      transpose.h 
      value.h 
      traverso_shared/TConfig.h
	  cthread.h
	  cserver.h
      )

if(LSCP_SUPPORT)
QT4_WRAP_CPP ( oom_moc_headers
	lsclient.h
	${oom_moc_headers}
      )
endif(LSCP_SUPPORT)

##
## Resource files
##
QT4_ADD_RESOURCES (oom_qrc_files 
		  oom.qrc
		  )

file (GLOB core_source_files
      app.cpp
      appearance.cpp
      audio.cpp
      audioconvert.cpp
      audioprefetch.cpp
      audiotrack.cpp
      cobject.cpp
      conf.cpp
      confmport.cpp
      ctrl.cpp
      dssihost.cpp 
      event.cpp
      eventlist.cpp 
      exportmidi.cpp 
      gconfig.cpp 
      globals.cpp
      help.cpp 
      helper.cpp
      importmidi.cpp 
      key.cpp
      memory.cpp 
      midi.cpp
      midictrl.cpp
      mididev.cpp 
      midieditor.cpp
      midievent.cpp 
      midifile.cpp 
      midiport.cpp
      midiseq.cpp
      miditransform.cpp
      mpevent.cpp
      mtc.cpp
      node.cpp
      osc.cpp
      part.cpp
      plugin.cpp
      pos.cpp
      route.cpp
      seqmsg.cpp 
      shortcuts.cpp 
      sig.cpp
      song.cpp
      songfile.cpp 
      stringparam.cpp
      sync.cpp
      synth.cpp  
      tempo.cpp
      thread.cpp
      ticksynth.cpp 
      track.cpp
	  trackview.cpp
      transport.cpp
      transpose.cpp
      undo.cpp
      value.cpp
      vst.cpp
      wave.cpp
      waveevent.cpp 
      wavetrack.cpp 
      xml.cpp
      traverso_shared/TConfig.cpp
	  cthread.cpp
	  cserver.cpp
      )
file (GLOB main_source_files
      main.cpp
      )
file (GLOB icons_source_files
      icons.cpp
      )
if(LSCP_SUPPORT)
file (GLOB core_source_files
	lsclient.cpp
	${core_source_files}
)
endif(LSCP_SUPPORT)

##
## Define target
##
add_library ( core SHARED
      ${oom_qrc_files}
      ${oom_moc_headers}
      ${core_source_files}
      )
add_executable ( oom
      ${main_source_files}
      )
add_library ( icons SHARED
      ${icons_source_files}
      )

set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${main_source_files}
      ${core_source_files}
      CACHE INTERNAL ""
      )

##
## Directories that will be passed to the compiler with -I flag
##
include_directories (
      arranger
      driver
      instruments
      liste
      midiedit
      mixer
      mplugins
      remote
      waveedit
      widgets
      )

# Make sure to include the uic generated headers paths.
include_directories(
      ${CMAKE_CURRENT_BINARY_DIR}/arranger
      ${CMAKE_CURRENT_BINARY_DIR}/driver
      ${CMAKE_CURRENT_BINARY_DIR}/instruments
      ${CMAKE_CURRENT_BINARY_DIR}/liste
      ${CMAKE_CURRENT_BINARY_DIR}/midiedit
      ${CMAKE_CURRENT_BINARY_DIR}/mixer
      ${CMAKE_CURRENT_BINARY_DIR}/mplugins 
      ${CMAKE_CURRENT_BINARY_DIR}/remote
      ${CMAKE_CURRENT_BINARY_DIR}/waveedit
      ${CMAKE_CURRENT_BINARY_DIR}/widgets 
      )

##
## Compilation flags and target name
##
set_target_properties( core
      # PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h"
      # PROPERTIES COMPILE_FLAGS "-Imidiedit -Iarranger -Iliste -Iwidgets -Imixer -Idriver -Iwaveedit -Implugins -Iinstruments -Iremote ${PYREM_CPP_FLAGS} -include ${PROJECT_BINARY_DIR}/all.h "
      # PROPERTIES COMPILE_FLAGS "${PYREM_CPP_FLAGS} -DINSTPREFIX='\"${CMAKE_INSTALL_PREFIX}\"' -DINSTLIBDIR='\"${LIBRARY_OUTPUT_DIRECTORY}\"' -include ${PROJECT_BINARY_DIR}/all.h "
      PROPERTIES COMPILE_FLAGS "${PYREM_CPP_FLAGS} -include ${PROJECT_BINARY_DIR}/all.h -fPIC"
      OUTPUT_NAME oom_core
      )
set_target_properties( oom
      PROPERTIES OUTPUT_NAME ${OOMidi_EXEC_NAME}
      )
set_target_properties( icons
      PROPERTIES OUTPUT_NAME oom_icons
      )

##
## Linkage
##
target_link_libraries(core
      al      
      arranger 
      awl
      cliplist
      ctrl
      driver
      icons
      instruments
      liste
      marker
      master
      midiedit
      mixer
      mplugins
      synti       
      waveedit
      widgets 
      
      ${QT_LIBRARIES}
	  ${QT_QTNETWORK_LIBRARY}
      ${SNDFILE_LIBRARIES}
      ${SAMPLERATE_LIBRARIES}
      ${UUID_LIBRARIES}
      ${PYLIBS}
      ${FST_LIB}
      dl
      )

if(HAVE_LASH)
      target_link_libraries(core ${LASH_LIBRARIES})
endif(HAVE_LASH)

if(OSC_SUPPORT)
      target_link_libraries(core ${LIBLO_LIBRARIES})
endif(OSC_SUPPORT)

if(LSCP_SUPPORT)
      target_link_libraries(core ${LSCP_LIBRARIES})
endif(LSCP_SUPPORT)

target_link_libraries(oom
      midiedit
      core
      )	

target_link_libraries(icons
      ${QT_LIBRARIES}
	  ${QT_QTNETWORK_LIBRARY}
      )	

##
## Install location
##
install( TARGETS oom 
      RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/
      )
install(TARGETS core icons
      DESTINATION
      ${OOMidi_MODULES_DIR}
      )

