//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#include <QtGui>
#include <QStringList>
#include <QList>
#include "song.h"
#include "globals.h"
#include "config.h"
#include "gconfig.h"
#include "midiport.h"
#include "minstrument.h"
#include "mididev.h"
#include "utils.h"
#include "audio.h"
#include "midi.h"
#include "midictrl.h"
#include "midiassign.h"
#include "tablespinner.h"
#include "midiportdelegate.h"
#include "ccdelegate.h"
#include "ccinfo.h"
#include "midimonitor.h"

MidiAssignDialog::MidiAssignDialog(QWidget* parent):QDialog(parent)
{
	setupUi(this);
	m_lasttype = 0;
	m_selected = 0;

	m_midicontrols = (QStringList() << "Volume" << "Pan" << "Chor" << "Rev" << "Var" << "Rec" << "Mute" << "Solo");
	m_allowed << CTRL_VOLUME << CTRL_PANPOT << CTRL_REVERB_SEND << CTRL_CHORUS_SEND << CTRL_VARIATION_SEND << CTRL_RECORD << CTRL_MUTE << CTRL_SOLO ;
	
	m_model = new QStandardItemModel(0, 4, this);
	tableView->setModel(m_model);

	m_ccmodel = new QStandardItemModel(0, 2, this);
	m_ccEdit->setModel(m_ccmodel);
	m_selmodel = new QItemSelectionModel(m_model);
	tableView->setSelectionModel(m_selmodel);

	_trackTypes = (QStringList() << "All Types" << "Outputs" << "Inputs" << "Auxs" << "Busses" << "Midi Tracks" << "Soft Synth" << "Audio Tracks");
	cmbType->addItems(_trackTypes);
	connect(cmbType, SIGNAL(currentIndexChanged(int)), SLOT(cmbTypeSelected(int)));
	cmbType->setCurrentIndex(0);

	//TableSpinnerDelegate* tdelegate = new TableSpinnerDelegate(this, -1);
	TableSpinnerDelegate* chandelegate = new TableSpinnerDelegate(this, 0, 15);
	MidiPortDelegate* mpdelegate = new MidiPortDelegate(this);
	CCInfoDelegate *infodelegate = new CCInfoDelegate(this);
	tableView->setItemDelegateForColumn(2, mpdelegate);
	tableView->setItemDelegateForColumn(3, chandelegate);
	//for(int i = 4; i < 12; ++i)
	//	tableView->setItemDelegateForColumn(i, tdelegate);
	
	m_ccEdit->setItemDelegateForColumn(1, infodelegate);
	m_cmbControl->addItem(tr("Track Record"), CTRL_RECORD);
	m_cmbControl->addItem(tr("Track Mute"), CTRL_MUTE);
	m_cmbControl->addItem(tr("Track Solo"), CTRL_SOLO);
	for(int i = 0; i < 128; ++i)
	{
		QString ctl(QString::number(i)+": ");
		m_cmbControl->addItem(ctl.append(midiCtrlName(i)), i);
	}

	connect(btnClose, SIGNAL(clicked(bool)), SLOT(btnCloseClicked()));
	connect(m_model, SIGNAL(itemChanged(QStandardItem*)), SLOT(itemChanged(QStandardItem*)));
	//connect(m_ccmodel, SIGNAL(itemChanged(QStandardItem*)), SLOT(controllerChanged(QStandardItem*)));
	connect(m_selmodel, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), SLOT(itemSelected(const QItemSelection&, const QItemSelection&)));
	connect(m_btnAdd, SIGNAL(clicked()), SLOT(btnAddController()));
	connect(m_btnDelete, SIGNAL(clicked()), SLOT(btnDeleteController()));
	cmbTypeSelected(m_lasttype);
}

void MidiAssignDialog::btnCloseClicked()
{
	close();
}

void MidiAssignDialog::itemChanged(QStandardItem* item)/*{{{*/
{
	//printf("itemChanged\n");
	if(item)
	{
		int row = item->row();
		int col = item->column();
		QStandardItem* trk = m_model->item(row, 1);
		Track* track = song->findTrack(trk->text());
		if(track)
		{
			MidiAssignData* data = track->midiAssign();
			switch(col)
			{
				case 0: //Enabled
					data->enabled = (item->checkState() == Qt::Checked);
				break;
				case 1: //Track not editable
				break;
				case 2: //Port
				{
					int p = data->port;
					data->port = item->data(MidiPortRole).toInt();
					QHashIterator<int, CCInfo*> iter(data->midimap);
					while(iter.hasNext())
					{
						iter.next();
						CCInfo* info = iter.value();
						info->setPort(data->port);
					}
					midiMonitor->msgModifyTrackPort(track, p);
				}
				break;
				case 3: //Channel
					data->channel = item->data(Qt::EditRole).toInt();
				break;
				/*case 4: //volume
				case 5: //Pan
				case 6: //Chorus
				case 7: //Reverb
				case 8: //Var Send
				case 9: //Rec
				case 10: //mute
				case 11: //Solo
				default:
				{
					int ctl = item->data(MidiControlRole).toInt();
					int cc = data->midimap[ctl];
					data->midimap[ctl] = item->data(Qt::EditRole).toInt();
					midiMonitor->msgModifyTrackController(track, ctl, cc);
				}
				break;*/
			}
		}
	}
}/*}}}*/

