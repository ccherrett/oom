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
	_selected = 0;
//MIDI=0, DRUM, WAVE, AUDIO_OUTPUT, AUDIO_INPUT, AUDIO_GROUP,AUDIO_AUX
	_trackTypes = (QStringList() << "Audio_Out" << "Audio_In" << "Audio_Aux" << "Audio_Group" << "Midi" << "Soft_Synth" << "Wave"); //new QStringList();
	_editing = false;
	_addmode = false;
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
	QStringList sel;
	listSelectedTracks->setModel(new QStringListModel(sel));
	
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

void TrackViewEditor::btnNewClicked(bool)/*{{{*/
{
	int t = 0;
        //MIDI = 0, DRUM, WAVE, AUDIO_OUTPUT, AUDIO_INPUT, AUDIO_GROUP, AUDIO_AUX, AUDIO_SOFTSYNTH
	int type = cmbType->currentIndex();
	switch (type)/*{{{*/
	{
		case 0:
			t = (int)Track::AUDIO_OUTPUT;
			break;
		case 1:
			t = (int)Track::AUDIO_INPUT;
			break;
		case 2:
			t = (int)Track::AUDIO_AUX;
			break;
		case 3:
			t = (int)Track::AUDIO_GROUP;
			break;
		case 4:
			t = Track::MIDI;
			break;
		case 5:
			t = Track::AUDIO_SOFTSYNTH;
			break;
		case 6:
			t = Track::WAVE;
			break;
	}/*}}}*/
	_selected = song->addNewTrackView(t);
	if(_selected)
	{
		_addmode = true;
		_editing = true;
		//Reset cmbViews
		cmbViews->blockSignals(true);
		cmbViews->setCurrentIndex(0);
		//cmbViews->addItem(_selected->viewName());
		//cmbViews->setCurrentIndex(cmbViews->find(_selected->viewName()));
		cmbViews->blockSignals(true);
		//Reset listSelectedTracks
		QStringListModel* model = (QStringListModel*)listSelectedTracks->model();
		if(model)
		{
			model->removeRows(0, model->rowCount());
		}
		//Clear txtName, set value to Untitled
		txtName->setText(_selected->viewName());
		txtName->setFocus(Qt::OtherFocusReason);
		txtName->setReadOnly(false);
		btnApply->setEnabled(true);
	}
}/*}}}*/

void TrackViewEditor::cmbViewSelected(int ind)/*{{{*/
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
	btnEdit->setEnabled(true);
	//Perform actions to populate list below based on selected view
	QString sl = cmbViews->itemText(ind);
	txtName->setText(sl);
	btnApply->setEnabled(false);
	TrackView* v = song->findTrackView(sl);
	_editing = true;
	if(v)
	{
		_selected = v;
		TrackList* l = v->tracks();
		if(l)
		{
			QStringList sl;
			for(ciTrack ci = l->begin(); ci != l->end(); ++ci)
			{
				sl.append((*ci)->name());
			}
			listSelectedTracks->setModel(new QStringListModel(sl));
		}
	}
	_editing = false;
}/*}}}*/

