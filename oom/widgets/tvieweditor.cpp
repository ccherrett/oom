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
	cmbViews->addItems(buildViewList());
	listAllTracks->setModel(new QStringListModel(stracks));
	
	btnOk = buttonBox->button(QDialogButtonBox::Ok);
	btnCancel = buttonBox->button(QDialogButtonBox::Cancel);
	btnApply = buttonBox->button(QDialogButtonBox::Apply);
	btnApply->setEnabled(false);
	btnEdit->setEnabled(false);

	btnAdd = actionBox->button(QDialogButtonBox::Yes);
	btnAdd->setText(tr("Add Track"));

	connect(btnAdd, SIGNAL(clicked(bool)), SLOT(btnAddTrack(bool)));
	
	btnRemove = actionBox->button(QDialogButtonBox::No);
	btnRemove->setText(tr("Remove Track"));
	btnRemove->setFocusPolicy(btnAdd->focusPolicy());
	
	txtName->setReadOnly(true);
	connect(btnRemove, SIGNAL(clicked(bool)), SLOT(btnRemoveTrack(bool)));
	connect(btnNew, SIGNAL(clicked(bool)), SLOT(btnNewClicked(bool)));

	connect(cmbViews, SIGNAL(currentIndexChanged(int)), SLOT(cmbViewSelected(int)));
	connect(cmbType, SIGNAL(currentIndexChanged(int)), SLOT(cmbTypeSelected(int)));
	connect(btnOk, SIGNAL(clicked(bool)), SLOT(btnOkClicked(bool)));
	connect(btnApply, SIGNAL(clicked(bool)), SLOT(btnApplyClicked(bool)));
	connect(btnCancel, SIGNAL(clicked(bool)), SLOT(btnCancelClicked(bool)));
}


QStringList TrackViewEditor::buildViewList()
{
	QStringList tv;
	tv.append(tr("Select Track View"));
	for(ciTrackView ci = song->trackviews()->begin(); ci != song->trackviews()->end(); ++ci)
	{
		tv.append((*ci)->viewName());
	}
	return tv;
}

//----------------------------------------------
// Slots
//----------------------------------------------

void TrackViewEditor::btnNewClicked(bool)
{
	_selected = 0;
	//Reset cmbViews
	cmbViews->blockSignals(true);
	cmbViews->setCurrentIndex(0);
	cmbViews->blockSignals(true);
	//Reset listSelectedTracks
	QStringListModel* model = (QStringListModel*)listSelectedTracks->model();
	if(model)
	{
		model->removeRows(0, model->rowCount());
	}
	//Clear txtName, set value to Untitled
	txtName->setText(tr("Untitled"));
	txtName->setFocus(Qt::OtherFocusReason);
	txtName->setReadOnly(false);
	btnApply->setEnabled(true);
}

void TrackViewEditor::cmbViewSelected(int ind)
{
	if(ind == 0)
	{
		//Clear listSelectedTracks
		QStringListModel* model = (QStringListModel*)listSelectedTracks->model();
		if(model)
		{
			model->removeRows(0, model->rowCount());
		}
		//Clear txtName
		txtName->setText("");
		//Disable btnEdit
		btnEdit->setEnabled(false);
		return;
	}
	btnEdit->setEnabled(false);
	//Perform actions to populate list below based on selected view
	QString sl = cmbViews->itemText(ind);
	btnApply->setEnabled(false);
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

void TrackViewEditor::btnApplyClicked(bool/* state*/)
{
	printf("TrackViewEditor::btnApplyClicked()\n");
}

void TrackViewEditor::btnOkClicked(bool/* state*/)
{
	printf("TrackViewEditor::btnOkClicked()\n");
}

void TrackViewEditor::btnCancelClicked(bool/* state*/)
{
	printf("TrackViewEditor::btnCancelClicked()\n");
}

void TrackViewEditor::btnAddTrack(bool/* state*/)
{
	//Perform actions to add action to right list and remove from left
	printf("Add button clicked\n");
	if(_selected)
		btnApply->setEnabled(true);
	QItemSelectionModel* model = listAllTracks->selectionModel();
	QStringListModel* lst = (QStringListModel*)listSelectedTracks->model(); 
	QStringListModel* lat = (QStringListModel*)listAllTracks->model(); 
	QList<int> del;
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
			{
				QStringList slist  = lst->stringList();
				slist.append(val);
				lst->setStringList(slist);
				printf("Adding Track from row: %d\n", row);
				del.append(row);
			}
			//printf("Found Track %s at index %d with type %d\n", val, row, trk->type());
		}
		//Delete the list in reverse order so the listview maintains the propper state
		if(!del.isEmpty())
		{
			QListIterator<int> it(del); 
			it.toBack();
			while(it.hasPrevious())
			{
				lat->removeRow(it.previous());
			}

		}
	}
}

void TrackViewEditor::btnRemoveTrack(bool/* state*/)
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
