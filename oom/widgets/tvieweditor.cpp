//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================


#include <QMessageBox>
#include <QDialog>
#include <QStringListModel>
#include <QPushButton>
#include <QItemSelectionModel>
#include <QModelIndexList>
#include <QModelIndex>

#include <math.h>
#include <string.h>
#include "tvieweditor.h"
#include "song.h"
#include "globals.h"
#include "config.h"
#include "gconfig.h"
#include "utils.h"
#include "audio.h"
#include "midi.h"
#include "icons.h"
#include "app.h"
#include "popupmenu.h"
#include "track.h"
#include "trackview.h"
#include "synth.h"

TrackViewEditor::TrackViewEditor(QWidget* parent, TrackViewList* vl) : QDialog(parent)
{
	setupUi(this);
	//MIDI=0, DRUM, WAVE, AUDIO_OUTPUT, AUDIO_INPUT, AUDIO_GROUP,AUDIO_AUX
	_trackTypes = (QStringList() << "Audio_Out" << "Audio_In" << "Audio_Aux" << "Audio_Group" << "Midi" << "Soft_Synth" << "Wave"); //new QStringList();
	//Populate trackTypes and pass it to cmbTypes 
	cmbType->addItems(_trackTypes);
	QStringList stracks;
	//Output is the first in the list and will be displayed by default.
	for (ciTrack t = song->outputs()->begin(); t != song->outputs()->end(); ++t)
	{
		stracks << (*t)->name();
	}
	QStringList tv;
	tv.append(tr("Select Track View"));
	for(ciTrackView ci = song->trackviews()->begin(); ci != song->trackviews()->end(); ++ci)
	{
		tv.append((*ci)->viewName());
	}
	cmbViews->addItems(tv);
	listAllTracks->setModel(new QStringListModel(stracks));
	btnAdd = actionBox->button(QDialogButtonBox::Yes);
	btnAdd->setText(tr("Add Track"));
	connect(btnAdd, SIGNAL(clicked(bool)), SLOT(btnAddTrack(bool)));
	btnRemove = actionBox->button(QDialogButtonBox::No);
	btnRemove->setText(tr("Remove Track"));
	btnRemove->setFocusPolicy(btnAdd->focusPolicy());
	connect(btnRemove, SIGNAL(clicked(bool)), SLOT(btnRemoveTrack(bool)));

	connect(cmbViews, SIGNAL(currentIndexChanged(QString&)), SLOT(cmbViewSelected(QString&)));
	connect(cmbType, SIGNAL(currentIndexChanged(int)), SLOT(cmbTypeSelected(int)));
}


//----------------------------------------------
// Slots
//----------------------------------------------

void TrackViewEditor::cmbViewSelected(QString& sl)
{
	//Perform actions to populate list below based on selected view
	TrackView* v = song->findTrackView(sl);
	if(v)
	{
		TrackList* l = v->tracks();
		if(l)
		{
			QStringList sl;
			for(ciTrack ci = l->begin(); ci != l->end(); ++ci)
			{
				sl.append((*ci)->name());
			}
			listSelectedTracks->setModel(new QStringListModel(sl));
//"Audio_Out" "Audio_In" "Audio_Aux" "Audio_Group" "Midi" "Soft_Synth"
			switch(v->type())
			{
				case Track::MIDI:
				case Track::DRUM:
					cmbType->setCurrentIndex(4);
				break;
				case Track::WAVE:
					cmbType->setCurrentIndex(6);
				break;
				case Track::AUDIO_OUTPUT:
					cmbType->setCurrentIndex(0);
				break;
				case Track::AUDIO_INPUT:
					cmbType->setCurrentIndex(1);
				break;
				case Track::AUDIO_GROUP:
					cmbType->setCurrentIndex(3);
				break;
				case Track::AUDIO_AUX:
					cmbType->setCurrentIndex(2);
				break;
				case Track::AUDIO_SOFTSYNTH:
					cmbType->setCurrentIndex(5);
				break;
			}
		}
	}
}

void TrackViewEditor::cmbTypeSelected(int type)
{
	//Perform actions to populate list below based on selected type
	//We need to repopulate and filter the allTrackList
	//"Audio_Out" "Audio_In" "Audio_Aux" "Audio_Group" "Midi" "Soft_Synth"
	QStringList stracks;
	switch (type)
	{/*{{{*/
		case 0:
			for (ciTrack t = song->outputs()->begin(); t != song->outputs()->end(); ++t)
			{
				//This should be checked against track in other views
				if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
		case 1:
			for (ciTrack t = song->inputs()->begin(); t != song->inputs()->end(); ++t)
			{
				//This should be checked against track in other views
				if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 2:
			for (ciTrack t = song->auxs()->begin(); t != song->auxs()->end(); ++t)
			{
				//This should be checked against track in other views
				if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 3:
			for (ciTrack t = song->groups()->begin(); t != song->groups()->end(); ++t)
			{
				//This should be checked against track in other views
				if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 4:
			for (ciTrack t = song->midis()->begin(); t != song->midis()->end(); ++t)
			{
				//This should be checked against track in other views
				if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 5:
			for (ciTrack t = song->syntis()->begin(); t != song->syntis()->end(); ++t)
			{
				//This should be checked against track in other views
				if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 6:
			for (ciTrack t = song->waves()->begin(); t != song->waves()->end(); ++t)
			{
				//This should be checked against track in other views
				if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
	}/*}}}*/
	listAllTracks->setModel(new QStringListModel(stracks));
}

void TrackViewEditor::btnAddTrack(bool state)
{
	//Perform actions to add action to right list and remove from left
	printf("Add button clicked\n");
	QItemSelectionModel* model = listAllTracks->selectionModel();
	if (model->hasSelection())
	{
		QModelIndexList sel = model->selectedRows(0);
		QList<QModelIndex>::const_iterator id;
		for (id = sel.constBegin(); id != sel.constEnd(); ++id)
			//for(QModelIndex* id = sel.begin(); id != sel.end(); ++id)
		{
			//We have to index we will get the row.
			int row = (*id).row();
			/*QStringListModel* m = */QAbstractItemModel* m = listAllTracks->model();
			QVariant v = m->data((*id));
			QString val = v.toString();
			Track* trk = song->findTrack(val);
			if (trk)
				printf("Adding Track from row: %d\n", row);
			//printf("Found Track %s at index %d with type %d\n", val, row, trk->type());
		}
	}
}

void TrackViewEditor::btnRemoveTrack(bool state)
{
	//Perform action to remove track from the selectedTracks list
	printf("Remove button clicked\n");
}

void TrackViewEditor::setSelectedTracks(TrackList* t)
{
	_selected = t;
	//Call methods to update the display
}

void TrackViewEditor::setTypes(QStringList t)
{
	_trackTypes = t;
	//Call methods to update the display
}

void TrackViewEditor::setViews(TrackViewList* l)
{
	_viewList = l;
	//Call methods to update the display
}
