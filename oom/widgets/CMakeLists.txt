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
QT4_WRAP_CPP (widget_mocs
	  toolbars/transporttools.h
	  toolbars/edittools.h
	  toolbars/epictools.h
	  toolbars/looptools.h
	  toolbars/misctools.h
	  toolbars/feedbacktools.h
      toolbars/tools.h  
      aboutbox_impl.h 
      action.h
      apconfig.h
      bigtime.h  
      canvas.h  
      checkbox.h  
      comboQuant.h  
      combobox.h  
      confmport.h 
      comment.h  
      commentdock.h  
      ctrlcombo.h  
      dentry.h  
      didyouknow.h
      doublelabel.h  
      filedialog.h  
      gatetime.h  
      genset.h  
      hitscale.h  
      intlabel.h  
      knob.h  
      lcombo.h  
      gcombo.h  
      metronome.h  
	  #midisyncimpl.h  
      mixdowndialog.h  
      mlabel.h  
      mtscale.h  
      pcscale.h  
	  tvieweditor.h
	  tviewdock.h
	  tracklistview.h
	  trackheader.h
	  EffectRack.h
	  rmap.h
	  rmapdialog.h
	  conductor/pctablemodel.h
	  conductor/pctable.h
	  conductor/Conductor.h
      nentry.h  
      noteinfo.h  
      pitchedit.h  
      pitchlabel.h  
      popupmenu.h  
      # posedit.h  
      poslabel.h  
      projectcreateimpl.h
      scrollscale.h  
      shortcutcapturedialog.h  
      shortcutconfig.h  
      # sigedit.h  
      siglabel.h  
      sigscale.h  
      slider.h  
      sliderbase.h  
      songinfo.h
      spinbox.h  
      spinboxFP.h  
      splitter.h  
      swidget.h  
      tb1.h  
      tempolabel.h  
      # ttoolbar.h  
      ttoolbutton.h  
      velocity.h  
      view.h  
      vscale.h  
	  menulist.h
	  tviewmenu.h
	  tablespinner.h
	  instrumenttree.h
	  instrumentdelegate.h
	  instrumentcombo.h
	  instrumentmenu.h
	  keymapmenu.h
	  midiassign.h
	  midiportdelegate.h
	  midipresetdelegate.h
	  mixerview.h
	  mixerdock.h
	  ccdelegate.h
	  ccedit.h
	  plugingui.h
	  plugindialog.h
	  CreateTrackDialog.h
	  ClipList/BookmarkList.h
	  ClipList/AudioClipList.h
	  ClipList/AudioPlayer.h
	  AutomationMenu.h
	  StretchDialog.h
	  TimeHeader.h
	  TrackInstrumentMenu.h
	  TrackEffects.h
	  TempoCanvas.h
	  TempoHeader.h
	  DoubleSlider.h
      )

##
## UI files
##
file (GLOB widgets_ui_files
      aboutbox.ui  
	  #      appearancebase.ui 
      apconfigbase.ui
      cliplisteditorbase.ui  
      commentbase.ui  
      commentdockbase.ui  
      configmidifilebase.ui
      didyouknow.ui  
      editnotedialogbase.ui  
      editsysexdialogbase.ui  
      fdialogbuttons.ui  
      gatetimebase.ui  
      gensetbase.ui  
      itransformbase.ui  
      metronomebase.ui  
	  #midisync.ui  
      mittransposebase.ui  
      mixdowndialogbase.ui  
	  conductor/ConductorBase.ui
      projectcreate.ui
      shortcutcapturedialogbase.ui  
      shortcutconfigbase.ui  
      songinfo.ui  
      synthconfigbase.ui  
      transformbase.ui  
      transposebase.ui  
      velocitybase.ui
	  trackvieweditorbase.ui
	  tviewdockbase.ui
	  trackheaderbase.ui
	  rmapdialogbase.ui
	  routemapbase.ui
	  midiassignbase.ui
	  cceditbase.ui
	  createtrackbase.ui
	  ClipList/AudioClipListBase.ui
	  stretchdialog.ui
      )
QT4_WRAP_UI (widget_ui_headers ${widgets_ui_files})

