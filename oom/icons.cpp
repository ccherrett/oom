//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: icons.cpp,v 1.13.2.8 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#include "globals.h"

#include <QIcon>

#include "xpm/track_comment.xpm"
#include "xpm/audio_bounce_to_file.xpm"
#include "xpm/audio_bounce_to_track.xpm"
#include "xpm/audio_restartaudio.xpm"
#include "xpm/automation_clear_data.xpm"
#include "xpm/automation_mixer.xpm"
#include "xpm/automation_take_snapshot.xpm"
#include "xpm/edit_midi.xpm"
#include "xpm/midi_edit_instrument.xpm"
#include "xpm/midi_init_instr.xpm"
#include "xpm/midi_inputplugins.xpm"
#include "xpm/midi_inputplugins_midi_input_filter.xpm"
#include "xpm/midi_inputplugins_midi_input_transform.xpm"
#include "xpm/midi_inputplugins_remote_control.xpm"
#include "xpm/midi_inputplugins_transpose.xpm"
#include "xpm/midi_inputplugins_random_rhythm_generator.xpm"
#include "xpm/midi_local_off.xpm"
#include "xpm/midi_reset_instr.xpm"
#include "xpm/midi_thru_off3.xpm"
#include "xpm/midi_thru_on3.xpm"
#include "xpm/settings_appearance_settings.xpm"
#include "xpm/settings_configureshortcuts.xpm"
#include "xpm/settings_follow_song.xpm"
#include "xpm/settings_globalsettings.xpm"
#include "xpm/settings_metronome.xpm"
#include "xpm/settings_midifileexport.xpm"
#include "xpm/settings_midiport_softsynths.xpm"
#include "xpm/settings_midisync.xpm"
#include "xpm/view_bigtime_window.xpm"
#include "xpm/view_cliplist.xpm"
#include "xpm/view_marker.xpm"
#include "xpm/view_mixer.xpm"
#include "xpm/view_transport_window.xpm"
#include "xpm/note.xpm"
#include "xpm/note1.xpm"
#include "xpm/stick.xpm"
#include "xpm/wave.xpm"
#include "xpm/synth.xpm"
#include "xpm/quant.xpm"
#include "xpm/fileprint.xpm"
#include "xpm/filesave.xpm"
#include "xpm/filesaveas.xpm"
#include "xpm/fileopen.xpm"
#include "xpm/fileprintS.xpm"
#include "xpm/filesaveS.xpm"
#include "xpm/filesaveasS.xpm"
#include "xpm/fileopenS.xpm"
#include "xpm/master.xpm"
#include "xpm/filenew.xpm"
#include "xpm/filenewS.xpm"
#include "xpm/home.xpm"
#include "xpm/back.xpm"
#include "xpm/forward.xpm"

#include "xpm/up.xpm"
#include "xpm/down.xpm"
#include "xpm/bold.xpm"
#include "xpm/italic.xpm"
#include "xpm/underlined.xpm"
#include "xpm/gv.xpm"
#include "xpm/midiin.xpm"
#include "xpm/sysex.xpm"
#include "xpm/ctrl.xpm"
#include "xpm/meta.xpm"
#include "xpm/pitch.xpm"
#include "xpm/cafter.xpm"
#include "xpm/pafter.xpm"
#include "xpm/flag.xpm"
#include "xpm/flagS.xpm"
#include "xpm/lock.xpm"
#include "xpm/toc.xpm"
#include "xpm/exitS.xpm"

#include "xpm/undo.xpm"
#include "xpm/redo.xpm"
#include "xpm/undoS.xpm"
#include "xpm/redoS.xpm"
#include "xpm/editcutS.xpm"
#include "xpm/editcopyS.xpm"
#include "xpm/editpasteS.xpm"
#include "xpm/editmuteS.xpm"
#include "xpm/editpastecloneS.xpm"
#include "xpm/editpaste2trackS.xpm"
#include "xpm/editpasteclone2trackS.xpm"

#include "xpm/buttondown.xpm"
#include "xpm/configure.xpm"


// next two lines will vanish soon
#include "xpm/solobutton.xpm"
#include "xpm/newmutebutton.xpm"

#include "xpm/redled.xpm"
#include "xpm/darkredled.xpm"
#include "xpm/greendot.xpm"
#include "xpm/bluedot.xpm"
#include "xpm/graydot.xpm"
#include "xpm/off.xpm"
#include "xpm/blacksquare.xpm"
#include "xpm/blacksqcheck.xpm"

#include "xpm/mastertrackS.xpm"
#include "xpm/localoffS.xpm"
#include "xpm/miditransformS.xpm"
#include "xpm/midi_plugS.xpm"
#include "xpm/mixerS.xpm"
#include "xpm/mustangS.xpm"
#include "xpm/resetS.xpm"
#include "xpm/track_add.xpm"
#include "xpm/track_delete.xpm"
#include "xpm/listS.xpm"
#include "xpm/inputpluginS.xpm"
#include "xpm/cliplistS.xpm"
#include "xpm/initS.xpm"

#include "xpm/addtrack_addmiditrack.xpm"
#include "xpm/addtrack_audiogroup.xpm"
#include "xpm/addtrack_audioinput.xpm"
#include "xpm/addtrack_audiooutput.xpm"
#include "xpm/addtrack_auxsend.xpm"
#include "xpm/addtrack_drumtrack.xpm"
#include "xpm/addtrack_wavetrack.xpm"
#include "xpm/edit_drumms.xpm"
#include "xpm/edit_list.xpm"
#include "xpm/remove_ctrl.xpm"
#include "xpm/edit_wave.xpm"
#include "xpm/edit_mastertrack.xpm"
#include "xpm/edit_pianoroll.xpm"
#include "xpm/edit_score.xpm"
#include "xpm/edit_track_add.xpm"
#include "xpm/edit_track_del.xpm"
#include "xpm/mastertrack_graphic.xpm"
#include "xpm/mastertrack_list.xpm"
#include "xpm/midi_transform.xpm"
#include "xpm/midi_transpose.xpm"
#include "xpm/select.xpm"
#include "xpm/select_all.xpm"
#include "xpm/select_all_parts_on_track.xpm"
#include "xpm/select_inside_loop.xpm"
#include "xpm/select_outside_loop.xpm"

#include "xpm/oom_icon.xpm"

#include "xpm/global.xpm"
#include "xpm/project.xpm"
#include "xpm/user.xpm"

#include "xpm/sine.xpm"
#include "xpm/saw.xpm"

#if QT_VERSION >= 0x040600
#define MPIXMAP(a,b) QPixmap(QIcon::fromTheme(b, QIcon(QPixmap(a))).pixmap(QPixmap(a).width(),QPixmap(a).height()))
#define MICON(a,b) QIcon(QIcon::fromTheme(b, QIcon(QPixmap(a))))
#else
#define MPIXMAP(a,b) QPixmap(a)
#define MICON(a,b) QIcon(QPixmap(a))
#endif

/* Quick API reference:
   -------------------

   QPixmap MPIXMAP(const char * const[] xpm, const QString & name)
   QIcon MICON(const char * const[] xpm, const QString & name)

      xpm:  a valid XPM image data
      name: filename of a theme icon, without the extension; or NULL
*/

QPixmap* track_commentIcon;
QPixmap* mastertrackSIcon;
QPixmap* localoffSIcon;
QPixmap* miditransformSIcon;
QPixmap* midi_plugSIcon;
QPixmap* miditransposeSIcon;
QPixmap* commentIcon;
QPixmap* midiThruOnIcon;
QPixmap* midiThruOffIcon;
QPixmap* mixerSIcon;
QPixmap* mustangSIcon;
QPixmap* resetSIcon;
QPixmap* track_addIcon;
QPixmap* track_deleteIcon;
QPixmap* listSIcon;
QPixmap* inputpluginSIcon;
QPixmap* cliplistSIcon;
QPixmap* mixerAudioSIcon;
QPixmap* initSIcon;
QPixmap* songInfoIcon;

QPixmap* addMidiIcon;
QPixmap* addAudioIcon;
QPixmap* addBussIcon;
QPixmap* addOutputIcon;
QPixmap* addInputIcon;
QPixmap* addAuxIcon;
QPixmap* addSynthIcon;

QPixmap* exitIcon;
QPixmap* exit1Icon;
QPixmap* newmuteIcon;
QPixmap* soloIcon;

QPixmap* crosshairIcon;
QPixmap* pointerIcon;
QPixmap* pencilIcon;
QPixmap* pencilCursorIcon;
QPixmap* deleteIcon;
QPixmap* punchinIcon;
QPixmap* punchoutIcon;
QPixmap* punchin1Icon;
QPixmap* punchout1Icon;
QPixmap* loopIcon;
QPixmap* loop1Icon;
QPixmap* playIcon;
QPixmap* auditionIcon;

QPixmap* record1_Icon;
QPixmap* record_on_Icon;
QPixmap* record_off_Icon;
QPixmap* record_over_Icon;
QPixmap* recordIcon;
QPixmap* stopIcon;
QPixmap* startIcon;
QPixmap* fforwardIcon;
QPixmap* frewindIcon;
QPixmap* dotIcon;
QPixmap* dothIcon;
QPixmap* dot1Icon;
QPixmap* note1Icon;
QPixmap* noteIcon;
QPixmap* stickIcon;
QPixmap* waveIcon;
QPixmap* synthIcon;
QPixmap* markIcon[4];
QPixmap* cutIcon;
QPixmap* steprecIcon;
QPixmap* glueIcon;
QPixmap* drawIcon;
QPixmap* quantIcon;
QPixmap* printIcon;
QPixmap* printIconS;
QPixmap* openIcon;
QPixmap* saveIcon;
QPixmap* saveasIcon;
QPixmap* openIconS;
QPixmap* saveIconS;
QPixmap* saveasIconS;
QPixmap* masterIcon;
QPixmap* filenewIcon;
QPixmap* filenewIconS;
QPixmap* homeIcon;
QPixmap* backIcon;
QPixmap* forwardIcon;
QPixmap* muteIcon;
QPixmap* upIcon;
QPixmap* downIcon;
QPixmap* boldIcon;
QPixmap* italicIcon;
QPixmap* underlinedIcon;
QPixmap* gvIcon;
QPixmap* midiinIcon;
QPixmap* sysexIcon;
QPixmap* ctrlIcon;
QPixmap* metaIcon;
QPixmap* pitchIcon;
QPixmap* cafterIcon;
QPixmap* pafterIcon;
QPixmap* flagIcon;
QPixmap* flagIconS;
QPixmap* flagIconSP;
QPixmap* flagIconSPSel;
QPixmap* lockIcon;
QPixmap* tocIcon;
QPixmap* exitIconS;

QPixmap* undoIcon;
QPixmap* redoIcon;
QPixmap* undoIconS;
QPixmap* redoIconS;

QPixmap* speakerIcon;
QPixmap* buttondownIcon;
QPixmap* configureIcon;

QPixmap* multiDisplay;
QPixmap* previousPartIcon;
QPixmap* nextPartIcon;
QPixmap* blankRecord;
QPixmap* preIcon;
QPixmap* preIconOn;
QPixmap* mixerIn;
QPixmap* mixerOut;
QPixmap* recEchoIconOn;
QPixmap* recEchoIconOff;

QPixmap* muteIconOn;
QPixmap* muteIconOff;
QPixmap* muteIconOver;
QIcon* muteIconSet4;

QPixmap* soloIconOn;
QPixmap* soloIconOff;
QPixmap* soloIconOver;
QPixmap* transport_soloIconOn;
QPixmap* transport_soloIconOff;
QPixmap* transport_soloIconOver;
QPixmap* soloblksqIconOn;
QPixmap* soloblksqIconOff;
QPixmap* soloblksqIconOver;
QIcon* soloIconSet1;
QIcon* soloIconSet2;
QIcon* soloIconSet3;

QPixmap* transport_muteIconOn;
QPixmap* transport_muteIconOff;
QPixmap* transport_muteIconOver;
QIcon* muteIconSet3;

QPixmap* midiInIconOn;
QPixmap* midiInIconOff;
QPixmap* midiInIconOver;
QIcon* midiInIconSet3;

QPixmap* transport_recordIconOn;
QPixmap* transport_recordIconOff;
QPixmap* transport_recordIconOver;
QIcon* recordIconSet3;

QPixmap* transport_playIconOn;
QPixmap* transport_playIconOff;
QPixmap* transport_playIconOver;
QIcon* playIconSet3;

QPixmap* transport_playIconRightOn;
QPixmap* transport_playIconRightOff;
QPixmap* transport_playIconRightOver;
QIcon* playIconSetRight;