void MidiAssignDialog::itemSelected(const QItemSelection& isel, const QItemSelection&) //Update the ccEdit table/*{{{*/
{
	printf("Selection changed\n");
	m_ccmodel->clear();
	QModelIndexList list = isel.indexes();
	if(list.size() > 0)
	{
		QModelIndex index = list.at(0);
		int row = index.row();
		QStandardItem* item = m_model->item(row, 1);
		if(item)
		{
			Track* track = song->findTrack(item->text());
			if(track)
			{
				m_trackname->setText(track->name());
				m_selected = track;
				MidiAssignData* data = track->midiAssign();
				if(data && !data->midimap.isEmpty())
				{
					QHashIterator<int, CCInfo*> iter(data->midimap);
					while(iter.hasNext())
					{
						iter.next();
						CCInfo* info = iter.value();
						QList<QStandardItem*> rowData;/*{{{*/
						QStandardItem* chk = new QStandardItem(data->enabled);
						chk->setCheckable(true);
						chk->setEditable(false);
						rowData.append(chk);
						QStandardItem* control = new QStandardItem(track->name());
						control->setEditable(true);
						control->setData(track->name(), TrackRole);
						control->setData(info->port(), PortRole);
						control->setData(info->channel(), ChannelRole);
						control->setData(info->controller(), ControlRole);
						control->setData(info->assignedControl(), CCRole);
						QString str;
						if(info->controller() == CTRL_RECORD)
							str.append(tr("Track Record"));
						else if(info->controller() == CTRL_MUTE)
							str.append(tr("Track Mute"));
						else if(info->controller() == CTRL_SOLO)
							str.append(tr("Track Solo"));
						else
							str.append(midiCtrlName(info->controller()));
						str.append(" Assigned To: ").append(QString::number(info->assignedControl())).append(" Chan : ");
						str.append(QString::number(info->channel()));
						control->setData(str, Qt::DisplayRole);
						rowData.append(control);
						m_ccmodel->appendRow(rowData);/*}}}*/
						m_ccEdit->setRowHeight(m_ccmodel->rowCount()-1, 50);
					}
				}
			}
		}
	}
	updateCCTableHeader();
}/*}}}*/

//Deals with the m_ccEdit table on a per track basis
void MidiAssignDialog::controllerChanged(QStandardItem*)/*{{{*/
{
}/*}}}*/

void MidiAssignDialog::btnAddController()/*{{{*/
{
	if(!m_selected)
		return;
	int ctrl = m_cmbControl->itemData(m_cmbControl->currentIndex()).toInt();
	MidiAssignData *data = m_selected->midiAssign();
	if(data)
	{
		bool allowed = true;
		if(!m_selected->isMidiTrack())
		{
			allowed = false;
			switch(ctrl)//{{{
			{
				case CTRL_VOLUME:
				case CTRL_PANPOT:
				case CTRL_MUTE:
				case CTRL_SOLO:
					allowed = true;
				break;
				case CTRL_RECORD:
					if(m_selected->type() != Track::AUDIO_INPUT)
					{
						allowed = true;
					}
				break;
			}//}}}
		}
		if(!allowed)
			return;
		if(data->midimap.isEmpty() || !data->midimap.contains(ctrl))
		{
			CCInfo* info = new CCInfo(m_selected, data->port, data->channel, ctrl, -1);
			data->midimap.insert(ctrl, info);
			QList<QStandardItem*> rowData;
			QStandardItem* chk = new QStandardItem(data->enabled);
			chk->setCheckable(true);
			chk->setEditable(false);
			rowData.append(chk);
			QStandardItem* control = new QStandardItem(m_selected->name());
			control->setEditable(true);
			control->setData(m_selected->name(), TrackRole);
			control->setData(info->port(), PortRole);
			control->setData(info->channel(), ChannelRole);
			control->setData(info->controller(), ControlRole);
			control->setData(info->assignedControl(), CCRole);
			QString str;
			if(info->controller() == CTRL_RECORD)
				str.append(tr("Track Record"));
			else if(info->controller() == CTRL_MUTE)
				str.append(tr("Track Mute"));
			else if(info->controller() == CTRL_SOLO)
				str.append(tr("Track Solo"));
			else
				str.append(midiCtrlName(info->controller()));
			str.append(" Assigned To: ").append(QString::number(info->assignedControl())).append(" Chan : ");
			str.append(QString::number(info->channel()));
			control->setData(str, Qt::DisplayRole);
			rowData.append(control);
			m_ccmodel->appendRow(rowData);
			m_ccEdit->setRowHeight(m_ccmodel->rowCount()-1, 50);
			//TODO: dirty the song here to force save
		}
	}
	updateCCTableHeader();
}/*}}}*/