##
## List of source files to compile
##
file (GLOB widgets_source_files
      toolbars/tools.cpp 
	  toolbars/transporttools.cpp
	  toolbars/edittools.cpp
	  toolbars/epictools.cpp
	  toolbars/looptools.cpp
	  toolbars/misctools.cpp
	  toolbars/feedbacktools.cpp
      aboutbox_impl.cpp 
      apconfig.cpp 
      bigtime.cpp 
      canvas.cpp 
      checkbox.cpp 
      citem.cpp
      comboQuant.cpp 
      combobox.cpp  
      confmport.cpp
      comment.cpp 
      commentdock.cpp 
      ctrlcombo.cpp 
      dentry.cpp 
      dimap.cpp
      doublelabel.cpp 
      drange.cpp
      filedialog.cpp 
      gatetime.cpp 
      genset.cpp 
      hitscale.cpp 
      intlabel.cpp 
      knob.cpp 
      lcombo.cpp 
      gcombo.cpp 
      metronome.cpp 
	  #midisyncimpl.cpp 
      mixdowndialog.cpp 
      mlabel.cpp 
      mmath.cpp
	  conductor/Conductor.cpp
	  tvieweditor.cpp
	  tviewdock.cpp
	  tracklistview.cpp
	  trackheader.cpp
	  EffectRack.cpp
	  rmap.cpp
	  rmapdialog.cpp
      mtscale.cpp 
      pcscale.cpp 
	  conductor/pctablemodel.cpp
	  conductor/pctable.cpp
      nentry.cpp 
      noteinfo.cpp 
      pitchedit.cpp 
      pitchlabel.cpp 
      popupmenu.cpp 
      # posedit.cpp 
      poslabel.cpp
      projectcreateimpl.cpp 
      scldiv.cpp
      scldraw.cpp
      sclif.cpp
      scrollscale.cpp  
      shortcutcapturedialog.cpp 
      shortcutconfig.cpp 
      # sigedit.cpp 
      siglabel.cpp 
      sigscale.cpp 
      slider.cpp 
      sliderbase.cpp 
      spinbox.cpp 
      spinboxFP.cpp 
      splitter.cpp  
      swidget.cpp 
      tb1.cpp   
      tempolabel.cpp 
      ttoolbar.cpp 
      ttoolbutton.cpp 
      velocity.cpp 
      view.cpp    
      vscale.cpp
	  menulist.cpp
	  tviewmenu.cpp
	  tablespinner.cpp
	  instrumenttree.cpp
	  instrumentdelegate.cpp
	  instrumentcombo.cpp
	  instrumentmenu.cpp
	  keymapmenu.cpp
	  midiassign.cpp
	  midiportdelegate.cpp
	  midipresetdelegate.cpp
	  mixerview.cpp
	  mixerdock.cpp
	  ccdelegate.cpp
	  ccedit.cpp
	  plugingui.cpp
	  plugindialog.cpp
	  CreateTrackDialog.cpp
	  ClipList/BookmarkList.cpp
	  ClipList/AudioClipList.cpp
	  ClipList/AudioPlayer.cpp
	  AutomationMenu.cpp
	  StretchDialog.cpp
	  TimeHeader.cpp
	  TrackInstrumentMenu.cpp
	  TrackEffects.cpp
	  TempoCanvas.cpp
	  TempoHeader.cpp
	  DoubleSlider.cpp
      )

##
## Define target
##
add_library ( widgets STATIC 
      ${widget_ui_headers}
      ${widget_mocs}
      ${widgets_source_files}
      )

##
## Append to the list of translations
##
set (FILES_TO_TRANSLATE
      ${FILES_TO_TRANSLATE}
      ${widgets_source_files}
      ${widgets_ui_files}
      CACHE INTERNAL ""
      )

##
## Compilation flags and target name
##
set_target_properties( widgets
      # PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h ${MUSECXXFLAGS} -I../ -I${PROJECT_SOURCE_DIR}/synti "
      PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h ${MUSECXXFLAGS} -I../ -I${PROJECT_SOURCE_DIR}/synti -fPIC"
	  #PROPERTIES COMPILE_FLAGS "-include ${PROJECT_BINARY_DIR}/all.h ${MUSECXXFLAGS} -I../ -I${PROJECT_SOURCE_DIR}/synti -fPIC -fopenmp"
	  #LINK_FLAGS "-fopenmp"
      OUTPUT_NAME oom_widgets
      )

##
## Linkage
##
target_link_libraries ( widgets
      ${QT_LIBRARIES}
      icons
	  core
      )

##
## Install location
##
#install(TARGETS widgets
#     DESTINATION
#     ${OOMidi_MODULES_DIR}
#     )





