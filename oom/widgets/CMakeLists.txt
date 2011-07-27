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
	  transporttools.h
	  edittools.h
	  epictools.h
	  looptools.h
	  misctools.h
	  feedbacktools.h
	  rmap.h
	  rmapdialog.h
	  pctablemodel.h
	  pctable.h
      mtrackinfo.h
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
      tools.h  
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
      mtrackinfobase.ui
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
      )
QT4_WRAP_UI (widget_ui_headers ${widgets_ui_files})

##
## List of source files to compile
##
file (GLOB widgets_source_files
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
      mtrackinfo.cpp
	  tvieweditor.cpp
	  tviewdock.cpp
	  tracklistview.cpp
	  trackheader.cpp
	  transporttools.cpp
	  edittools.cpp
	  epictools.cpp
	  looptools.cpp
	  misctools.cpp
	  feedbacktools.cpp
	  rmap.cpp
	  rmapdialog.cpp
      mtscale.cpp 
      pcscale.cpp 
	  pctablemodel.cpp
	  pctable.cpp
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
      tools.cpp 
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
      )

##
## Define target
##
add_library ( widgets SHARED 
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
      )

##
## Install location
##
install(TARGETS widgets
      DESTINATION
      ${OOMidi_MODULES_DIR}
      )





