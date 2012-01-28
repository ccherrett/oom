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
#include <QStandardItemModel>
#include <QMap>

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
#include "instrumentdelegate.h"
#include "tablespinner.h"

TrackViewEditor::TrackViewEditor(QWidget* parent, TrackViewList*) : QDialog(parent)
{
	setupUi(this);
	_selected = 0;
//MIDI=0, DRUM, WAVE, AUDIO_OUTPUT, AUDIO_INPUT, AUDIO_BUSS,AUDIO_AUX
	_trackTypes = (QStringList() << "All Types" << "Outputs" << "Inputs" << "Auxs" << "Busses" << "Midi Tracks" << "Soft Synth" << "Audio Tracks"); //new QStringList();
	_editing = false;
	_addmode = false;
	InstrumentDelegate* pdelegate = new InstrumentDelegate(this);
	TableSpinnerDelegate* tdelegate = new TableSpinnerDelegate(this);
	m_model = new QStandardItemModel(0, 3, this);
	m_selmodel = new QItemSelectionModel(m_model);
	optionsTable->setModel(m_model);
	optionsTable->setItemDelegateForColumn(1, tdelegate);
	optionsTable->setItemDelegateForColumn(2, pdelegate);
	updateTableHeader();
	//Populate trackTypes and pass it to cmbTypes 
	cmbType->addItems(_trackTypes);
	QStringList stracks;
	//Output is the first in the list and will be displayed by default.
	for (ciTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
	{
		stracks << (*t)->name();
	}
	buildViewList();
	listAllTracks->setModel(new QStringListModel(stracks));
	
	btnOk = buttonBox->button(QDialogButtonBox::Ok);
	btnCancel = buttonBox->button(QDialogButtonBox::Cancel);
	btnApply = buttonBox->button(QDialogButtonBox::Apply);
	btnApply->setEnabled(false);
	btnCopy->setEnabled(false);

	btnUp->setIcon(*up_arrowIconSet3);
	btnDown->setIcon(*down_arrowIconSet3);
	btnRemove->setIcon(*garbageIconSet3);
	btnAdd->setIcon(*plusIconSet3);
	btnAddVirtual->setIcon(*plusIconSet3);

	connect(btnAdd, SIGNAL(clicked()), SLOT(btnAddTrack()));
	
	txtName->setReadOnly(true);
	connect(btnRemove, SIGNAL(clicked()), SLOT(btnRemoveTrack()));
	connect(btnNew, SIGNAL(clicked(bool)), SLOT(btnNewClicked(bool)));

	connect(cmbViews, SIGNAL(currentIndexChanged(int)), SLOT(cmbViewSelected(int)));
	connect(cmbType, SIGNAL(currentIndexChanged(int)), SLOT(cmbTypeSelected(int)));
	connect(btnOk, SIGNAL(clicked(bool)), SLOT(btnOkClicked(bool)));
	connect(btnApply, SIGNAL(clicked(bool)), SLOT(btnApplyClicked(bool)));
	connect(btnCancel, SIGNAL(clicked(bool)), SLOT(btnCancelClicked(bool)));
	connect(btnDelete, SIGNAL(clicked(bool)), SLOT(btnDeleteClicked(bool)));
	connect(btnCopy, SIGNAL(clicked(bool)), SLOT(btnCopyClicked(bool)));
	connect(txtName, SIGNAL(textEdited(QString)), SLOT(txtNameEdited(QString)));
	connect(btnUp, SIGNAL(clicked(bool)), SLOT(btnUpClicked(bool)));
	connect(btnDown, SIGNAL(clicked(bool)), SLOT(btnDownClicked(bool)));
	connect(chkRecord, SIGNAL(toggled(bool)), SLOT(chkRecordChecked(bool)));
	connect(m_model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(settingsChanged(QStandardItem*)));
	connect(txtComment, SIGNAL(textChanged()), SLOT(txtCommentChanged()));
}


void TrackViewEditor::buildViewList()
{
	cmbViews->addItem(tr("Select Track View"), 0);
	QHash<qint64, TrackView*>::const_iterator iter = song->trackviews()->constBegin();
	while (iter != song->trackviews()->constEnd())
	{
		cmbViews->addItem(iter.value()->viewName(), iter.value()->id());
		++iter;
	}
}

//----------------------------------------------
// Slots
//----------------------------------------------

void TrackViewEditor::txtNameEdited(QString text)
{
	if(_selected)
	{
		_editing = true;
		int cursorPos = txtName->cursorPosition();
		txtName->setText(_selected->getValidName(text));
		txtName->setCursorPosition(cursorPos);
		btnApply->setEnabled(true);
	}
}

void TrackViewEditor::chkRecordChecked(bool)
{
	if(_selected)
	{
		_editing = true;
		btnApply->setEnabled(true);
	}
}

void TrackViewEditor::settingsChanged(QStandardItem*)
{
	if(_selected)
	{
		_editing = true;
		btnApply->setEnabled(true);
	}
}

void TrackViewEditor::txtCommentChanged()
{
	if(_selected)
	{
		_editing = true;
		btnApply->setEnabled(true);
	}
}

void TrackViewEditor::btnNewClicked(bool)/*{{{*/
{
	reset();
	_selected = song->addNewTrackView();
	if(_selected)
	{
		_addmode = true;
		_editing = true;
		btnDelete->setEnabled(false);
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
		m_model->clear();
		//Clear txtName
		txtName->setText("");
		//Disable btnCopy
		btnCopy->setEnabled(false);
		btnDelete->setEnabled(false);
		chkRecord->setChecked(false);
		return;
	}
	m_model->clear();
	btnCopy->setEnabled(true);
	btnDelete->setEnabled(true);
	//Perform actions to populate list below based on selected view
	QString sl = cmbViews->itemText(ind);
	qint64 tvid = cmbViews->itemData(ind).toLongLong();
	txtName->setText(sl);
	TrackView* v = song->findTrackViewById(tvid);
	_editing = true;
	if(v)
	{
		_selected = v;
		txtComment->blockSignals(true);
		txtComment->setText(v->comment());
		txtComment->blockSignals(false);
		txtComment->moveCursor(QTextCursor::End);
		bool hasSettings = !v->trackSettings()->isEmpty();
		for(int i = 0; i < v->tracks()->size(); ++i)
		{	
			qint64 tid = v->tracks()->at(i);
			Track* t = song->findTrackById(tid);
			if(t)
			{
				QString trackname = t->name();
				QStandardItem* tname = new QStandardItem(trackname);
				tname->setData(t->id());
				tname->setEditable(false);
				QString tp("0");
				QString pname(t->isMidiTrack() ? tr("Select Patch") : "-");
				int prog = 0;
				//Check if there are settings
				if(hasSettings)
				{
					if(v->hasSettings(t->id()))
					{
						TrackSettings *tset = v->getTrackSettings(t->id());
						if(tset)
						{
							tp = QString::number(tset->transpose);
							pname = tset->pname;
							prog = tset->program;
						}
					}
				}

				QStandardItem* transp = new QStandardItem(tp);
				transp->setEditable(false);
				if(t->isMidiTrack())
					transp->setEditable(true);
				QStandardItem* patch = new QStandardItem(pname);
				patch->setData(prog, ProgramRole);
				patch->setEditable(false);
				if(t->isMidiTrack())
					patch->setEditable(true);
				QList<QStandardItem*> rowData;
				rowData.append(tname);
				rowData.append(transp);
				rowData.append(patch);
				m_model->appendRow(rowData);
			}
		}
		chkRecord->setChecked(v->record());
	}
	txtName->setReadOnly(false);
	btnApply->setEnabled(false);
	updateTableHeader();
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
			for (ciTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
			{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 1:
			for (ciTrack t = song->outputs()->begin(); t != song->outputs()->end(); ++t)
			{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 2:
			for (ciTrack t = song->inputs()->begin(); t != song->inputs()->end(); ++t)
			{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 3:
			for (ciTrack t = song->auxs()->begin(); t != song->auxs()->end(); ++t)
			{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 4:
			for (ciTrack t = song->groups()->begin(); t != song->groups()->end(); ++t)
			{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 5:
			for (ciTrack t = song->midis()->begin(); t != song->midis()->end(); ++t)
			{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
					stracks << (*t)->name();
			}
			break;
		case 6:
            //for (ciTrack t = song->syntis()->begin(); t != song->syntis()->end(); ++t)
            //{
				//This should be checked against track in other views
				//if(song->findTrackView((*t)) == 0)
            //		stracks << (*t)->name();
            //}
			break;
		case 7:
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

void TrackViewEditor::btnApplyClicked(bool/* state*/)/*{{{*/
{
	//printf("TrackViewEditor::btnApplyClicked()\n");
	if(_editing && _selected)
	{
		_selected->setViewName(txtName->text());
		//printf("after setviewname\n");
		_selected->tracks()->clear();
		_selected->trackSettings()->clear();
		//Process all the tracks in the listSelectedTracks and add them to the TrackList
		for(int row = 0; row < m_model->rowCount(); ++row)
		{
			QStandardItem* titem = m_model->item(row, 0);
			if(titem)
			{
				Track *t = song->findTrackById(titem->data().toLongLong());
				if(t)
				{
					_selected->addTrack(t->id());
					if(t->isMidiTrack())
					{
						QStandardItem* trans = m_model->item(row, 1);
						int transpose = trans->data(Qt::DisplayRole).toInt();
						QStandardItem* patch = m_model->item(row, 2);
						QString pname = patch->text();
						int prog = patch->data(ProgramRole).toInt();
						//printf("Track name: %s - Transpose: %d - Program: %d - Patch Name: %s\n", 
						//	titem->text().toUtf8().constData(), transpose, prog, pname.toUtf8().constData());
						if((transpose != 0) || prog >= 0)
						{
							//printf("Added TrackSettings for Track: %s\n", titem->text().toUtf8().constData());
							TrackSettings *ts = new TrackSettings;
							ts->pname = pname;
							ts->program = prog;
							ts->transpose = transpose;
							ts->tid = t->id();
							_selected->addTrackSetting(ts->tid, ts);
						}
					}
				}
			}
		}
		_selected->setRecord(chkRecord->isChecked());
		_selected->setComment(txtComment->toPlainText());
		song->dirty = true;
		song->updateTrackViews();
		if(_addmode)
			song->update(SC_VIEW_ADDED);
		else
			song->update(SC_VIEW_CHANGED);
		btnApply->setEnabled(false);
		reset();
	}
}/*}}}*/

void TrackViewEditor::btnOkClicked(bool state)
{
	//printf("TrackViewEditor::btnOkClicked()\n");
	//Apply is the same as ok without the close so just call the other method
	if(_editing)
		btnApplyClicked(state);
	//Do other close cleanup;
}

void TrackViewEditor::reset()/*{{{*/
{
	//Reset cmbViews
	cmbViews->blockSignals(true);
	QAbstractItemModel *mod = cmbViews->model();
	if(mod)
		mod->removeRows(0, mod->rowCount());
	m_model->clear();
	buildViewList();
	cmbViews->blockSignals(false);
	txtComment->blockSignals(true);
	txtComment->setText("");
	txtComment->blockSignals(false);
	cmbViews->setCurrentIndex(0);
	btnApply->setEnabled(false);
	btnDelete->setEnabled(false);
	btnCopy->setEnabled(false);
	txtName->setReadOnly(true);
	_editing = false;
	_addmode = false;
	_selected = 0;
	chkRecord->setChecked(false);
	txtName->setText("");
	updateTableHeader();
}/*}}}*/

void TrackViewEditor::btnCancelClicked(bool/* state*/)/*{{{*/
{
	//printf("TrackViewEditor::btnCancelClicked()\n");
	if(_addmode)
	{//remove the trackview that was added to the song
		TrackView* v = song->findTrackViewById(_selected->id());
		song->removeTrackView(_selected->id());
		delete v;
	}
	_addmode = false;
	_selected = 0;
	song->update(SC_VIEW_CHANGED);
}/*}}}*/

void TrackViewEditor::btnCopyClicked(bool)/*{{{*/
{
	if(_selected)
	{
		TrackView* tv = song->addNewTrackView();
		if(tv)
		{
			for(int i = 0; i < _selected->tracks()->size(); ++i)
			{
				tv->addTrack(_selected->tracks()->at(i));
			}

			QMap<qint64, TrackSettings*>::ConstIterator ts;
			for(ts = _selected->trackSettings()->begin(); ts != _selected->trackSettings()->end(); ++ts)
			{
				//if((*ts).valid)
					tv->addTrackSetting((*ts)->tid, (*ts));
			}
			tv->setComment(_selected->comment());
			tv->setViewName(tv->getValidName(_selected->viewName()));
			tv->setRecord(_selected->record());
			cmbViews->addItem(tv->viewName());
			cmbViews->setCurrentIndex(cmbViews->findText(tv->viewName()));
			_selected = tv;
			btnApply->setEnabled(true);
			_addmode = true;
			_editing = true;
		}
	}
}/*}}}*/

void TrackViewEditor::btnDeleteClicked(bool)/*{{{*/
{
	if(_selected)
	{
		song->removeTrackView(_selected->id());
		song->dirty = true;
		song->updateTrackViews();
		song->update(SC_VIEW_DELETED);
		reset();
	}
}/*}}}*/


void TrackViewEditor::btnAddTrack()/*{{{*/
{
	//Perform actions to add action to right list and remove from left
	//printf("Add button clicked\n");
	if(_selected)
	{
		btnApply->setEnabled(true);
		_editing = true;
		QItemSelectionModel* model = listAllTracks->selectionModel();
		QAbstractItemModel* lat = listAllTracks->model(); 
		if (model->hasSelection())
		{
			QModelIndexList sel = model->selectedRows(0);
			QList<QModelIndex>::const_iterator id;
			for (id = sel.constBegin(); id != sel.constEnd(); ++id)
			{
				//We have to index we will get the row.
				QVariant v = lat->data((*id));
				QString val = v.toString();
				Track* trk = song->findTrack(val);
				if (trk)
				{
				//	printf("Adding Track from row: %d\n", row);
					QList<QStandardItem *> items = m_model->findItems(val);
					if(items.isEmpty())
					{
						QStandardItem* tname = new QStandardItem(trk->name());
						tname->setData(trk->id());
						tname->setEditable(false);
						QStandardItem* transp = new QStandardItem(QString::number(0));
						transp->setEditable(false);
						if(trk->isMidiTrack())
							transp->setEditable(true);
						QStandardItem* patch = new QStandardItem(trk->isMidiTrack() ? tr("Select Patch") : "-");
						patch->setData(0, ProgramRole);
						patch->setEditable(false);
						if(trk->isMidiTrack())
							patch->setEditable(true);
						QList<QStandardItem*> rowData;
						rowData.append(tname);
						rowData.append(transp);
						rowData.append(patch);
						m_model->appendRow(rowData);
						optionsTable->selectRow((m_model->rowCount()-1));
					}
				}
				//printf("Found Track %s at index %d with type %d\n", val, row, trk->type());
			}
		}
	}
	updateTableHeader();
}/*}}}*/

void TrackViewEditor::btnRemoveTrack()/*{{{*/
{
	//Perform action to remove track from the selectedTracks list
	//printf("Remove button clicked\n");
	if(_selected)
	{
		btnApply->setEnabled(true);
		_editing = true;
		QList<int> selrows = getSelectedRows();
		QList<int> del;
		if (!selrows.isEmpty())
		{
			for(int i = 0; i < selrows.size(); ++i)
			{
				//We have to index we will get the row.
				int row = selrows[i];
				QStandardItem* item = m_model->item(row);
				QString val = item->text();
				//Track* trk = song->findTrack(val);
				//if (trk)
				//{
					del.append(row);
				//}
				//printf("Found Track %s at index %d with type %d\n", val, row, trk->type());
			}
			//Delete the list in reverse order so the listview maintains the propper state
			if(!del.isEmpty())
			{
				QListIterator<int> it(del); 
				it.toBack();
				while(it.hasPrevious())
				{
					m_model->takeRow(it.previous());
				}

			}
		}
	}
	updateTableHeader();
}/*}}}*/

void TrackViewEditor::btnAddVirtualTrack()
{
}

void TrackViewEditor::btnUpClicked(bool)/*{{{*/
{
	//Perform action to remove track from the selectedTracks list
	//printf("Remove up clicked\n");
	if(_selected)
	{
		btnApply->setEnabled(true);
		_editing = true;
		QList<int> rows = getSelectedRows();
		if (!rows.isEmpty())
		{
			int id = rows.at(0);
			if ((id - 1) < 0)
				return;
			int row = (id - 1);
			QList<QStandardItem*> item = m_model->takeRow(id);
			m_model->insertRow(row, item);
			optionsTable->selectRow(row);
		}
	}
}/*}}}*/

void TrackViewEditor::btnDownClicked(bool)/*{{{*/
{
	//Perform action to remove track from the selectedTracks list
	//printf("Remove up clicked\n");
	if(_selected)
	{
		btnApply->setEnabled(true);
		_editing = true;
		QList<int> rows = getSelectedRows();
		if (!rows.isEmpty())
		{
			int id = rows.at(0);
			if ((id + 1) >= m_model->rowCount())
				return;
			int row = (id + 1);
			QList<QStandardItem*> item = m_model->takeRow(id);
			m_model->insertRow(row, item);
			optionsTable->selectRow(row);
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

QList<int> TrackViewEditor::getSelectedRows()/*{{{*/
{
	QList<int> rv;
	QItemSelectionModel* smodel = optionsTable->selectionModel();
	if (smodel->hasSelection())
	{
		QModelIndexList indexes = smodel->selectedRows();
		QList<QModelIndex>::const_iterator id;
		for (id = indexes.constBegin(); id != indexes.constEnd(); ++id)
		{
			int row = (*id).row();
			rv.append(row);
		}
	}
	return rv;
}/*}}}*/

void TrackViewEditor::updateTableHeader()/*{{{*/
{
	QStandardItem* tname = new QStandardItem(tr("Track"));
	QStandardItem* transp = new QStandardItem(tr("Transpose"));
	QStandardItem* hpatch = new QStandardItem(tr("Patch"));
	m_model->setHorizontalHeaderItem(0, tname);
	m_model->setHorizontalHeaderItem(1, transp);
	m_model->setHorizontalHeaderItem(2, hpatch);
	optionsTable->setColumnWidth(0, 150);
	optionsTable->setColumnWidth(1, 80);
	//optionsTable->setColumnWidth(2, 120);
	optionsTable->horizontalHeader()->setStretchLastSection(true);
}/*}}}*/