void MidiAssignDialog::btnDeleteController()/*{{{*/
{
	if(!m_selected)
		return;
	MidiAssignData* data = m_selected->midiAssign();
	for(int i = 0; i < m_ccmodel->rowCount(); ++i)
	{
		QStandardItem* item = m_ccmodel->item(i, 0);
		if(item->checkState() == Qt::Checked)
		{
			QStandardItem* ctl = m_ccmodel->item(i, 1);
			int control = ctl->data(ControlRole).toInt();
			if(!data->midimap.isEmpty() && data->midimap.contains(control))
			{
			printf("Delete clicked\n");
				CCInfo* info = data->midimap.value(control);
				midiMonitor->msgDeleteTrackController(info);
				data->midimap.remove(control);
				m_ccmodel->takeRow(i);
				//TODO: dirty the song here to force save
			}
		}
	}
	updateCCTableHeader();
}/*}}}*/

void MidiAssignDialog::cmbTypeSelected(int type)/*{{{*/
{
	//Perform actions to populate list below based on selected type
	//We need to repopulate and filter the allTrackList
	//"Audio_Out" "Audio_In" "Audio_Aux" "Audio_Group" "Midi" "Soft_Synth"
	m_lasttype = type;
	QString defaultname;
	defaultname.sprintf("%d:%s", 1, midiPorts[0].portname().toLatin1().constData());
	m_model->clear();
	m_ccmodel->clear();

	for (ciTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
	{
		if (type == 1 && (*t)->type() != Track::AUDIO_OUTPUT)
			continue;
		else if(type == 2 && (*t)->type() != Track::AUDIO_INPUT)
			continue;
		else if(type == 3 && (*t)->type() != Track::AUDIO_AUX)
			continue;
		else if(type == 4 && (*t)->type() != Track::AUDIO_BUSS)
			continue;
		else if(type == 5 && !(*t)->isMidiTrack())
			continue;
		else if(type == 6 && (*t)->type() != Track::AUDIO_SOFTSYNTH)
			continue;
		else if(type == 7 && (*t)->type() != Track::WAVE)
			continue;
		MidiAssignData* data = (*t)->midiAssign();
		QList<QStandardItem*> rowData;
		QStandardItem* enable = new QStandardItem(data->enabled);
		enable->setCheckable(true);
		enable->setCheckState(data->enabled ? Qt::Checked : Qt::Unchecked);
		enable->setEditable(false);
		rowData.append(enable);
		QStandardItem* trk = new QStandardItem((*t)->name());
		trk->setEditable(false);
		rowData.append(trk);
		//printf("MidiPort from Assign %d\n", data->port);
		QString pname;
		if(data->port >= 0)
			pname.sprintf("%d:%s", data->port + 1, midiPorts[data->port].portname().toLatin1().constData());
		else
			pname = defaultname;
		QStandardItem* port = new QStandardItem(pname);
		port->setData(data->port, MidiPortRole);
		port->setEditable(true);
		rowData.append(port);
		QStandardItem* chan = new QStandardItem(QString::number(data->channel));
		chan->setData(data->channel, Qt::EditRole);
		chan->setEditable(true);
		rowData.append(chan);
		m_model->appendRow(rowData);
	}
	updateTableHeader();
	updateCCTableHeader();
}/*}}}*/

void MidiAssignDialog::updateTableHeader()/*{{{*/
{
	//int inum = 4;
	QStandardItem* en = new QStandardItem(tr("En"));
	m_model->setHorizontalHeaderItem(0, en);
	tableView->setColumnWidth(0, 20);
	QStandardItem* track = new QStandardItem(tr("Track"));
	m_model->setHorizontalHeaderItem(1, track);
	tableView->setColumnWidth(1, 180);
	QStandardItem* port = new QStandardItem(tr("MidiPort"));
	m_model->setHorizontalHeaderItem(2, port);
	tableView->setColumnWidth(2, 120);
	QStandardItem* chan = new QStandardItem(tr("Chan"));
	m_model->setHorizontalHeaderItem(3, chan);
	tableView->setColumnWidth(3, 60);
	tableView->horizontalHeader()->setStretchLastSection(true);
}/*}}}*/

void MidiAssignDialog::updateCCTableHeader()/*{{{*/
{
	QStandardItem* en = new QStandardItem(tr("Sel"));
	m_ccmodel->setHorizontalHeaderItem(0, en);
	m_ccEdit->setColumnWidth(0, 30);
	QStandardItem* track = new QStandardItem(tr("Controller"));
	m_ccmodel->setHorizontalHeaderItem(1, track);
	m_ccEdit->setColumnWidth(1, 180);
	m_ccEdit->horizontalHeader()->setStretchLastSection(true);
}/*}}}*/

void MidiAssignDialog::showEvent(QShowEvent*)
{
	m_ccmodel->clear();
	m_trackname->setText("");
	cmbTypeSelected(m_lasttype);
}