QPixmap* transport_stopIconLeftOn;
QPixmap* transport_stopIconLeftOff;
QPixmap* transport_stopIconLeftOver;
QIcon* stopIconSetLeft;

QPixmap* transport_startIconOn;
QPixmap* transport_startIconOff;
QPixmap* transport_startIconOver;
QIcon* startIconSet3;

QPixmap* transport_rewindIconOn;
QPixmap* transport_rewindIconOff;
QPixmap* transport_rewindIconOver;
QIcon* rewindIconSet3;

QPixmap* transport_forwardIconOn;
QPixmap* transport_forwardIconOff;
QPixmap* transport_forwardIconOver;
QIcon* forwardIconSet3;

QPixmap* transport_stopIconOn;
QPixmap* transport_stopIconOff;
QPixmap* transport_stopIconOver;
QIcon* stopIconSet3;

QPixmap* pointerIconOn;
QPixmap* pointerIconOff;
QPixmap* pointerIconOver;
QIcon* pointerIconSet3;

QPixmap* pencilIconOn;
QPixmap* pencilIconOff;
QPixmap* pencilIconOver;
QIcon* pencilIconSet3;

QPixmap* deleteIconOn;
QPixmap* deleteIconOff;
QPixmap* deleteIconOver;
QIcon* deleteIconSet3;

QPixmap* cutIconOn;
QPixmap* cutIconOff;
QPixmap* cutIconOver;
QIcon* cutIconSet3;

QIcon* note1IconSet3;

QPixmap* glueIconOn;
QPixmap* glueIconOff;
QPixmap* glueIconOver;
QIcon* glueIconSet3;

QIcon* quantIconSet3;

QPixmap* drawIconOn;
QPixmap* drawIconOff;
QPixmap* drawIconOver;
QIcon* drawIconSet3;

QPixmap* stretchIconOn;
QPixmap* stretchIconOff;
QPixmap* stretchIconOver;
QIcon* stretchIconSet3;

QPixmap* multiDisplayIconOn;
QPixmap* multiDisplayIconOff;
QPixmap* multiDisplayIconOver;
QIcon* multiDisplayIconSet3;

QPixmap* selectMultiIconOn;
QPixmap* selectMultiIconOff;
QPixmap* selectMultiIconOver;
QIcon* selectMultiIconSet3;

QPixmap* auditionIconOn;
QPixmap* auditionIconOff;
QPixmap* auditionIconOver;
QIcon* auditionIconSet3;

QPixmap* feedbackIconOn;
QPixmap* feedbackIconOff;
QPixmap* feedbackIconOver;
QIcon* feedbackIconSet3;

QPixmap* globalKeysIconOn;
QPixmap* globalKeysIconOff;
QPixmap* globalKeysIconOver;
QIcon* globalKeysIconSet3;

QPixmap* stepIconOn;
QPixmap* stepIconOff;
QPixmap* stepIconOver;
QIcon* stepIconSet3;

QPixmap* punchinIconOn;
QPixmap* punchinIconOff;
QPixmap* punchinIconOver;
QIcon* punchinIconSet3;

QPixmap* punchoutIconOn;
QPixmap* punchoutIconOff;
QPixmap* punchoutIconOver;
QIcon* punchoutIconSet3;

QPixmap* loopIconOn;
QPixmap* loopIconOff;
QPixmap* loopIconOver;
QIcon* loopIconSet3;

QPixmap* punchinVertIconOn;
QPixmap* punchinVertIconOff;
QPixmap* punchinVertIconOver;
QIcon* punchinVertIconSet3;

QPixmap* punchoutVertIconOn;
QPixmap* punchoutVertIconOff;
QPixmap* punchoutVertIconOver;
QIcon* punchoutVertIconSet3;

QPixmap* loopVertIconOn;
QPixmap* loopVertIconOff;
QPixmap* loopVertIconOver;
QIcon* loopVertIconSet3;

QPixmap* expandIconOn;
QPixmap* expandIconOff;
QPixmap* expandIconOver;
QIcon*   expandIconSet3;

QPixmap* vuIconOn;
QPixmap* vuIconOff;
QPixmap* vuIconOver;
QIcon*   vuIconSet3;

QPixmap* refreshIconOn;
QPixmap* refreshIconOff;
QPixmap* refreshIconOver;
QIcon*   refreshIconSet3;

QIcon* mixer_resizeIconSet3;
QIcon* mixer_inputIconSet3;
QIcon* mixer_outputIconSet3;
QIcon* mixer_powerIconSet3;
QIcon* mixer_recordIconSet3;
QIcon* mixer_muteIconSet3;
QIcon* mixer_soloIconSet3;
QIcon* mixer_stereoIconSet3;
QIcon* mixerIconSet3;
QIcon* pcloaderIconSet3;

QPixmap* enabled_OnIcon;
QPixmap* enabled_OffIcon;
QPixmap* enabled_OverIcon;
QIcon* enabledIconSet3;

QPixmap* up_arrow_OnIcon;
QPixmap* up_arrow_OffIcon;
QPixmap* up_arrow_OverIcon;
QIcon* up_arrowIconSet3;

QPixmap* down_arrow_OnIcon;
QPixmap* down_arrow_OffIcon;
QPixmap* down_arrow_OverIcon;
QIcon* down_arrowIconSet3;

QIcon* collapseIconSet3;

QPixmap* plus_OnIcon;
QPixmap* plus_OffIcon;
QPixmap* plus_OverIcon;
QIcon* plusIconSet3;

QPixmap* browse_OnIcon;
QPixmap* browse_OffIcon;
QPixmap* browse_OverIcon;
QIcon* browseIconSet3;

QPixmap* garbage_OnIcon;
QPixmap* garbage_OffIcon;
QPixmap* garbage_OverIcon;
QIcon* garbageIconSet3;

QPixmap* duplicate_OnIcon;
QPixmap* duplicate_OffIcon;
QPixmap* duplicate_OverIcon;
QIcon* duplicateIconSet3;

QPixmap* comment_OnIcon;
QPixmap* comment_OffIcon;
QPixmap* comment_OverIcon;
QIcon* commentIconSet3;

QPixmap* transpose_OnIcon;
QPixmap* transpose_OffIcon;
QPixmap* transpose_OverIcon;
QIcon* transposeIconSet3;

QPixmap* connect_OnIcon;
QPixmap* connect_OffIcon;
QPixmap* connect_OverIcon;
QIcon*   connectIconSet3;

QPixmap* input_OnIcon;
QPixmap* input_OffIcon;
QPixmap* input_OverIcon;
QIcon*   inputIconSet3;

QPixmap* load_OnIcon;
QPixmap* load_OffIcon;
QPixmap* load_OverIcon;
QIcon*   loadIconSet3;

QPixmap* pluginGUI_OnIcon;
QPixmap* pluginGUI_OffIcon;
QPixmap* pluginGUI_OverIcon;
QIcon*   pluginGUIIconSet3;

QPixmap* route_edit_OnIcon;
QPixmap* route_edit_OffIcon;
QPixmap* route_edit_OverIcon;
QIcon*   route_editIconSet3;

QPixmap* next_OnIcon;
QPixmap* next_OffIcon;
QPixmap* next_OverIcon;
QIcon*   nextIconSet3;

QPixmap* previous_OnIcon;
QPixmap* previous_OffIcon;
QPixmap* previous_OverIcon;
QIcon*   previousIconSet3;

QPixmap* metronome_OnIcon;
QPixmap* metronome_OffIcon;
QPixmap* metronome_OverIcon;
QIcon*   metronomeIconSet3;

QPixmap* dragMidiIcon;
QPixmap* dragAudioIcon;
QPixmap* dragInputIcon;
QPixmap* dragOutputIcon;
QPixmap* dragAuxIcon;
QPixmap* dragBussIcon;

QPixmap* mixer_record_OnIcon;
QPixmap* mixer_record_OffIcon;
QPixmap* mixer_record_OverIcon;
QPixmap* mixer_mute_OnIcon;
QPixmap* mixer_mute_OffIcon;
QPixmap* mixer_mute_OverIcon;
QPixmap* mixer_solo_OnIcon;
QPixmap* mixer_solo_OffIcon;
QPixmap* mixer_solo_OverIcon;
QPixmap* mixer_resize_OffIcon;
QPixmap* mixer_resize_OverIcon;
QPixmap* mixer_input_OffIcon;
QPixmap* mixer_input_OverIcon;
QPixmap* mixer_output_OffIcon;
QPixmap* mixer_output_OverIcon;
QPixmap* mixer_power_OnIcon;
QPixmap* mixer_power_OffIcon;
QPixmap* mixer_power_OverIcon;
QPixmap* mixer_blank_OffIcon;
QPixmap* mixer_stereo_OnIcon;
QPixmap* mixer_mono_OnIcon;

QPixmap* mixer_OnIcon;
QPixmap* mixer_OffIcon;
QPixmap* mixer_OverIcon;
QPixmap* pcloader_OnIcon;
QPixmap* pcloader_OffIcon;
QPixmap* pcloader_OverIcon;

QPixmap* speakerIconOn;
QPixmap* speakerIconOff;
QPixmap* speakerIconOver;
QIcon* speakerIconSet3;

QPixmap* globalArmIconOff;
QPixmap* globalArmIconOver;
QIcon* globalArmIconSet3;

QPixmap* panicIconOff;
QPixmap* panicIconOver;
QIcon* panicIconSet3;

QPixmap* editmuteIcon;
QPixmap* editmuteSIcon;
QPixmap* panicIcon;
QPixmap* feedbackIcon;

QPixmap* arranger_record_on_Icon;
QPixmap* arranger_record_off_Icon;
QPixmap* arranger_record_over_Icon;
QPixmap* arranger_mute_on_Icon;
QPixmap* arranger_mute_off_Icon;
QPixmap* arranger_mute_over_Icon;
QPixmap* arranger_solo_on_Icon;
QPixmap* arranger_solo_off_Icon;
QPixmap* arranger_solo_over_Icon;
QPixmap* duplicatePCIcon;
QPixmap* garbagePCIcon;
QPixmap* upPCIcon;
QPixmap* downPCIcon;

QIcon* pianoIconSet;
QIcon* scoreIconSet;
QIcon* editcutIconSet;
QIcon* editmuteIconSet;
QIcon* editcopyIconSet;
QIcon* editpasteIconSet;
QIcon* editpaste2TrackIconSet;
QIcon* editpasteCloneIconSet;
QIcon* editpasteClone2TrackIconSet;

/* Not used - Orcan
QIcon* pianoIcon;
QIcon* editcutIcon;
QIcon* editcopyIcon;
QIcon* editpasteIcon;
QIcon* editpasteCloneIcon;
QIcon* editpaste2TrackIcon;
QIcon* editpasteClone2TrackIcon;
*/

QPixmap* redLedIcon;
QPixmap* darkRedLedIcon;
QPixmap* greendotIcon;
//QPixmap* darkgreendotIcon;
QPixmap* graydotIcon;
QPixmap* bluedotIcon;
QPixmap* offIcon;
QPixmap* blacksquareIcon;
QPixmap* blacksqcheckIcon;

QPixmap* addtrack_addmiditrackIcon;
QPixmap* addtrack_audiogroupIcon;
QPixmap* addtrack_audioinputIcon;
QPixmap* addtrack_audiooutputIcon;
QPixmap* addtrack_auxsendIcon;
QPixmap* addtrack_drumtrackIcon;
QPixmap* addtrack_wavetrackIcon;
QPixmap* edit_drummsIcon;
QPixmap* edit_listIcon;
QPixmap* remove_ctrlIcon;
QPixmap* edit_waveIcon;
QPixmap* edit_mastertrackIcon;
QPixmap* edit_pianorollIcon;
QPixmap* edit_scoreIcon;
QPixmap* edit_track_addIcon;
QPixmap* edit_track_delIcon;
QPixmap* mastertrack_graphicIcon;
QPixmap* mastertrack_listIcon;
QPixmap* midi_transformIcon;
QPixmap* midi_transposeIcon;
QPixmap* selectIcon;
QPixmap* selectMultiIcon;
QPixmap* select_allIcon;
QPixmap* select_all_parts_on_trackIcon;
QPixmap* select_deselect_allIcon;
QPixmap* select_inside_loopIcon;
QPixmap* select_invert_selectionIcon;
QPixmap* select_outside_loopIcon;

