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
      Composer
      cliplist
      ctrl
      driver
      instruments
      liste
      network
      marker
      master
      midiedit
      mixer
      mplugins
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
      cobject.h
      conf.h
      AbstractMidiEditor.h
      miditransform.h
      song.h
      thread.h
      transport.h
      transpose.h
      value.h
      traverso_shared/TConfig.h
      traverso_shared/AddRemoveCtrlValues.h
      traverso_shared/CommandGroup.h
      traverso_shared/OOMCommand.h
	  AddRemoveTrackCommand.h
      cthread.h
      cserver.h
      midimonitor.h
      ccinfo.h
      FadeCurve.h
	  TrackManager.h
	  NameValidator.h
      )

##
## Resource files
##
QT4_ADD_RESOURCES (oom_qrc_files 
		  oom.qrc
		  )

file (GLOB core_source_files
      app.cpp
      audio.cpp
      audioconvert.cpp
      audioprefetch.cpp
      audiotrack.cpp
      cobject.cpp
      conf.cpp
      ctrl.cpp
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
      AbstractMidiEditor.cpp
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
      plugin_ladspa.cpp
      plugin_lv2.cpp
      plugin_vst.cpp
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
      utils.cpp
      undo.cpp
      value.cpp
      wave.cpp
      waveevent.cpp
      wavetrack.cpp
      xml.cpp
      traverso_shared/TConfig.cpp
      traverso_shared/AddRemoveCtrlValues.cpp
      traverso_shared/CommandGroup.cpp
      traverso_shared/OOMCommand.cpp
	  AddRemoveTrackCommand.cpp
      cthread.cpp
      cserver.cpp
      midimonitor.cpp
      ccinfo.cpp
      FadeCurve.cpp
	  TrackManager.cpp
	  NameValidator.cpp
      lib_functions.cpp
      )
file (GLOB main_source_files
      main.cpp
      )
file (GLOB icons_source_files
      icons.cpp
      )

##
## Define target
##
add_library ( core STATIC
      ${oom_qrc_files}
      ${oom_moc_headers}
      ${core_source_files}
      )
add_executable ( oom
      ${main_source_files}
      )
add_library ( icons STATIC
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
      Composer
      driver
      instruments
      liste
      midiedit
      mixer
      mplugins
      remote
      widgets
      ${OOMIDI_LV2_INCLUDES}
      )

# Make sure to include the uic generated headers paths.
include_directories(
      ${CMAKE_CURRENT_BINARY_DIR}/Composer
      ${CMAKE_CURRENT_BINARY_DIR}/driver
      ${CMAKE_CURRENT_BINARY_DIR}/instruments
      ${CMAKE_CURRENT_BINARY_DIR}/liste
      ${CMAKE_CURRENT_BINARY_DIR}/midiedit
      ${CMAKE_CURRENT_BINARY_DIR}/mixer
      ${CMAKE_CURRENT_BINARY_DIR}/mplugins 
      ${CMAKE_CURRENT_BINARY_DIR}/remote
      ${CMAKE_CURRENT_BINARY_DIR}/widgets 
      )

##
## Compilation flags and target name
##
set_target_properties( core
      #PROPERTIES COMPILE_FLAGS "${PYREM_CPP_FLAGS} -include ${PROJECT_BINARY_DIR}/all.h -fPIC"
      PROPERTIES COMPILE_FLAGS "${PYREM_CPP_FLAGS} -include ${PROJECT_BINARY_DIR}/all.h -fPIC -fopenmp"
      LINK_FLAGS "-fopenmp"
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
      Composer
      awl
      cliplist
      ctrl
      driver
      icons
      instruments
      liste
      network
      marker
      master
      midiedit
      mixer
      mplugins
      synti
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

if (ENABLE_LILV_STATIC)
        if(LILV_SUPPORT)
              target_link_libraries(core lilv_bundle)
        endif(LILV_SUPPORT)
else (ENABLE_LILV_STATIC)
        if(LILV_SUPPORT)
              target_link_libraries(core ${LIBLILV_LIBRARIES})
        endif(LILV_SUPPORT)
endif (ENABLE_LILV_STATIC)

if(GTK2UI_SUPPORT)
        target_link_libraries(core ${GTK2_LIBRARIES})
endif(GTK2UI_SUPPORT)

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
  #install(TARGETS core icons
  #   DESTINATION
  #   ${OOMidi_MODULES_DIR}
  #   )