void TrackViewEditor::cmbTypeSelected(int type)/*{{{*/
{
	//Perform actions to populate list below based on selected type
	//We need to repopulate and filter the allTrackList
	//"Audio_Out" "Audio_In" "Audio_Aux" "Audio_Group" "Midi" "Soft_Synth"
	//if(!_editing)
	//	cmbViews->setCurrentIndex(0);
	QStringList stracks;
	switch (type)
	{/*{{{*/
		case 0:
			for (ciTrack t = song->outputs()->begin(); t != song->outputs()->end(); ++t)
			{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
		case 1:
			for (ciTrack t = song->inputs()->begin(); t != song->inputs()->end(); ++t)
			{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 2:
			for (ciTrack t = song->auxs()->begin(); t != song->auxs()->end(); ++t)
			{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 3:
			for (ciTrack t = song->groups()->begin(); t != song->groups()->end(); ++t)
			{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 4:
			for (ciTrack t = song->midis()->begin(); t != song->midis()->end(); ++t)
			{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 5:
			for (ciTrack t = song->syntis()->begin(); t != song->syntis()->end(); ++t)
			{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 6:
			for (ciTrack t = song->waves()->begin(); t != song->waves()->end(); ++t)
			{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
	}/*}}}*/
	listAllTracks->setModel(new QStringListModel(stracks));
}/*}}}*/

void TrackViewEditor::btnApplyClicked(bool/* state*/)
{
	printf("TrackViewEditor::btnApplyClicked()\n");
	if(_editing && _selected)
	{
		_selected->setViewName(txtName->text());
		TrackList *tl = _selected->tracks();

		for(iTrack it = tl->begin(); it != tl->end(); ++it)
			tl->erase(it);
		//Process all the tracks in the listSelectedTracks and add them to the TrackList
		QStringListModel* model = (QStringListModel*)listSelectedTracks->model(); 
		if(model)
		{
			QStringList list = model->stringList();
			for(int i = 0; i < list.size(); ++i)
			{
				Track *t = song->findTrack(list.at(i));
				if(t)
				{
					tl->push_back(t);
				}
			}
		}
		song->dirty = true;
		song->updateTrackViews1();
		cmbViews->blockSignals(true);
		if(_addmode)
			cmbViews->addItem(_selected->viewName());
		cmbViews->blockSignals(false);
		cmbViews->setCurrentIndex(0);
		btnApply->setEnabled(false);
		_editing = false;
		_addmode = false;
	}
}

void TrackViewEditor::btnOkClicked(bool state)
{
	printf("TrackViewEditor::btnOkClicked()\n");
	//Apply is the same as ok without the close so just call the other method
	if(_editing)
		btnApplyClicked(state);
	//Do other close cleanup;
}

void TrackViewEditor::btnCancelClicked(bool/* state*/)
{
	printf("TrackViewEditor::btnCancelClicked()\n");
	if(_addmode)
	{//remove the trackview that was added to the song
		TrackView* v = song->trackviews()->back();
		song->trackviews()->erase(v);
		delete v;
	}
	_addmode = false;
	_selected = 0;
	song->update();
}

void TrackViewEditor::btnAddTrack(bool/* state*/)/*{{{*/
{
	//Perform actions to add action to right list and remove from left
	printf("Add button clicked\n");
	if(_selected)
	{
		btnApply->setEnabled(true);
		QItemSelectionModel* model = listAllTracks->selectionModel();
		QStringListModel* lst = (QStringListModel*)listSelectedTracks->model(); 
		QAbstractItemModel* lat = listAllTracks->model(); 
		//QList<int> del;
		if (model->hasSelection())
		{
			QModelIndexList sel = model->selectedRows(0);
			QList<QModelIndex>::const_iterator id;
			for (id = sel.constBegin(); id != sel.constEnd(); ++id)
			{
				//We have to index we will get the row.
				//int row = (*id).row();
				QVariant v = lat->data((*id));
				QString val = v.toString();
				Track* trk = song->findTrack(val);
				if (trk && lst)
				{
				//	printf("Adding Track from row: %d\n", row);
					QStringList slist  = lst->stringList();
					if(slist.indexOf(val) == -1)
					{
						slist.append(val);
						lst->setStringList(slist);
					}
					//del.append(row);
				}
				//printf("Found Track %s at index %d with type %d\n", val, row, trk->type());
			}
			//Delete the list in reverse order so the listview maintains the propper state
			/*if(!del.isEmpty())
			{
				QListIterator<int> it(del); 
				it.toBack();
				while(it.hasPrevious())
				{
					lat->removeRow(it.previous());
				}
	
			}*/
		}
	}
}/*}}}*/

void TrackViewEditor::btnRemoveTrack(bool/* state*/)/*{{{*/
{
	//Perform action to remove track from the selectedTracks list
	printf("Remove button clicked\n");
	if(_selected)
	{
		btnApply->setEnabled(true);
		QItemSelectionModel* model = listSelectedTracks->selectionModel();
		QAbstractItemModel* lat = listSelectedTracks->model(); 
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
				///*QStringListModel* m = */QAbstractItemModel* m = listAllTracks->model();
				QVariant v = lat->data((*id));
				QString val = v.toString();
				Track* trk = song->findTrack(val);
				if (trk)
				{
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
}/*}}}*/

void TrackViewEditor::setSelected(TrackView* t)
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