QPixmap* audio_bounce_to_fileIcon;
QPixmap* audio_bounce_to_trackIcon;
QPixmap* audio_restartaudioIcon;
QPixmap* automation_clear_dataIcon;
QPixmap* automation_mixerIcon;
QPixmap* automation_take_snapshotIcon;
QPixmap* edit_midiIcon;
QPixmap* midi_edit_instrumentIcon;
QPixmap* midi_init_instrIcon;
QPixmap* midi_inputpluginsIcon;
QPixmap* midi_inputplugins_midi_input_filterIcon;
QPixmap* midi_inputplugins_midi_input_transformIcon;
QPixmap* midi_inputplugins_random_rhythm_generatorIcon;
QPixmap* midi_inputplugins_remote_controlIcon;
QPixmap* midi_inputplugins_transposeIcon;
QPixmap* midi_local_offIcon;
QPixmap* midi_reset_instrIcon;
QPixmap* settings_appearance_settingsIcon;
QPixmap* settings_configureshortcutsIcon;
QPixmap* settings_follow_songIcon;
QPixmap* settings_globalsettingsIcon;
QPixmap* settings_metronomeIcon;
QPixmap* settings_midifileexportIcon;
QPixmap* settings_midiport_softsynthsIcon;
QPixmap* settings_midisyncIcon;
QPixmap* view_bigtime_windowIcon;
QPixmap* view_cliplistIcon;
QPixmap* view_markerIcon;
QPixmap* view_mixerIcon;
QPixmap* view_transport_windowIcon;

QPixmap* mixerIcon;
QPixmap* expandIcon;
QPixmap* monoIcon;
QPixmap* stereoIcon;
QPixmap* automationIcon;
QPixmap* portIcon;
QPixmap* automationDisabledIcon;
QPixmap* portDisabledIcon;
QPixmap* addTVIcon;
QPixmap* oomIcon;
QPixmap* aboutOOMidiImage;
QPixmap* oomLeftSideLogo;
QPixmap* reminder1_OnIcon;
QPixmap* reminder1_OffIcon;
QPixmap* reminder1_OverIcon;
QPixmap* reminder2_OnIcon;
QPixmap* reminder2_OffIcon;
QPixmap* reminder2_OverIcon;
QPixmap* reminder3_OnIcon;
QPixmap* reminder3_OffIcon;
QPixmap* reminder3_OverIcon;
QPixmap* record_track_OnIcon;
QPixmap* record_track_OffIcon;
QPixmap* record_track_OverIcon;
QPixmap* mute_track_OnIcon;
QPixmap* mute_track_OffIcon;
QPixmap* mute_track_OverIcon;
QPixmap* solo_track_OnIcon;
QPixmap* solo_track_OffIcon;
QPixmap* solo_track_OverIcon;
QPixmap* input_indicator_OnIcon;
QPixmap* input_indicator_OffIcon;
QPixmap* input_indicator_OverIcon;
QPixmap* automation_track_OffIcon;
QPixmap* automation_track_OverIcon;
QPixmap* instrument_track_OffIcon;
QPixmap* instrument_track_OverIcon;
QPixmap* instrument_track_ActiveIcon;

QPixmap* instrument_OnIcon;
QPixmap* instrument_OffIcon;
QPixmap* instrument_OverIcon;

QIcon* reminder1IconSet3;
QIcon* reminder2IconSet3;
QIcon* reminder3IconSet3;

QIcon* record_trackIconSet3;
QIcon* mute_trackIconSet3;
QIcon* solo_trackIconSet3;
QIcon* automation_trackIconSet3;
QIcon* instrument_trackIconSet3;
QIcon* instrumentIconSet3;
QIcon* input_indicatorIconSet3;

QPixmap* armAllIcon;
QPixmap* globalKeysIcon;

QIcon* globalIcon;
QIcon* projectIcon;
QIcon* userIcon;


QPixmap* sineIcon;
QPixmap* sawIcon;

//---------------------------------------------------------
//   initIcons
//---------------------------------------------------------

