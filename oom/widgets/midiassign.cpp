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
#include "midimonitor.h"

MidiAssignDialog::MidiAssignDialog(QWidget* parent):QDialog(parent)
{
	setupUi(this);
	m_lasttype = 0;
	m_midicontrols = (QStringList() << "Volume" << "Pan" << "Chor" << "Rev" << "Var" << "Rec" << "Mute" << "Solo");
	m_allowed << CTRL_VOLUME << CTRL_PANPOT << CTRL_REVERB_SEND << CTRL_CHORUS_SEND << CTRL_VARIATION_SEND << CTRL_RECORD << CTRL_MUTE << CTRL_SOLO ;
	m_model = new QStandardItemModel(0, 11, this);
	tableView->setModel(m_model);
	_trackTypes = (QStringList() << "All Types" << "Outputs" << "Inputs" << "Auxs" << "Busses" << "Midi Tracks" << "Soft Synth" << "Audio Tracks");
	cmbType->addItems(_trackTypes);
	connect(cmbType, SIGNAL(currentIndexChanged(int)), SLOT(cmbTypeSelected(int)));
	cmbType->setCurrentIndex(0);

	TableSpinnerDelegate* tdelegate = new TableSpinnerDelegate(this, -1);
	TableSpinnerDelegate* chandelegate = new TableSpinnerDelegate(this, 0, 15);
	MidiPortDelegate* mpdelegate = new MidiPortDelegate(this);
	tableView->setItemDelegateForColumn(2, mpdelegate);
	tableView->setItemDelegateForColumn(3, chandelegate);
	for(int i = 4; i < 12; ++i)
		tableView->setItemDelegateForColumn(i, tdelegate);

	connect(btnClose, SIGNAL(clicked(bool)), SLOT(btnCloseClicked()));
	connect(m_model, SIGNAL(itemChanged(QStandardItem*)), SLOT(itemChanged(QStandardItem*)));
	//updateTableHeader();
	cmbTypeSelected(m_lasttype);
}
void MidiAssignDialog::btnCloseClicked()
{
	close();
}

void MidiAssignDialog::portChanged(int)
{
}

void MidiAssignDialog::channelChanged(int)
{
}

void MidiAssignDialog::itemChanged(QStandardItem* item)
{
	printf("itemChanged\n");
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
				case 11: //Solo*/
				default:
				{
					int ctl = item->data(MidiControlRole).toInt();
					int cc = data->midimap[ctl];
					data->midimap[ctl] = item->data(Qt::EditRole).toInt();
					midiMonitor->msgModifyTrackController(track, ctl, cc);
				}
				break;
			}
		}
	}
}

void MidiAssignDialog::cmbTypeSelected(int type)/*{{{*/
{
	//Perform actions to populate list below based on selected type
	//We need to repopulate and filter the allTrackList
	//"Audio_Out" "Audio_In" "Audio_Aux" "Audio_Group" "Midi" "Soft_Synth"
	//m_model->blockSignals(true);
	m_lasttype = type;
	QString defaultname;
	defaultname.sprintf("%d:%s", 1, midiPorts[0].portname().toLatin1().constData());
	m_model->clear();

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
		QList<QStandardItem*> rowData;/*{{{*/
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
		for(int i = 0; i < m_allowed.size(); ++i)
		{
			int ctl = m_allowed.at(i);
			QStandardItem* item = new QStandardItem(QString::number(data->midimap[ctl]));
			item->setData(data->midimap[ctl], Qt::EditRole);
			item->setData(ctl, MidiControlRole);
			rowData.append(item);
			item->setEditable(true);
			if(!(*t)->isMidiTrack())
			{
				item->setEditable(false);
				switch(ctl)/*{{{*/
				{
					case CTRL_VOLUME:
					case CTRL_PANPOT:
					case CTRL_RECORD:
					case CTRL_MUTE:
					case CTRL_SOLO:
						item->setEditable(true);
					break;
				}/*}}}*/
			}
		}
		m_model->appendRow(rowData);/*}}}*/
	}
	updateTableHeader();
	//m_model->blockSignals(false);
}/*}}}*/

void MidiAssignDialog::updateTableHeader()/*{{{*/
{
	int inum = 4;
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
	foreach(QString ctl, m_midicontrols)
	{
		QStandardItem* item = new QStandardItem(ctl);
		m_model->setHorizontalHeaderItem(inum, item);
		tableView->setColumnWidth(inum, 60);
		inum++;
	}
	tableView->horizontalHeader()->setStretchLastSection(true);
}/*}}}*/

void MidiAssignDialog::showEvent(QShowEvent*)
{
	cmbTypeSelected(m_lasttype);
}