void initIcons()
      {
      track_commentIcon = new MPIXMAP(track_comment_xpm, NULL);
      crosshairIcon  = new MPIXMAP(":/images/crosshair.png", NULL);
      pointerIcon  = new MPIXMAP(":/images/icons/select.png", NULL);
      pencilIcon   = new MPIXMAP(":/images/icons/pencil.png", NULL);
      pencilCursorIcon   = new MPIXMAP(":/images/icons/pencil_cursor.png", NULL);
      deleteIcon   = new MPIXMAP(":/images/icons/eraser.png", NULL);
      punchinIcon  = new MPIXMAP(":/images/icons/transport-punchin.png", NULL);
      punchoutIcon = new MPIXMAP(":/images/icons/transport-punchout.png", NULL);
      punchin1Icon = new MPIXMAP(":/images/icons/transport-punchin.png", NULL);
      punchout1Icon = new MPIXMAP(":/images/icons/transport-punchout.png", NULL);
      loopIcon     = new MPIXMAP(":/images/icons/transport-loop.png", NULL);
      loop1Icon    = new MPIXMAP(":/images/icons/transport-loop.png", NULL);
      playIcon     = new MPIXMAP(":/images/icons/transport-play.png", NULL);
      auditionIcon     = new MPIXMAP(":/images/icons/audition.png", NULL);

      record1_Icon  = new MPIXMAP(":/images/icons/mixer-record.png", NULL);
      record_on_Icon = new MPIXMAP(":/images/icons/mixer-record_on.png", NULL);
      record_off_Icon = new MPIXMAP(":/images/icons/mixer-record.png", NULL);
      record_over_Icon = new MPIXMAP(":/images/icons/mixer-record_over.png", NULL);
      recordIcon   = new MPIXMAP(":/images/icons/transport-record.png", NULL);
      stopIcon     = new MPIXMAP(":/images/icons/transport-stop.png", NULL);
      startIcon    = new MPIXMAP(":/images/icons/transport-rewind-end.png", NULL);
      fforwardIcon = new MPIXMAP(":/images/icons/transport-ffwd.png", NULL);
      frewindIcon  = new MPIXMAP(":/images/icons/transport-rewind.png", NULL);
      dotIcon      = new MPIXMAP(":/images/icons/arranger_solo_on.png", NULL);
      dothIcon     = new MPIXMAP(":/images/icons/arranger_solo_off.png", NULL);
      dot1Icon     = new MPIXMAP(":/images/icons/arranger_solo_on.png", NULL);
      noteIcon     = new MPIXMAP(note_xpm, NULL);
      note1Icon    = new MPIXMAP(note1_xpm, NULL);
      stickIcon    = new MPIXMAP(stick_xpm, NULL);
      waveIcon     = new MPIXMAP(wave_xpm, NULL);
      synthIcon    = new MPIXMAP(synth_xpm, NULL);
      markIcon[0]  = new MPIXMAP(":/images/icons/cmark.png", NULL);
      markIcon[1]  = new MPIXMAP(":/images/icons/lmark.png", NULL);
      markIcon[2]  = new MPIXMAP(":/images/icons/rmark.png", NULL);
      markIcon[3]  = new MPIXMAP(":/images/icons/emark.png", NULL);
      cutIcon      = new MPIXMAP(":/images/icons/split.png", NULL);
      steprecIcon  = new MPIXMAP(":/images/icons/step_by_step.png", NULL);
      glueIcon     = new MPIXMAP(":/images/icons/join_tracks.png", NULL);
      drawIcon     = new MPIXMAP(":/images/icons/line-tool.png", NULL);
      quantIcon    = new MPIXMAP(quant_xpm, NULL);
      saveIcon     = new MPIXMAP(filesave_xpm, NULL);
      saveasIcon     = new MPIXMAP(filesaveas_xpm, NULL);
      printIcon    = new MPIXMAP(fileprint_xpm, NULL);//"document-print");
      openIcon     = new MPIXMAP(fileopen_xpm, NULL);//"document-open");
      saveIconS     = new MPIXMAP(filesaveS_xpm, NULL);//"document-save");
      saveasIconS     = new MPIXMAP(filesaveasS_xpm, NULL);//"document-save-as");
      printIconS    = new MPIXMAP(fileprintS_xpm, NULL);//"document-print");
      openIconS     = new MPIXMAP(fileopenS_xpm, NULL);//"document-open");
      masterIcon   = new MPIXMAP(master_xpm, NULL);//"mixer-master");
      filenewIcon  = new MPIXMAP(filenew_xpm, NULL);//"document-new");
      filenewIconS  = new MPIXMAP(filenewS_xpm, NULL);//"document-new");
      homeIcon     = new MPIXMAP(home_xpm, NULL);//"user-home");
      backIcon     = new MPIXMAP(back_xpm, NULL);//"go-previous");
      forwardIcon  = new MPIXMAP(forward_xpm, NULL);//"go-next");
      muteIcon     = new MPIXMAP(editmuteS_xpm, NULL);//"audio-volume-muted");
      upIcon       = new MPIXMAP(up_xpm, NULL);//"go-up");
      downIcon     = new MPIXMAP(down_xpm, NULL);//"go-down");
      boldIcon     = new MPIXMAP(bold_xpm, NULL);//"format-text-bold");
      italicIcon     = new MPIXMAP(italic_xpm, NULL);//"format-text-italic");
      underlinedIcon = new MPIXMAP(underlined_xpm, NULL);//"format-text-underline");
      gvIcon     = new MPIXMAP(gv_xpm, NULL);
      midiinIcon = new MPIXMAP(midiin_xpm, NULL);
      sysexIcon   = new MPIXMAP(sysex_xpm, NULL);
      ctrlIcon    = new MPIXMAP(ctrl_xpm, NULL);
      metaIcon    = new MPIXMAP(meta_xpm, NULL);
      pitchIcon   = new MPIXMAP(pitch_xpm, NULL);
      cafterIcon  = new MPIXMAP(cafter_xpm, NULL);
      pafterIcon  = new MPIXMAP(pafter_xpm, NULL);
      flagIcon    = new MPIXMAP(flag_xpm, NULL);
      flagIconS   = new MPIXMAP(flagS_xpm, NULL);
      flagIconSP  = new MPIXMAP(":/images/flagSP.png", NULL);//ProgramChange Flag
      flagIconSPSel  = new MPIXMAP(":/images/flagSP-select.png", NULL);//ProgramChange Flag
      upPCIcon 	  = new MPIXMAP(":/images/icons/up.png", NULL);//ProgramChange Flag
      downPCIcon   = new MPIXMAP(":/images/icons/down.png", NULL);//ProgramChange Flag
      garbagePCIcon = new MPIXMAP(":/images/icons/garbage.png", NULL);//ProgramChange Flag
      duplicatePCIcon = new MPIXMAP(":/images/icons/duplicate.png", NULL);//ProgramChange Flag
      arranger_record_on_Icon = new MPIXMAP(":/images/icons/arranger_record_on.png", NULL);//ProgramChange Flag
      arranger_record_off_Icon = new MPIXMAP(":/images/icons/arranger_record_off.png", NULL);//ProgramChange Flag
      arranger_record_over_Icon = new MPIXMAP(":/images/icons/arranger_record_over.png", NULL);//ProgramChange Flag
      arranger_mute_on_Icon = new MPIXMAP(":/images/icons/arranger_mute_on.png", NULL);//ProgramChange Flag
      arranger_mute_off_Icon = new MPIXMAP(":/images/icons/arranger_mute_off.png", NULL);//ProgramChange Flag
      arranger_mute_over_Icon = new MPIXMAP(":/images/icons/arranger_mute_over.png", NULL);//ProgramChange Flag
      arranger_solo_on_Icon = new MPIXMAP(":/images/icons/arranger_solo_on.png", NULL);//ProgramChange Flag
      arranger_solo_off_Icon = new MPIXMAP(":/images/icons/arranger_solo_off.png", NULL);//ProgramChange Flag
      arranger_solo_over_Icon = new MPIXMAP(":/images/icons/arranger_solo_over.png", NULL);//ProgramChange Flag
      duplicatePCIcon = new MPIXMAP(":/images/icons/duplicate.png", NULL);//ProgramChange Flag
      lockIcon    = new MPIXMAP(lock_xpm, NULL);
      tocIcon     = new MPIXMAP(toc_xpm, NULL);
      exitIconS   = new MPIXMAP(exitS_xpm, NULL);//"application-exit");

      undoIcon     = new MPIXMAP(undo_xpm, NULL);//"edit-undo");
      redoIcon     = new MPIXMAP(redo_xpm, NULL);//"edit-redo");
      undoIconS    = new MPIXMAP(undoS_xpm, NULL);//"edit-undo");
      redoIconS    = new MPIXMAP(redoS_xpm, NULL);//"edit-redo");

      speakerIcon    = new MPIXMAP(":/images/icons/speaker.png", NULL);
      buttondownIcon = new MPIXMAP(buttondown_xpm, NULL);//"arrow-down");
      configureIcon  = new MPIXMAP(configure_xpm, NULL);
	  armAllIcon = new MPIXMAP(":/images/icons/globe-edit.png", NULL);
      globalKeysIcon = new MPIXMAP(":/images/icons/globe.png", NULL);

      editmuteIcon  = new MPIXMAP(":/images/icons/mute-all.png", NULL);
      editmuteSIcon = new MPIXMAP(editmuteS_xpm, NULL);
      panicIcon  = new MPIXMAP(":/images/icons/transport-panic.png", NULL);
      feedbackIcon  = new MPIXMAP(":/images/icons/feedback.png", NULL);

      editcutIconSet       = new MICON(editcutS_xpm, NULL);//"edit-cut"); // ddskrjo
      editcopyIconSet      = new MICON(editcopyS_xpm, NULL);//"edit-copy");
      editpasteIconSet     = new MICON(editpasteS_xpm, NULL);//"edit-paste");
      editmuteIconSet      = new MICON(editmuteS_xpm, NULL);//"audio-volume-muted");
      editpaste2TrackIconSet = new MICON(editpaste2trackS_xpm, NULL);
      editpasteCloneIconSet  = new MICON(editpastecloneS_xpm, NULL);
      editpasteClone2TrackIconSet = new MICON(editpasteclone2trackS_xpm, NULL); // ..
      exitIcon             = new MPIXMAP(":/images/icons/mixer-exit.png", NULL);//"application-exit");
      exit1Icon            = new MPIXMAP(":/images/icons/mixer-exit_on.png", NULL);//"application-exit");

      // 2 lines odd code
      newmuteIcon          = new MPIXMAP(newmutebutton_xpm, NULL);
      soloIcon             = new MPIXMAP(solobutton_xpm, NULL);

      multiDisplay         = new MPIXMAP(":/images/icons/multi_display.png", NULL);
      previousPartIcon     = new MPIXMAP(":/images/icons/previous_part.png", NULL);
      nextPartIcon         = new MPIXMAP(":/images/icons/next_part.png", NULL);
      blankRecord          = new MPIXMAP(":/images/icons/blank_record.png", NULL);
      preIcon        	   = new MPIXMAP(":/images/icons/mixer-pre.png", NULL);
      preIconOn        	   = new MPIXMAP(":/images/icons/mixer-pre_on.png", NULL);
      mixerIn        	   = new MPIXMAP(":/images/icons/mixer-in.png", NULL);
      mixerOut        	   = new MPIXMAP(":/images/icons/mixer-out.png", NULL);
      recEchoIconOn        = new MPIXMAP(":/images/icons/mixer-record.png", NULL);
      recEchoIconOff       = new MPIXMAP(":/images/icons/mixer-record.png", NULL);
      muteIconOn           = new MPIXMAP(":/images/icons/mixer-mute_on.png", NULL);
      muteIconOff          = new MPIXMAP(":/images/icons/mixer-mute.png", NULL);
      muteIconOver         = new MPIXMAP(":/images/icons/mixer-mute_over.png", NULL);
      soloIconOn           = new MPIXMAP(":/images/icons/mixer-solo_on.png", NULL);
      soloIconOff          = new MPIXMAP(":/images/icons/mixer-solo.png", NULL);
      soloIconOver         = new MPIXMAP(":/images/icons/mixer-solo_over.png", NULL);
      soloblksqIconOn      = new MPIXMAP(":/images/icons/mixer-solo_on.png", NULL);
      soloblksqIconOff     = new MPIXMAP(":/images/icons/mixer-solo.png", NULL);
      soloblksqIconOver    = new MPIXMAP(":/images/icons/mixer-solo_over.png", NULL);
      transport_soloIconOn = new MPIXMAP(":/images/icons/transport-solo_new_on.png", NULL);
      transport_soloIconOff= new MPIXMAP(":/images/icons/transport-solo_new_off.png", NULL);
      transport_soloIconOver= new MPIXMAP(":/images/icons/transport-solo_new_over.png", NULL);
      soloIconSet1         = new QIcon(); 
      soloIconSet2         = new QIcon();
      soloIconSet3         = new QIcon();
      soloIconSet1->addPixmap(*soloIconOn, QIcon::Normal, QIcon::On);
      soloIconSet1->addPixmap(*soloIconOff, QIcon::Normal, QIcon::Off);
      soloIconSet1->addPixmap(*soloIconOver, QIcon::Active);
      soloIconSet2->addPixmap(*soloblksqIconOn, QIcon::Normal, QIcon::On);
      soloIconSet2->addPixmap(*soloblksqIconOff, QIcon::Normal, QIcon::Off);
      soloIconSet2->addPixmap(*soloblksqIconOver, QIcon::Active);
      soloIconSet3->addPixmap(*transport_soloIconOn, QIcon::Normal, QIcon::On);
      soloIconSet3->addPixmap(*transport_soloIconOff, QIcon::Normal, QIcon::Off);
      soloIconSet3->addPixmap(*transport_soloIconOver, QIcon::Active);
      
      transport_muteIconOn = new MPIXMAP(":/images/icons/transport-mute_new_on.png", NULL);
      transport_muteIconOff= new MPIXMAP(":/images/icons/transport-mute_new_off.png", NULL);
      transport_muteIconOver= new MPIXMAP(":/images/icons/transport-mute_new_over.png", NULL);
	  muteIconSet3         = new QIcon();
      muteIconSet3->addPixmap(*transport_muteIconOn, QIcon::Normal, QIcon::On);
      muteIconSet3->addPixmap(*transport_muteIconOff, QIcon::Normal, QIcon::Off);
      muteIconSet3->addPixmap(*transport_muteIconOver, QIcon::Active);
      
	  midiInIconOn = new MPIXMAP(":/images/icons/midiin_new_on.png", NULL);
      midiInIconOff= new MPIXMAP(":/images/icons/midiin_new_off.png", NULL);
      midiInIconOver= new MPIXMAP(":/images/icons/midiin_new_over.png", NULL);
	  midiInIconSet3         = new QIcon();
      midiInIconSet3->addPixmap(*midiInIconOn, QIcon::Normal, QIcon::On);
      midiInIconSet3->addPixmap(*midiInIconOff, QIcon::Normal, QIcon::Off);
      midiInIconSet3->addPixmap(*midiInIconOver, QIcon::Active);
	  muteIconSet4         = new QIcon();
	  transport_recordIconOn = new MPIXMAP(":/images/icons/transport-record_new_on.png", NULL);
      transport_recordIconOff = new MPIXMAP(":/images/icons/transport-record_new_off.png", NULL);
      transport_recordIconOver = new MPIXMAP(":/images/icons/transport-record_new_over.png", NULL);
	  recordIconSet3         = new QIcon();
      recordIconSet3->addPixmap(*transport_recordIconOn, QIcon::Normal, QIcon::On);
      recordIconSet3->addPixmap(*transport_recordIconOff, QIcon::Normal, QIcon::Off);
      recordIconSet3->addPixmap(*transport_recordIconOver, QIcon::Active);
	  
	  transport_playIconOn = new MPIXMAP(":/images/icons/transport-play_new_on.png", NULL);
      transport_playIconOff= new MPIXMAP(":/images/icons/transport-play_new_off.png", NULL);
      transport_playIconOver= new MPIXMAP(":/images/icons/transport-play_new_over.png", NULL);
	  playIconSet3         = new QIcon();
      playIconSet3->addPixmap(*transport_playIconOn, QIcon::Normal, QIcon::On);
      playIconSet3->addPixmap(*transport_playIconOff, QIcon::Normal, QIcon::Off);
      playIconSet3->addPixmap(*transport_playIconOver, QIcon::Active);
	  
	  transport_playIconRightOn = new MPIXMAP(":/images/icons/transport-play_new_end_on.png", NULL);
      transport_playIconRightOff= new MPIXMAP(":/images/icons/transport-play_new_end_off.png", NULL);
      transport_playIconRightOver= new MPIXMAP(":/images/icons/transport-play_new_end_over.png", NULL);
	  playIconSetRight         = new QIcon();
      playIconSetRight->addPixmap(*transport_playIconRightOn, QIcon::Normal, QIcon::On);
      playIconSetRight->addPixmap(*transport_playIconRightOff, QIcon::Normal, QIcon::Off);
      playIconSetRight->addPixmap(*transport_playIconRightOver, QIcon::Active);
	  
	  transport_stopIconLeftOn = new MPIXMAP(":/images/icons/transport-stop_new_end_on.png", NULL);
      transport_stopIconLeftOff= new MPIXMAP(":/images/icons/transport-stop_new_end_off.png", NULL);
      transport_stopIconLeftOver= new MPIXMAP(":/images/icons/transport-stop_new_end_over.png", NULL);
	  stopIconSetLeft         = new QIcon();
      stopIconSetLeft->addPixmap(*transport_stopIconLeftOn, QIcon::Normal, QIcon::On);
      stopIconSetLeft->addPixmap(*transport_stopIconLeftOff, QIcon::Normal, QIcon::Off);
      stopIconSetLeft->addPixmap(*transport_stopIconLeftOver, QIcon::Active);
	  
      transport_startIconOff= new MPIXMAP(":/images/icons/transport-rewind-end_new_off.png", NULL);
      transport_startIconOver= new MPIXMAP(":/images/icons/transport-rewind-end_new_over.png", NULL);
	  startIconSet3         = new QIcon();
      startIconSet3->addPixmap(*transport_startIconOff, QIcon::Normal, QIcon::Off);
      startIconSet3->addPixmap(*transport_startIconOver, QIcon::Active);
      
	  transport_rewindIconOff= new MPIXMAP(":/images/icons/transport-rewind_new_off.png", NULL);
      transport_rewindIconOver= new MPIXMAP(":/images/icons/transport-rewind_new_over.png", NULL);
	  rewindIconSet3         = new QIcon();
      rewindIconSet3->addPixmap(*transport_rewindIconOff, QIcon::Normal, QIcon::Off);
      rewindIconSet3->addPixmap(*transport_rewindIconOver, QIcon::Active);
	  
      transport_forwardIconOff= new MPIXMAP(":/images/icons/transport-ffwd_new_off.png", NULL);
      transport_forwardIconOver= new MPIXMAP(":/images/icons/transport-ffwd_new_over.png", NULL);
	  forwardIconSet3         = new QIcon();
      forwardIconSet3->addPixmap(*transport_forwardIconOff, QIcon::Normal, QIcon::Off);
      forwardIconSet3->addPixmap(*transport_forwardIconOver, QIcon::Active);
	  
	  transport_stopIconOn = new MPIXMAP(":/images/icons/transport-stop_new_on.png", NULL);
      transport_stopIconOff= new MPIXMAP(":/images/icons/transport-stop_new_off.png", NULL);
      transport_stopIconOver= new MPIXMAP(":/images/icons/transport-stop_new_over.png", NULL);
	  stopIconSet3         = new QIcon();
      stopIconSet3->addPixmap(*transport_stopIconOn, QIcon::Normal, QIcon::On);
      stopIconSet3->addPixmap(*transport_stopIconOff, QIcon::Normal, QIcon::Off);
      stopIconSet3->addPixmap(*transport_stopIconOver, QIcon::Active);
	  
	  pointerIconOn = new MPIXMAP(":/images/icons/pointer_new_on.png", NULL);
      pointerIconOff= new MPIXMAP(":/images/icons/pointer_new_off.png", NULL);
      pointerIconOver= new MPIXMAP(":/images/icons/pointer_new_over.png", NULL);
	  pointerIconSet3         = new QIcon();
      pointerIconSet3->addPixmap(*pointerIconOn, QIcon::Normal, QIcon::On);
      pointerIconSet3->addPixmap(*pointerIconOff, QIcon::Normal, QIcon::Off);
      pointerIconSet3->addPixmap(*pointerIconOver, QIcon::Active);
	  
	  deleteIconOn = new MPIXMAP(":/images/icons/eraser_new_on.png", NULL);
      deleteIconOff= new MPIXMAP(":/images/icons/eraser_new_off.png", NULL);
      deleteIconOver= new MPIXMAP(":/images/icons/eraser_new_over.png", NULL);
	  deleteIconSet3         = new QIcon();
      deleteIconSet3->addPixmap(*deleteIconOn, QIcon::Normal, QIcon::On);
      deleteIconSet3->addPixmap(*deleteIconOff, QIcon::Normal, QIcon::Off);
      deleteIconSet3->addPixmap(*deleteIconOver, QIcon::Active);
	  
	  pencilIconOn = new MPIXMAP(":/images/icons/pencil_new_on.png", NULL);
      pencilIconOff= new MPIXMAP(":/images/icons/pencil_new_off.png", NULL);
      pencilIconOver= new MPIXMAP(":/images/icons/pencil_new_over.png", NULL);
	  pencilIconSet3         = new QIcon();
      pencilIconSet3->addPixmap(*pencilIconOn, QIcon::Normal, QIcon::On);
      pencilIconSet3->addPixmap(*pencilIconOff, QIcon::Normal, QIcon::Off);
      pencilIconSet3->addPixmap(*pencilIconOver, QIcon::Active);
	  
	  cutIconOn = new MPIXMAP(":/images/icons/split_new_on.png", NULL);
      cutIconOff= new MPIXMAP(":/images/icons/split_new_off.png", NULL);
      cutIconOver= new MPIXMAP(":/images/icons/split_new_over.png", NULL);
	  cutIconSet3         = new QIcon();
      cutIconSet3->addPixmap(*cutIconOn, QIcon::Normal, QIcon::On);
      cutIconSet3->addPixmap(*cutIconOff, QIcon::Normal, QIcon::Off);
      cutIconSet3->addPixmap(*cutIconOver, QIcon::Active);
	  
	  note1IconSet3         = new QIcon();
	  
	  glueIconOn = new MPIXMAP(":/images/icons/join_new_on.png", NULL);
      glueIconOff= new MPIXMAP(":/images/icons/join_new_off.png", NULL);
      glueIconOver= new MPIXMAP(":/images/icons/join_new_over.png", NULL);
	  glueIconSet3         = new QIcon();
      glueIconSet3->addPixmap(*glueIconOn, QIcon::Normal, QIcon::On);
      glueIconSet3->addPixmap(*glueIconOff, QIcon::Normal, QIcon::Off);
      glueIconSet3->addPixmap(*glueIconOver, QIcon::Active);
	  
	  quantIconSet3         = new QIcon();
	  
	  drawIconOn = new MPIXMAP(":/images/icons/linetool_new_on.png", NULL);
      drawIconOff= new MPIXMAP(":/images/icons/linetool_new_off.png", NULL);
      drawIconOver= new MPIXMAP(":/images/icons/linetool_new_over.png", NULL);
	  drawIconSet3         = new QIcon();
      drawIconSet3->addPixmap(*drawIconOn, QIcon::Normal, QIcon::On);
      drawIconSet3->addPixmap(*drawIconOff, QIcon::Normal, QIcon::Off);
      drawIconSet3->addPixmap(*drawIconOver, QIcon::Active);
	  
	  stretchIconOn = new MPIXMAP(":/images/icons/stretchtool_new_on.png", NULL);
      stretchIconOff= new MPIXMAP(":/images/icons/stretchtool_new_off.png", NULL);
      stretchIconOver= new MPIXMAP(":/images/icons/stretchtool_new_over.png", NULL);
	  stretchIconSet3         = new QIcon();
      stretchIconSet3->addPixmap(*stretchIconOn, QIcon::Normal, QIcon::On);
      stretchIconSet3->addPixmap(*stretchIconOff, QIcon::Normal, QIcon::Off);
      stretchIconSet3->addPixmap(*stretchIconOver, QIcon::Active);
	  
	  multiDisplayIconOn = new MPIXMAP(":/images/icons/epicParts_new_on.png", NULL);
      multiDisplayIconOff= new MPIXMAP(":/images/icons/epicParts_new_off.png", NULL);
      multiDisplayIconOver= new MPIXMAP(":/images/icons/epicParts_new_over.png", NULL);
	  multiDisplayIconSet3         = new QIcon();
      multiDisplayIconSet3->addPixmap(*multiDisplayIconOn, QIcon::Normal, QIcon::On);
      multiDisplayIconSet3->addPixmap(*multiDisplayIconOff, QIcon::Normal, QIcon::Off);
      multiDisplayIconSet3->addPixmap(*multiDisplayIconOver, QIcon::Active);
	  
	  selectMultiIconOn = new MPIXMAP(":/images/icons/epicSelect_new_on.png", NULL);
      selectMultiIconOff= new MPIXMAP(":/images/icons/epicSelect_new_off.png", NULL);
      selectMultiIconOver= new MPIXMAP(":/images/icons/epicSelect_new_over.png", NULL);
	  selectMultiIconSet3         = new QIcon();
      selectMultiIconSet3->addPixmap(*selectMultiIconOn, QIcon::Normal, QIcon::On);
      selectMultiIconSet3->addPixmap(*selectMultiIconOff, QIcon::Normal, QIcon::Off);
      selectMultiIconSet3->addPixmap(*selectMultiIconOver, QIcon::Active);
	  
	  auditionIconOn = new MPIXMAP(":/images/icons/audition_new_on.png", NULL);
      auditionIconOff= new MPIXMAP(":/images/icons/audition_new_off.png", NULL);
      auditionIconOver= new MPIXMAP(":/images/icons/audition_new_over.png", NULL);
	  auditionIconSet3         = new QIcon();
      auditionIconSet3->addPixmap(*auditionIconOn, QIcon::Normal, QIcon::On);
      auditionIconSet3->addPixmap(*auditionIconOff, QIcon::Normal, QIcon::Off);
      auditionIconSet3->addPixmap(*auditionIconOver, QIcon::Active);
	  
	  feedbackIconOn = new MPIXMAP(":/images/icons/feedback_new_on.png", NULL);
      feedbackIconOff= new MPIXMAP(":/images/icons/feedback_new_off.png", NULL);
      feedbackIconOver= new MPIXMAP(":/images/icons/feedback_new_over.png", NULL);
	  feedbackIconSet3         = new QIcon();
      feedbackIconSet3->addPixmap(*feedbackIconOn, QIcon::Normal, QIcon::On);
      feedbackIconSet3->addPixmap(*feedbackIconOff, QIcon::Normal, QIcon::Off);
      feedbackIconSet3->addPixmap(*feedbackIconOver, QIcon::Active);
	  
	  globalKeysIconOn = new MPIXMAP(":/images/icons/epicDraw_new_on.png", NULL);
      globalKeysIconOff= new MPIXMAP(":/images/icons/epicDraw_new_off.png", NULL);
      globalKeysIconOver= new MPIXMAP(":/images/icons/epicDraw_new_over.png", NULL);
	  globalKeysIconSet3         = new QIcon();
      globalKeysIconSet3->addPixmap(*globalKeysIconOn, QIcon::Normal, QIcon::On);
      globalKeysIconSet3->addPixmap(*globalKeysIconOff, QIcon::Normal, QIcon::Off);
      globalKeysIconSet3->addPixmap(*globalKeysIconOver, QIcon::Active);
	  
	  stepIconOn = new MPIXMAP(":/images/icons/step_new_on.png", NULL);
      stepIconOff= new MPIXMAP(":/images/icons/step_new_off.png", NULL);
      stepIconOver= new MPIXMAP(":/images/icons/step_new_over.png", NULL);
	  stepIconSet3         = new QIcon();
      stepIconSet3->addPixmap(*stepIconOn, QIcon::Normal, QIcon::On);
      stepIconSet3->addPixmap(*stepIconOff, QIcon::Normal, QIcon::Off);
      stepIconSet3->addPixmap(*stepIconOver, QIcon::Active);
	  
	  punchinIconOn = new MPIXMAP(":/images/icons/left_new_on.png", NULL);
      punchinIconOff= new MPIXMAP(":/images/icons/left_new_off.png", NULL);
      punchinIconOver= new MPIXMAP(":/images/icons/left_new_over.png", NULL);
	  punchinIconSet3         = new QIcon();
      punchinIconSet3->addPixmap(*punchinIconOn, QIcon::Normal, QIcon::On);
      punchinIconSet3->addPixmap(*punchinIconOff, QIcon::Normal, QIcon::Off);
      punchinIconSet3->addPixmap(*punchinIconOver, QIcon::Active);
	  
	  punchoutIconOn = new MPIXMAP(":/images/icons/right_new_on.png", NULL);
      punchoutIconOff= new MPIXMAP(":/images/icons/right_new_off.png", NULL);
      punchoutIconOver= new MPIXMAP(":/images/icons/right_new_over.png", NULL);
	  punchoutIconSet3         = new QIcon();
      punchoutIconSet3->addPixmap(*punchoutIconOn, QIcon::Normal, QIcon::On);
      punchoutIconSet3->addPixmap(*punchoutIconOff, QIcon::Normal, QIcon::Off);
      punchoutIconSet3->addPixmap(*punchoutIconOver, QIcon::Active);
	  
	  loopIconOn = new MPIXMAP(":/images/icons/loop_new_on.png", NULL);
      loopIconOff= new MPIXMAP(":/images/icons/loop_new_off.png", NULL);
      loopIconOver= new MPIXMAP(":/images/icons/loop_new_over.png", NULL);
	  loopIconSet3         = new QIcon();
      loopIconSet3->addPixmap(*loopIconOn, QIcon::Normal, QIcon::On);
      loopIconSet3->addPixmap(*loopIconOff, QIcon::Normal, QIcon::Off);
      loopIconSet3->addPixmap(*loopIconOver, QIcon::Active);
	  
	  punchinVertIconOn = new MPIXMAP(":/images/icons/left_new_on_v.png", NULL);
      punchinVertIconOff= new MPIXMAP(":/images/icons/left_new_off_v.png", NULL);
      punchinVertIconOver= new MPIXMAP(":/images/icons/left_new_over_v.png", NULL);
	  punchinVertIconSet3         = new QIcon();
      punchinVertIconSet3->addPixmap(*punchinVertIconOn, QIcon::Normal, QIcon::On);
      punchinVertIconSet3->addPixmap(*punchinVertIconOff, QIcon::Normal, QIcon::Off);
      punchinVertIconSet3->addPixmap(*punchinVertIconOver, QIcon::Active);
	  
	  punchoutVertIconOn = new MPIXMAP(":/images/icons/right_new_on_v.png", NULL);
      punchoutVertIconOff= new MPIXMAP(":/images/icons/right_new_off_v.png", NULL);
      punchoutVertIconOver= new MPIXMAP(":/images/icons/right_new_over_v.png", NULL);
	  punchoutVertIconSet3         = new QIcon();
      punchoutVertIconSet3->addPixmap(*punchoutVertIconOn, QIcon::Normal, QIcon::On);
      punchoutVertIconSet3->addPixmap(*punchoutVertIconOff, QIcon::Normal, QIcon::Off);
      punchoutVertIconSet3->addPixmap(*punchoutVertIconOver, QIcon::Active);
	  
	  loopVertIconOn = new MPIXMAP(":/images/icons/loop_new_on_v.png", NULL);
      loopVertIconOff= new MPIXMAP(":/images/icons/loop_new_off_v.png", NULL);
      loopVertIconOver= new MPIXMAP(":/images/icons/loop_new_over_v.png", NULL);
	  loopVertIconSet3         = new QIcon();
      loopVertIconSet3->addPixmap(*loopVertIconOn, QIcon::Normal, QIcon::On);
      loopVertIconSet3->addPixmap(*loopVertIconOff, QIcon::Normal, QIcon::Off);
      loopVertIconSet3->addPixmap(*loopVertIconOver, QIcon::Active);
	  
	  expandIconOn = new MPIXMAP(":/images/expand_new_on.png", NULL);
      expandIconOff= new MPIXMAP(":/images/expand_new_off.png", NULL);
      expandIconOver= new MPIXMAP(":/images/expand_new_over.png", NULL);
	  expandIconSet3         = new QIcon();
      expandIconSet3->addPixmap(*expandIconOn, QIcon::Normal, QIcon::On);
      expandIconSet3->addPixmap(*expandIconOff, QIcon::Normal, QIcon::Off);
      expandIconSet3->addPixmap(*expandIconOver, QIcon::Active);
	  
	  vuIconOn = new MPIXMAP(":/images/vu_new_on.png", NULL);
      vuIconOff= new MPIXMAP(":/images/vu_new_off.png", NULL);
      vuIconOver= new MPIXMAP(":/images/vu_new_over.png", NULL);
	  vuIconSet3         = new QIcon();
      vuIconSet3->addPixmap(*vuIconOn, QIcon::Normal, QIcon::On);
      vuIconSet3->addPixmap(*vuIconOff, QIcon::Normal, QIcon::Off);
      vuIconSet3->addPixmap(*vuIconOver, QIcon::Active);
	  
	  refreshIconOn = new MPIXMAP(":/images/refresh_new_on.png", NULL);
      refreshIconOff= new MPIXMAP(":/images/refresh_new_off.png", NULL);
      refreshIconOver= new MPIXMAP(":/images/refresh_new_over.png", NULL);
	  refreshIconSet3         = new QIcon();
      refreshIconSet3->addPixmap(*refreshIconOff, QIcon::Normal, QIcon::Off);
      refreshIconSet3->addPixmap(*refreshIconOver, QIcon::Active);
	  
	  speakerIconOn = new MPIXMAP(":/images/icons/speaker_new_on.png", NULL);
      speakerIconOff= new MPIXMAP(":/images/icons/speaker_new_off.png", NULL);
      speakerIconOver= new MPIXMAP(":/images/icons/speaker_new_over.png", NULL);
	  speakerIconSet3         = new QIcon();
      speakerIconSet3->addPixmap(*speakerIconOn, QIcon::Normal, QIcon::On);
      speakerIconSet3->addPixmap(*speakerIconOff, QIcon::Normal, QIcon::Off);
      speakerIconSet3->addPixmap(*speakerIconOver, QIcon::Active);
	  
	  globalArmIconOver = new MPIXMAP(":/images/icons/epicRecord_new_over.png", NULL);
      globalArmIconOff= new MPIXMAP(":/images/icons/epicRecord_new_off.png", NULL);
	  globalArmIconSet3         = new QIcon();
      globalArmIconSet3->addPixmap(*globalArmIconOff, QIcon::Normal, QIcon::Off);
      globalArmIconSet3->addPixmap(*globalArmIconOver, QIcon::Active);
	  
	  panicIconOver = new MPIXMAP(":/images/icons/panic_new_over.png", NULL);
      panicIconOff= new MPIXMAP(":/images/icons/panic_new_off.png", NULL);
	  panicIconSet3         = new QIcon();
      panicIconSet3->addPixmap(*panicIconOff, QIcon::Normal, QIcon::Off);
      panicIconSet3->addPixmap(*panicIconOver, QIcon::Active);
	  
      mixer_resize_OffIcon= new MPIXMAP(":/images/icons/mixer_resize_new_off.png", NULL);
      mixer_resize_OverIcon= new MPIXMAP(":/images/icons/mixer_resize_new_over.png", NULL);
	  mixer_resizeIconSet3         = new QIcon();
      mixer_resizeIconSet3->addPixmap(*mixer_resize_OffIcon, QIcon::Normal, QIcon::Off);
      mixer_resizeIconSet3->addPixmap(*mixer_resize_OverIcon, QIcon::Active);
	  
	  mixer_record_OnIcon = new MPIXMAP(":/images/icons/mixer_record_new_on.png", NULL);
      mixer_record_OffIcon= new MPIXMAP(":/images/icons/mixer_record_new_off.png", NULL);
      mixer_record_OverIcon= new MPIXMAP(":/images/icons/mixer_record_new_over.png", NULL);
	  mixer_recordIconSet3         = new QIcon();
      mixer_recordIconSet3->addPixmap(*mixer_record_OnIcon, QIcon::Normal, QIcon::On);
      mixer_recordIconSet3->addPixmap(*mixer_record_OffIcon, QIcon::Normal, QIcon::Off);
      mixer_recordIconSet3->addPixmap(*mixer_record_OverIcon, QIcon::Active);
	  
	  mixer_mute_OnIcon = new MPIXMAP(":/images/icons/mixer_mute_new_on.png", NULL);
      mixer_mute_OffIcon= new MPIXMAP(":/images/icons/mixer_mute_new_off.png", NULL);
      mixer_mute_OverIcon= new MPIXMAP(":/images/icons/mixer_mute_new_over.png", NULL);
	  mixer_muteIconSet3         = new QIcon();
      mixer_muteIconSet3->addPixmap(*mixer_mute_OnIcon, QIcon::Normal, QIcon::On);
      mixer_muteIconSet3->addPixmap(*mixer_mute_OffIcon, QIcon::Normal, QIcon::Off);
      mixer_muteIconSet3->addPixmap(*mixer_mute_OverIcon, QIcon::Active);
	  
	  mixer_solo_OnIcon = new MPIXMAP(":/images/icons/mixer_solo_new_on.png", NULL);
      mixer_solo_OffIcon= new MPIXMAP(":/images/icons/mixer_solo_new_off.png", NULL);
      mixer_solo_OverIcon= new MPIXMAP(":/images/icons/mixer_solo_new_over.png", NULL);
	  mixer_soloIconSet3         = new QIcon();
      mixer_soloIconSet3->addPixmap(*mixer_solo_OnIcon, QIcon::Normal, QIcon::On);
      mixer_soloIconSet3->addPixmap(*mixer_solo_OffIcon, QIcon::Normal, QIcon::Off);
      mixer_soloIconSet3->addPixmap(*mixer_solo_OverIcon, QIcon::Active);
	  
      mixer_input_OffIcon= new MPIXMAP(":/images/icons/mixer_input_new_off.png", NULL);
      mixer_input_OverIcon= new MPIXMAP(":/images/icons/mixer_input_new_over.png", NULL);
	  mixer_inputIconSet3         = new QIcon();
      mixer_inputIconSet3->addPixmap(*mixer_input_OffIcon, QIcon::Normal, QIcon::Off);
      mixer_inputIconSet3->addPixmap(*mixer_input_OverIcon, QIcon::Active);
	  
      mixer_output_OffIcon= new MPIXMAP(":/images/icons/mixer_output_new_off.png", NULL);
      mixer_output_OverIcon= new MPIXMAP(":/images/icons/mixer_output_new_over.png", NULL);
	  mixer_outputIconSet3         = new QIcon();
      mixer_outputIconSet3->addPixmap(*mixer_output_OffIcon, QIcon::Normal, QIcon::Off);
      mixer_outputIconSet3->addPixmap(*mixer_output_OverIcon, QIcon::Active);
	  
      mixer_power_OnIcon= new MPIXMAP(":/images/icons/mixer_power_new_on.png", NULL);
      mixer_power_OffIcon= new MPIXMAP(":/images/icons/mixer_power_new_off.png", NULL);
      mixer_power_OverIcon= new MPIXMAP(":/images/icons/mixer_power_new_over.png", NULL);
	  mixer_powerIconSet3         = new QIcon();
      mixer_powerIconSet3->addPixmap(*mixer_power_OnIcon, QIcon::Normal, QIcon::On);
      mixer_powerIconSet3->addPixmap(*mixer_power_OffIcon, QIcon::Normal, QIcon::Off);
      mixer_powerIconSet3->addPixmap(*mixer_power_OverIcon, QIcon::Active);
      
	  mixer_blank_OffIcon= new MPIXMAP(":/images/icons/mixer_blank_new_off.png", NULL);
	  
      mixer_stereo_OnIcon= new MPIXMAP(":/images/icons/mixer_stereo_new_on.png", NULL);
      mixer_mono_OnIcon= new MPIXMAP(":/images/icons/mixer_mono_new_on.png", NULL);
	  mixer_stereoIconSet3         = new QIcon();
      mixer_stereoIconSet3->addPixmap(*mixer_stereo_OnIcon, QIcon::Normal, QIcon::On);
      mixer_stereoIconSet3->addPixmap(*mixer_mono_OnIcon, QIcon::Normal, QIcon::Off);
	  
      mixer_OnIcon= new MPIXMAP(":/images/icons/mixer_new_on.png", NULL);
      mixer_OffIcon= new MPIXMAP(":/images/icons/mixer_new_off.png", NULL);
      mixer_OverIcon= new MPIXMAP(":/images/icons/mixer_new_over.png", NULL);
	  mixerIconSet3         = new QIcon();
      mixerIconSet3->addPixmap(*mixer_OnIcon, QIcon::Normal, QIcon::On);
      mixerIconSet3->addPixmap(*mixer_OffIcon, QIcon::Normal, QIcon::Off);
      mixerIconSet3->addPixmap(*mixer_OverIcon, QIcon::Active);
	  
      pcloader_OnIcon= new MPIXMAP(":/images/icons/pcloader_new_on.png", NULL);
      pcloader_OffIcon= new MPIXMAP(":/images/icons/pcloader_new_off.png", NULL);
      pcloader_OverIcon= new MPIXMAP(":/images/icons/pcloader_new_over.png", NULL);
	  pcloaderIconSet3         = new QIcon();
      pcloaderIconSet3->addPixmap(*pcloader_OnIcon, QIcon::Normal, QIcon::On);
      pcloaderIconSet3->addPixmap(*pcloader_OffIcon, QIcon::Normal, QIcon::Off);
      pcloaderIconSet3->addPixmap(*pcloader_OverIcon, QIcon::Active);
	  
      enabled_OnIcon= new MPIXMAP(":/images/icons/enabled_new_on.png", NULL);
      enabled_OffIcon= new MPIXMAP(":/images/icons/enabled_new_off.png", NULL);
      enabled_OverIcon= new MPIXMAP(":/images/icons/enabled_new_over.png", NULL);
	  enabledIconSet3         = new QIcon();
      enabledIconSet3->addPixmap(*enabled_OnIcon, QIcon::Normal, QIcon::On);
      enabledIconSet3->addPixmap(*enabled_OffIcon, QIcon::Normal, QIcon::Off);
      enabledIconSet3->addPixmap(*enabled_OverIcon, QIcon::Active);
	  
      up_arrow_OnIcon= new MPIXMAP(":/images/up_arrow_new_on.png", NULL);
      up_arrow_OffIcon= new MPIXMAP(":/images/up_arrow_new_off.png", NULL);
      up_arrow_OverIcon= new MPIXMAP(":/images/up_arrow_new_over.png", NULL);
	  up_arrowIconSet3         = new QIcon();
      up_arrowIconSet3->addPixmap(*up_arrow_OnIcon, QIcon::Normal, QIcon::On);
      up_arrowIconSet3->addPixmap(*up_arrow_OffIcon, QIcon::Normal, QIcon::Off);
      up_arrowIconSet3->addPixmap(*up_arrow_OverIcon, QIcon::Active);
	  
      down_arrow_OnIcon= new MPIXMAP(":/images/down_arrow_new_on.png", NULL);
      down_arrow_OffIcon= new MPIXMAP(":/images/down_arrow_new_off.png", NULL);
      down_arrow_OverIcon= new MPIXMAP(":/images/down_arrow_new_over.png", NULL);
	  down_arrowIconSet3         = new QIcon();
      down_arrowIconSet3->addPixmap(*down_arrow_OnIcon, QIcon::Normal, QIcon::On);
      down_arrowIconSet3->addPixmap(*down_arrow_OffIcon, QIcon::Normal, QIcon::Off);
      down_arrowIconSet3->addPixmap(*down_arrow_OverIcon, QIcon::Active);
	  
	  collapseIconSet3         = new QIcon();
      collapseIconSet3->addPixmap(*up_arrow_OnIcon, QIcon::Normal, QIcon::On);
      collapseIconSet3->addPixmap(*down_arrow_OffIcon, QIcon::Normal, QIcon::Off);
      collapseIconSet3->addPixmap(*up_arrow_OverIcon, QIcon::Active, QIcon::On);
      collapseIconSet3->addPixmap(*down_arrow_OverIcon, QIcon::Active, QIcon::Off);
	  
      plus_OnIcon= new MPIXMAP(":/images/plus_new_on.png", NULL);
      plus_OffIcon= new MPIXMAP(":/images/plus_new_off.png", NULL);
      plus_OverIcon= new MPIXMAP(":/images/plus_new_over.png", NULL);
	  plusIconSet3         = new QIcon();
      plusIconSet3->addPixmap(*plus_OnIcon, QIcon::Normal, QIcon::On);
      plusIconSet3->addPixmap(*plus_OffIcon, QIcon::Normal, QIcon::Off);
      plusIconSet3->addPixmap(*plus_OverIcon, QIcon::Active);
	  
      browse_OnIcon= new MPIXMAP(":/images/browse_new_on.png", NULL);
      browse_OffIcon= new MPIXMAP(":/images/browse_new_off.png", NULL);
      browse_OverIcon= new MPIXMAP(":/images/browse_new_over.png", NULL);
	  browseIconSet3         = new QIcon();
      browseIconSet3->addPixmap(*browse_OnIcon, QIcon::Normal, QIcon::On);
      browseIconSet3->addPixmap(*browse_OffIcon, QIcon::Normal, QIcon::Off);
      browseIconSet3->addPixmap(*browse_OverIcon, QIcon::Active);
	  
      garbage_OnIcon= new MPIXMAP(":/images/garbage_new_on.png", NULL);
      garbage_OffIcon= new MPIXMAP(":/images/garbage_new_off.png", NULL);
      garbage_OverIcon= new MPIXMAP(":/images/garbage_new_over.png", NULL);
	  garbageIconSet3         = new QIcon();
      garbageIconSet3->addPixmap(*garbage_OnIcon, QIcon::Normal, QIcon::On);
      garbageIconSet3->addPixmap(*garbage_OffIcon, QIcon::Normal, QIcon::Off);
      garbageIconSet3->addPixmap(*garbage_OverIcon, QIcon::Active);
	  
      duplicate_OnIcon= new MPIXMAP(":/images/duplicate_new_on.png", NULL);
      duplicate_OffIcon= new MPIXMAP(":/images/duplicate_new_off.png", NULL);
      duplicate_OverIcon= new MPIXMAP(":/images/duplicate_new_over.png", NULL);
	  duplicateIconSet3         = new QIcon();
      duplicateIconSet3->addPixmap(*duplicate_OnIcon, QIcon::Normal, QIcon::On);
      duplicateIconSet3->addPixmap(*duplicate_OffIcon, QIcon::Normal, QIcon::Off);
      duplicateIconSet3->addPixmap(*duplicate_OverIcon, QIcon::Active);
	  
      comment_OnIcon= new MPIXMAP(":/images/comment_new_on.png", NULL);
      comment_OffIcon= new MPIXMAP(":/images/comment_new_off.png", NULL);
      comment_OverIcon= new MPIXMAP(":/images/comment_new_over.png", NULL);
	  commentIconSet3         = new QIcon();
      commentIconSet3->addPixmap(*comment_OnIcon, QIcon::Normal, QIcon::On);
      commentIconSet3->addPixmap(*comment_OffIcon, QIcon::Normal, QIcon::Off);
      commentIconSet3->addPixmap(*comment_OverIcon, QIcon::Active);
	  
      metronome_OnIcon= new MPIXMAP(":/images/icons/metronome_new_on.png", NULL);
      metronome_OffIcon= new MPIXMAP(":/images/icons/metronome_new_off.png", NULL);
      metronome_OverIcon= new MPIXMAP(":/images/icons/metronome_new_over.png", NULL);
	  metronomeIconSet3         = new QIcon();
      metronomeIconSet3->addPixmap(*metronome_OnIcon, QIcon::Normal, QIcon::On);
      metronomeIconSet3->addPixmap(*metronome_OffIcon, QIcon::Normal, QIcon::Off);
      metronomeIconSet3->addPixmap(*metronome_OverIcon, QIcon::Active);
      
	  dragMidiIcon= new MPIXMAP(":/images/drag_midi.png", NULL);
	  dragAudioIcon= new MPIXMAP(":/images/drag_audio.png", NULL);
	  dragInputIcon= new MPIXMAP(":/images/drag_input.png", NULL);
	  dragOutputIcon= new MPIXMAP(":/images/drag_out.png", NULL);
	  dragAuxIcon= new MPIXMAP(":/images/drag_aux.png", NULL);
	  dragBussIcon= new MPIXMAP(":/images/drag_buss.png", NULL);
	  
      transpose_OnIcon= new MPIXMAP(":/images/transpose_new_on.png", NULL);
      transpose_OffIcon= new MPIXMAP(":/images/transpose_new_off.png", NULL);
      transpose_OverIcon= new MPIXMAP(":/images/transpose_new_over.png", NULL);
	  transposeIconSet3         = new QIcon();
      transposeIconSet3->addPixmap(*transpose_OnIcon, QIcon::Normal, QIcon::On);
      transposeIconSet3->addPixmap(*transpose_OffIcon, QIcon::Normal, QIcon::Off);
      transposeIconSet3->addPixmap(*transpose_OverIcon, QIcon::Active);
	  
      connect_OnIcon= new MPIXMAP(":/images/connect_new_on.png", NULL);
      connect_OffIcon= new MPIXMAP(":/images/connect_new_off.png", NULL);
      connect_OverIcon= new MPIXMAP(":/images/connect_new_over.png", NULL);
	  connectIconSet3         = new QIcon();
      connectIconSet3->addPixmap(*connect_OnIcon, QIcon::Normal, QIcon::On);
      connectIconSet3->addPixmap(*connect_OffIcon, QIcon::Normal, QIcon::Off);
      connectIconSet3->addPixmap(*connect_OverIcon, QIcon::Active);
	  
      input_OnIcon= new MPIXMAP(":/images/input_new_on.png", NULL);
      input_OffIcon= new MPIXMAP(":/images/input_new_off.png", NULL);
      input_OverIcon= new MPIXMAP(":/images/input_new_over.png", NULL);
	  inputIconSet3         = new QIcon();
      inputIconSet3->addPixmap(*input_OnIcon, QIcon::Normal, QIcon::On);
      inputIconSet3->addPixmap(*input_OffIcon, QIcon::Normal, QIcon::Off);
      inputIconSet3->addPixmap(*input_OverIcon, QIcon::Active);
	  
      load_OnIcon= new MPIXMAP(":/images/load_new_on.png", NULL);
      load_OffIcon= new MPIXMAP(":/images/load_new_off.png", NULL);
      load_OverIcon= new MPIXMAP(":/images/load_new_over.png", NULL);
	  loadIconSet3         = new QIcon();
      loadIconSet3->addPixmap(*load_OnIcon, QIcon::Normal, QIcon::On);
      loadIconSet3->addPixmap(*load_OffIcon, QIcon::Normal, QIcon::Off);
      loadIconSet3->addPixmap(*load_OverIcon, QIcon::Active);
	  
      pluginGUI_OnIcon= new MPIXMAP(":/images/pluginGUI_new_on.png", NULL);
      pluginGUI_OffIcon= new MPIXMAP(":/images/pluginGUI_new_off.png", NULL);
      pluginGUI_OverIcon= new MPIXMAP(":/images/pluginGUI_new_over.png", NULL);
	  pluginGUIIconSet3         = new QIcon();
      pluginGUIIconSet3->addPixmap(*pluginGUI_OnIcon, QIcon::Normal, QIcon::On);
      pluginGUIIconSet3->addPixmap(*pluginGUI_OffIcon, QIcon::Normal, QIcon::Off);
      pluginGUIIconSet3->addPixmap(*pluginGUI_OverIcon, QIcon::Active);
	  
      route_edit_OnIcon= new MPIXMAP(":/images/route_edit_new_on.png", NULL);
      route_edit_OffIcon= new MPIXMAP(":/images/route_edit_new_off.png", NULL);
      route_edit_OverIcon= new MPIXMAP(":/images/route_edit_new_over.png", NULL);
	  route_editIconSet3         = new QIcon();
      route_editIconSet3->addPixmap(*route_edit_OnIcon, QIcon::Normal, QIcon::On);
      route_editIconSet3->addPixmap(*route_edit_OffIcon, QIcon::Normal, QIcon::Off);
      route_editIconSet3->addPixmap(*route_edit_OverIcon, QIcon::Active);
	  
      next_OnIcon= new MPIXMAP(":/images/next_new_on.png", NULL);
      next_OffIcon= new MPIXMAP(":/images/next_new_off.png", NULL);
      next_OverIcon= new MPIXMAP(":/images/next_new_over.png", NULL);
	  nextIconSet3         = new QIcon();
      nextIconSet3->addPixmap(*next_OnIcon, QIcon::Normal, QIcon::On);
      nextIconSet3->addPixmap(*next_OffIcon, QIcon::Normal, QIcon::Off);
      nextIconSet3->addPixmap(*next_OverIcon, QIcon::Active);
	  
      previous_OnIcon= new MPIXMAP(":/images/previous_new_on.png", NULL);
      previous_OffIcon= new MPIXMAP(":/images/previous_new_off.png", NULL);
      previous_OverIcon= new MPIXMAP(":/images/previous_new_over.png", NULL);
	  previousIconSet3         = new QIcon();
      previousIconSet3->addPixmap(*previous_OnIcon, QIcon::Normal, QIcon::On);
      previousIconSet3->addPixmap(*previous_OffIcon, QIcon::Normal, QIcon::Off);
      previousIconSet3->addPixmap(*previous_OverIcon, QIcon::Active);
	  
	  redLedIcon           = new MPIXMAP(redled_xpm, NULL);
      darkRedLedIcon       = new MPIXMAP(darkredled_xpm, NULL);
      greendotIcon         = new MPIXMAP(greendot_xpm, NULL);
      bluedotIcon          = new MPIXMAP(bluedot_xpm, NULL);
      graydotIcon          = new MPIXMAP(graydot_xpm, NULL);
      offIcon              = new MPIXMAP(off_xpm, NULL);
      blacksquareIcon      = new MPIXMAP(blacksquare_xpm, NULL);
      blacksqcheckIcon     = new MPIXMAP(blacksqcheck_xpm, NULL);

      mastertrackSIcon     = new MPIXMAP(mastertrackS_xpm, NULL);
      localoffSIcon        = new MPIXMAP(localoffS_xpm, NULL);
      miditransformSIcon   = new MPIXMAP(miditransformS_xpm, NULL);
      midi_plugSIcon       = new MPIXMAP(midi_plugS_xpm, NULL);
      miditransposeSIcon   = new MPIXMAP(":/images/icons/arrow_switch.png", NULL);
      commentIcon          = new MPIXMAP(":/images/icons/comment.png", NULL);
      midiThruOnIcon       = new MPIXMAP(midi_thru_on3_xpm, NULL);      
      midiThruOffIcon      = new MPIXMAP(midi_thru_off3_xpm, NULL);      
      
      mixerSIcon           = new MPIXMAP(mixerS_xpm, NULL);
      mustangSIcon         = new MPIXMAP(mustangS_xpm, NULL);
      resetSIcon           = new MPIXMAP(resetS_xpm, NULL);
      track_addIcon        = new MPIXMAP(track_add_xpm, NULL);
      track_deleteIcon     = new MPIXMAP(track_delete_xpm, NULL);
      listSIcon            = new MPIXMAP(listS_xpm, NULL);
      inputpluginSIcon     = new MPIXMAP(inputpluginS_xpm, NULL);
      cliplistSIcon        = new MPIXMAP(cliplistS_xpm, NULL);
      mixerAudioSIcon      = new MPIXMAP(":/images/icons/mixeraudioS.png", NULL);
      initSIcon            = new MPIXMAP(initS_xpm, NULL);
      songInfoIcon         = new MPIXMAP(":/images/icons/initSong.png", NULL);

      addMidiIcon     = new MPIXMAP(":/images/icons/views_midi_menu.png", NULL);
      addBussIcon     = new MPIXMAP(":/images/icons/views_busses_menu.png", NULL);
      addInputIcon    = new MPIXMAP(":/images/icons/views_inputs_menu.png", NULL);
      addOutputIcon   = new MPIXMAP(":/images/icons/views_outputs_menu.png", NULL);
      addAuxIcon      = new MPIXMAP(":/images/icons/views_auxs_menu.png", NULL);
      addAudioIcon    = new MPIXMAP(":/images/icons/views_audio_menu.png", NULL);
      addSynthIcon    = new MPIXMAP(":/images/icons/views_synth_menu.png", NULL);
      
	  addtrack_addmiditrackIcon     = new MPIXMAP(addtrack_addmiditrack_xpm, NULL);
      addtrack_audiogroupIcon       = new MPIXMAP(addtrack_audiogroup_xpm, NULL);
      addtrack_audioinputIcon       = new MPIXMAP(addtrack_audioinput_xpm, NULL);
      addtrack_audiooutputIcon      = new MPIXMAP(addtrack_audiooutput_xpm, NULL);
      addtrack_auxsendIcon          = new MPIXMAP(addtrack_auxsend_xpm, NULL);
      addtrack_drumtrackIcon        = new MPIXMAP(addtrack_drumtrack_xpm, NULL);
      addtrack_wavetrackIcon        = new MPIXMAP(addtrack_wavetrack_xpm, NULL);
      edit_drummsIcon               = new MPIXMAP(edit_drumms_xpm, NULL);
      edit_listIcon                 = new MPIXMAP(edit_list_xpm, NULL);
      remove_ctrlIcon               = new MPIXMAP(remove_ctrl_xpm, NULL);
      edit_waveIcon                 = new MPIXMAP(edit_wave_xpm, NULL);
      edit_mastertrackIcon          = new MPIXMAP(edit_mastertrack_xpm, NULL);
      edit_pianorollIcon            = new MPIXMAP(edit_pianoroll_xpm, NULL);
      edit_scoreIcon                = new MPIXMAP(edit_score_xpm, NULL);
      edit_track_addIcon            = new MPIXMAP(edit_track_add_xpm, NULL);
      edit_track_delIcon            = new MPIXMAP(edit_track_del_xpm, NULL);
      mastertrack_graphicIcon       = new MPIXMAP(mastertrack_graphic_xpm, NULL);
      mastertrack_listIcon          = new MPIXMAP(mastertrack_list_xpm, NULL);
      midi_transformIcon            = new MPIXMAP(midi_transform_xpm, NULL);
      midi_transposeIcon            = new MPIXMAP(midi_transpose_xpm, NULL);
      selectIcon                    = new MPIXMAP(select_xpm, NULL);
      selectMultiIcon               = new MPIXMAP(":/images/icons/select_multi.png", NULL);
      select_allIcon                = new MPIXMAP(select_all_xpm, NULL);
      select_all_parts_on_trackIcon = new MPIXMAP(select_all_parts_on_track_xpm, NULL);
      select_deselect_allIcon       = new MPIXMAP(":/images/icons/deselectall.png", NULL);
      select_inside_loopIcon        = new MPIXMAP(select_inside_loop_xpm, NULL);
      select_invert_selectionIcon   = new MPIXMAP(":/images/icons/select_invert_selection.png", NULL);
      select_outside_loopIcon       = new MPIXMAP(select_outside_loop_xpm, NULL);
      pianoIconSet                  = new MICON(edit_pianoroll_xpm, NULL); // ddskrjo

      audio_bounce_to_fileIcon                      = new MPIXMAP(audio_bounce_to_file_xpm, NULL);
      audio_bounce_to_trackIcon                     = new MPIXMAP(audio_bounce_to_track_xpm, NULL);
      audio_restartaudioIcon                        = new MPIXMAP(audio_restartaudio_xpm, NULL);
      automation_clear_dataIcon                     = new MPIXMAP(automation_clear_data_xpm, NULL);
      automation_mixerIcon                          = new MPIXMAP(automation_mixer_xpm, NULL);
      automation_take_snapshotIcon                  = new MPIXMAP(automation_take_snapshot_xpm, NULL);
      edit_midiIcon                                 = new MPIXMAP(edit_midi_xpm, NULL);
      midi_edit_instrumentIcon                      = new MPIXMAP(midi_edit_instrument_xpm, NULL);
      midi_init_instrIcon                           = new MPIXMAP(midi_init_instr_xpm, NULL);
      midi_inputpluginsIcon                         = new MPIXMAP(midi_inputplugins_xpm, NULL);
      midi_inputplugins_midi_input_filterIcon       = new MPIXMAP(midi_inputplugins_midi_input_filter_xpm, NULL);
      midi_inputplugins_midi_input_transformIcon    = new MPIXMAP(midi_inputplugins_midi_input_transform_xpm, NULL);
      midi_inputplugins_random_rhythm_generatorIcon = new MPIXMAP(midi_inputplugins_random_rhythm_generator_xpm, NULL);
      midi_inputplugins_remote_controlIcon          = new MPIXMAP(midi_inputplugins_remote_control_xpm, NULL);
      midi_inputplugins_transposeIcon               = new MPIXMAP(midi_inputplugins_transpose_xpm, NULL);
      midi_local_offIcon                            = new MPIXMAP(midi_local_off_xpm, NULL);
      midi_reset_instrIcon                          = new MPIXMAP(midi_reset_instr_xpm, NULL);
      settings_appearance_settingsIcon              = new MPIXMAP(settings_appearance_settings_xpm, NULL);
      settings_configureshortcutsIcon               = new MPIXMAP(settings_configureshortcuts_xpm, NULL);
      settings_follow_songIcon                      = new MPIXMAP(settings_follow_song_xpm, NULL);
      settings_globalsettingsIcon                   = new MPIXMAP(settings_globalsettings_xpm, NULL);
      settings_metronomeIcon                        = new MPIXMAP(settings_metronome_xpm, NULL);
      settings_midifileexportIcon                   = new MPIXMAP(settings_midifileexport_xpm, NULL);
      settings_midiport_softsynthsIcon              = new MPIXMAP(settings_midiport_softsynths_xpm, NULL);
      settings_midisyncIcon                         = new MPIXMAP(settings_midisync_xpm, NULL);
      view_bigtime_windowIcon                       = new MPIXMAP(view_bigtime_window_xpm, NULL);
      view_cliplistIcon                             = new MPIXMAP(view_cliplist_xpm, NULL);
      view_markerIcon                               = new MPIXMAP(view_marker_xpm, NULL);
      view_mixerIcon                                = new MPIXMAP(view_mixer_xpm, NULL);
      view_transport_windowIcon                     = new MPIXMAP(view_transport_window_xpm, NULL);

      mixerIcon                                     = new MPIXMAP(":/images/icons/showmixer.png", NULL);
      expandIcon                                    = new MPIXMAP(":/images/icons/mixer-expand.png", NULL);
      monoIcon                                      = new MPIXMAP(":/images/icons/mixer-mono.png", NULL);
      stereoIcon                                    = new MPIXMAP(":/images/icons/mixer-stereo.png", NULL);
      automationIcon                                = new MPIXMAP(":/images/icons/automation.png", NULL);
      portIcon                                		= new MPIXMAP(":/images/icons/ports.png", NULL);
      automationDisabledIcon                        = new MPIXMAP(":/images/icons/automation_disabled.png", NULL);
      portDisabledIcon                              = new MPIXMAP(":/images/icons/ports_disabled.png", NULL);
      addTVIcon                              		= new MPIXMAP(":/images/icons/add_tv.png", NULL);
      reminder1_OnIcon                              = new MPIXMAP(":/images/icons/reminder1_new_on.png", NULL);
      reminder1_OffIcon                        		= new MPIXMAP(":/images/icons/reminder1_new_off.png", NULL);
      reminder1_OverIcon                       		= new MPIXMAP(":/images/icons/reminder1_new_over.png", NULL);
      reminder2_OnIcon                              = new MPIXMAP(":/images/icons/reminder2_new_on.png", NULL);
      reminder2_OffIcon                        		= new MPIXMAP(":/images/icons/reminder2_new_off.png", NULL);
      reminder2_OverIcon                       		= new MPIXMAP(":/images/icons/reminder2_new_over.png", NULL);
      reminder3_OnIcon                              = new MPIXMAP(":/images/icons/reminder3_new_on.png", NULL);
      reminder3_OffIcon                        		= new MPIXMAP(":/images/icons/reminder3_new_off.png", NULL);
      reminder3_OverIcon                       		= new MPIXMAP(":/images/icons/reminder3_new_over.png", NULL);
      
	  record_track_OnIcon                           = new MPIXMAP(":/images/icons/record_track_new_on.png", NULL);
      record_track_OffIcon                          = new MPIXMAP(":/images/icons/record_track_new_off.png", NULL);
      record_track_OverIcon                       	= new MPIXMAP(":/images/icons/record_track_new_over.png", NULL);
      
	  mute_track_OnIcon                          	= new MPIXMAP(":/images/icons/mute_track_new_on.png", NULL);
      mute_track_OffIcon                         	= new MPIXMAP(":/images/icons/mute_track_new_off.png", NULL);
      mute_track_OverIcon                        	= new MPIXMAP(":/images/icons/mute_track_new_over.png", NULL);
	  
	  solo_track_OnIcon                          	= new MPIXMAP(":/images/icons/solo_track_new_on.png", NULL);
      solo_track_OffIcon                         	= new MPIXMAP(":/images/icons/solo_track_new_off.png", NULL);
      solo_track_OverIcon                        	= new MPIXMAP(":/images/icons/solo_track_new_over.png", NULL);
	  
	  input_indicator_OnIcon                        = new MPIXMAP(":/images/icons/input_indicator_new_on.png", NULL);
      input_indicator_OffIcon                       = new MPIXMAP(":/images/icons/input_indicator_new_off.png", NULL);
      input_indicator_OverIcon                      = new MPIXMAP(":/images/icons/input_indicator_new_over.png", NULL);
	  
      automation_track_OffIcon                      = new MPIXMAP(":/images/icons/automation_track_new_off.png", NULL);
      automation_track_OverIcon                     = new MPIXMAP(":/images/icons/automation_track_new_over.png", NULL);
	  
      instrument_track_OffIcon                      = new MPIXMAP(":/images/icons/instrument_track_new_off.png", NULL);
      instrument_track_OverIcon                     = new MPIXMAP(":/images/icons/instrument_track_new_over.png", NULL);
      instrument_track_ActiveIcon                   = new MPIXMAP(":/images/icons/input_note.png", NULL);
	  
      instrument_OnIcon                             = new MPIXMAP(":/images/instrument_new_on.png", NULL);
      instrument_OffIcon                            = new MPIXMAP(":/images/instrument_new_off.png", NULL);
      instrument_OverIcon                           = new MPIXMAP(":/images/instrument_new_over.png", NULL);
	  
	  reminder1IconSet3         = new QIcon();
	  reminder1IconSet3->addPixmap(*reminder1_OnIcon, QIcon::Normal, QIcon::On);
      reminder1IconSet3->addPixmap(*reminder1_OffIcon, QIcon::Normal, QIcon::Off);
      reminder1IconSet3->addPixmap(*reminder1_OverIcon, QIcon::Active);
	  
	  reminder2IconSet3         = new QIcon();
	  reminder2IconSet3->addPixmap(*reminder2_OnIcon, QIcon::Normal, QIcon::On);
      reminder2IconSet3->addPixmap(*reminder2_OffIcon, QIcon::Normal, QIcon::Off);
      reminder2IconSet3->addPixmap(*reminder2_OverIcon, QIcon::Active);
	  
	  reminder3IconSet3         = new QIcon();
	  reminder3IconSet3->addPixmap(*reminder3_OnIcon, QIcon::Normal, QIcon::On);
      reminder3IconSet3->addPixmap(*reminder3_OffIcon, QIcon::Normal, QIcon::Off);
      reminder3IconSet3->addPixmap(*reminder3_OverIcon, QIcon::Active);
	  
	  record_trackIconSet3         = new QIcon();
	  record_trackIconSet3->addPixmap(*record_track_OnIcon, QIcon::Normal, QIcon::On);
      record_trackIconSet3->addPixmap(*record_track_OffIcon, QIcon::Normal, QIcon::Off);
      record_trackIconSet3->addPixmap(*record_track_OverIcon, QIcon::Active);
	  
	  mute_trackIconSet3         = new QIcon();
	  mute_trackIconSet3->addPixmap(*mute_track_OnIcon, QIcon::Normal, QIcon::On);
      mute_trackIconSet3->addPixmap(*mute_track_OffIcon, QIcon::Normal, QIcon::Off);
      mute_trackIconSet3->addPixmap(*mute_track_OverIcon, QIcon::Active);
	  
	  solo_trackIconSet3         = new QIcon();
	  solo_trackIconSet3->addPixmap(*solo_track_OnIcon, QIcon::Normal, QIcon::On);
      solo_trackIconSet3->addPixmap(*solo_track_OffIcon, QIcon::Normal, QIcon::Off);
      solo_trackIconSet3->addPixmap(*solo_track_OverIcon, QIcon::Active);
	  
	  input_indicatorIconSet3         = new QIcon();
	  input_indicatorIconSet3->addPixmap(*input_indicator_OnIcon, QIcon::Normal, QIcon::On);
      input_indicatorIconSet3->addPixmap(*input_indicator_OffIcon, QIcon::Normal, QIcon::Off);
      input_indicatorIconSet3->addPixmap(*input_indicator_OverIcon, QIcon::Active);
	  
	  automation_trackIconSet3         = new QIcon();
      automation_trackIconSet3->addPixmap(*automation_track_OffIcon, QIcon::Normal, QIcon::Off);
      automation_trackIconSet3->addPixmap(*automation_track_OverIcon, QIcon::Active);

	  instrument_trackIconSet3         = new QIcon();
      instrument_trackIconSet3->addPixmap(*instrument_track_OffIcon, QIcon::Normal, QIcon::Off);
      instrument_trackIconSet3->addPixmap(*instrument_track_OverIcon, QIcon::Active);

	  instrumentIconSet3         = new QIcon();
      instrumentIconSet3->addPixmap(*instrument_OnIcon, QIcon::Normal, QIcon::On);
      instrumentIconSet3->addPixmap(*instrument_OffIcon, QIcon::Normal, QIcon::Off);
      instrumentIconSet3->addPixmap(*instrument_OverIcon, QIcon::Active);

      oomIcon                                      = new MPIXMAP(oom_icon_xpm, NULL);
      aboutOOMidiImage                             = new MPIXMAP(":/images/oom-about.png", NULL);
      oomLeftSideLogo                              = new MPIXMAP(":/images/oom_leftside_logo.png", NULL);
      globalIcon                                    = new MICON(global_xpm, NULL);//"folder");
      userIcon                                      = new MICON(user_xpm, NULL);//"user-home");
      projectIcon                                   = new MICON(project_xpm, NULL);//"folder-sound");

      sineIcon                                      = new MPIXMAP(sine_xpm, NULL);
      sawIcon                                       = new MPIXMAP(saw_xpm, NULL);
      }

