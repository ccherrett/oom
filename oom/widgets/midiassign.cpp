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

MidiAssignDialog::MidiAssignDialog(QWidget* parent):QDialog(parent)
{
	setupUi(this);
	m_midicontrols = (QStringList() << "Volume" << "Pan" << "Chor" << "Rev" << "Var" << "Rec" << "Mute" << "Solo");
	m_model = new QStandardItemModel(0, 11, this);
	tableView->setModel(m_model);
	_trackTypes = (QStringList() << "All Types" << "Outputs" << "Inputs" << "Auxs" << "Busses" << "Midi Tracks" << "Soft Synth" << "Audio Tracks");
	cmbType->addItems(_trackTypes);
	connect(cmbType, SIGNAL(currentIndexChanged(int)), SLOT(cmbTypeSelected(int)));
	cmbType->setCurrentIndex(0);

	TableSpinnerDelegate* tdelegate = new TableSpinnerDelegate(this, 0);
	TableSpinnerDelegate* chandelegate = new TableSpinnerDelegate(this, 0, 15);
	MidiPortDelegate* mpdelegate = new MidiPortDelegate(this);
	tableView->setItemDelegateForColumn(1, mpdelegate);
	tableView->setItemDelegateForColumn(2, chandelegate);
	for(int i = 3; i < 11; ++i)
		tableView->setItemDelegateForColumn(i, tdelegate);

	connect(btnClose, SIGNAL(clicked(bool)), SLOT(btnCloseClicked()));
	//updateTableHeader();
	cmbTypeSelected(0);
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

void MidiAssignDialog::cmbTypeSelected(int type)/*{{{*/
{
	//Perform actions to populate list below based on selected type
	//We need to repopulate and filter the allTrackList
	//"Audio_Out" "Audio_In" "Audio_Aux" "Audio_Group" "Midi" "Soft_Synth"
	QString defaultname;
	defaultname.sprintf("%d:%s", 1, midiPorts[0].portname().toLatin1().constData());
	m_model->clear();
	switch (type)
	{/*{{{*/
		case 0:
			for (ciTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
			{
				QList<QStandardItem*> rowData;/*{{{*/
				QStandardItem* trk = new QStandardItem((*t)->name());
				trk->setEditable(false);
				rowData.append(trk);
				QStandardItem* port = new QStandardItem(defaultname);
				port->setData(0, MidiPortRole);//TODO: This will be the data from MidiAssignData
				port->setEditable(true);
				rowData.append(port);
				QStandardItem* chan = new QStandardItem("0");
				chan->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
				chan->setEditable(true);
				rowData.append(chan);
				for(int i = 0; i < m_midicontrols.size(); ++i)
				{
					QStandardItem* item = new QStandardItem("0");
					item->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
					rowData.append(item);
					item->setEditable(true);
					int op = 6;
					if((*t)->type() == Track::AUDIO_OUTPUT)
						op = 5;
					if(!(*t)->isMidiTrack() && (i > 1 && i < op))
					{
						item->setEditable(false);
					}
				}
				m_model->appendRow(rowData);/*}}}*/
			}
			break;
		case 1:
			for (ciTrack t = song->outputs()->begin(); t != song->outputs()->end(); ++t)
			{
				QList<QStandardItem*> rowData;/*{{{*/
				QStandardItem* trk = new QStandardItem((*t)->name());
				trk->setEditable(false);
				rowData.append(trk);
				QStandardItem* port = new QStandardItem(defaultname);
				port->setData(0, MidiPortRole);//TODO: This will be the data from MidiAssignData
				port->setEditable(true);
				rowData.append(port);
				QStandardItem* chan = new QStandardItem("0");
				chan->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
				chan->setEditable(true);
				rowData.append(chan);
				for(int i = 0; i < m_midicontrols.size(); ++i)
				{
					QStandardItem* item = new QStandardItem("0");
					item->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
					rowData.append(item);
					item->setEditable(true);
					if((i > 1 && i < 5))
					{
						item->setEditable(false);
					}
				}
				m_model->appendRow(rowData);/*}}}*/
			}
			break;
		case 2:
			for (ciTrack t = song->inputs()->begin(); t != song->inputs()->end(); ++t)
			{
				QList<QStandardItem*> rowData;/*{{{*/
				QStandardItem* trk = new QStandardItem((*t)->name());
				trk->setEditable(false);
				rowData.append(trk);
				QStandardItem* port = new QStandardItem(defaultname);
				port->setData(0, MidiPortRole);//TODO: This will be the data from MidiAssignData
				port->setEditable(true);
				rowData.append(port);
				QStandardItem* chan = new QStandardItem("0");
				chan->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
				chan->setEditable(true);
				rowData.append(chan);
				for(int i = 0; i < m_midicontrols.size(); ++i)
				{
					QStandardItem* item = new QStandardItem("0");
					item->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
					rowData.append(item);
					item->setEditable(true);
					if((i > 1 && i < 6))
					{
						item->setEditable(false);
					}
				}
				m_model->appendRow(rowData);/*}}}*/
			}
			break;
		case 3:
			for (ciTrack t = song->auxs()->begin(); t != song->auxs()->end(); ++t)
			{
				QList<QStandardItem*> rowData;/*{{{*/
				QStandardItem* trk = new QStandardItem((*t)->name());
				trk->setEditable(false);
				rowData.append(trk);
				QStandardItem* port = new QStandardItem(defaultname);
				port->setData(0, MidiPortRole);//TODO: This will be the data from MidiAssignData
				port->setEditable(true);
				rowData.append(port);
				QStandardItem* chan = new QStandardItem("0");
				chan->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
				chan->setEditable(true);
				rowData.append(chan);
				for(int i = 0; i < m_midicontrols.size(); ++i)
				{
					QStandardItem* item = new QStandardItem("0");
					item->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
					rowData.append(item);
					item->setEditable(true);
					if((i > 1 && i < 6))
					{
						item->setEditable(false);
					}
				}
				m_model->appendRow(rowData);/*}}}*/
			}
			break;
		case 4:
			for (ciTrack t = song->groups()->begin(); t != song->groups()->end(); ++t)
			{
				QList<QStandardItem*> rowData;/*{{{*/
				QStandardItem* trk = new QStandardItem((*t)->name());
				trk->setEditable(false);
				rowData.append(trk);
				QStandardItem* port = new QStandardItem(defaultname);
				port->setData(0, MidiPortRole);//TODO: This will be the data from MidiAssignData
				port->setEditable(true);
				rowData.append(port);
				QStandardItem* chan = new QStandardItem("0");
				chan->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
				chan->setEditable(true);
				rowData.append(chan);
				for(int i = 0; i < m_midicontrols.size(); ++i)
				{
					QStandardItem* item = new QStandardItem("0");
					item->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
					rowData.append(item);
					item->setEditable(true);
					if((i > 1 && i < 6))
					{
						item->setEditable(false);
					}
				}
				m_model->appendRow(rowData);/*}}}*/
			}
			break;
		case 5:
			for (ciTrack t = song->midis()->begin(); t != song->midis()->end(); ++t)
			{
				QList<QStandardItem*> rowData;/*{{{*/
				QStandardItem* trk = new QStandardItem((*t)->name());
				trk->setEditable(false);
				rowData.append(trk);
				QStandardItem* port = new QStandardItem(defaultname);
				port->setData(0, MidiPortRole);//TODO: This will be the data from MidiAssignData
				port->setEditable(true);
				rowData.append(port);
				QStandardItem* chan = new QStandardItem("0");
				chan->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
				chan->setEditable(true);
				rowData.append(chan);
				for(int i = 0; i < m_midicontrols.size(); ++i)
				{
					QStandardItem* item = new QStandardItem("0");
					item->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
					rowData.append(item);
					item->setEditable(true);
				}
				m_model->appendRow(rowData);/*}}}*/
			}
			break;
		case 6:
			for (ciTrack t = song->syntis()->begin(); t != song->syntis()->end(); ++t)
			{
				QList<QStandardItem*> rowData;/*{{{*/
				QStandardItem* trk = new QStandardItem((*t)->name());
				trk->setEditable(false);
				rowData.append(trk);
				QStandardItem* port = new QStandardItem(defaultname);
				port->setData(0, MidiPortRole);//TODO: This will be the data from MidiAssignData
				port->setEditable(true);
				rowData.append(port);
				QStandardItem* chan = new QStandardItem("0");
				chan->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
				rowData.append(chan);
				for(int i = 0; i < m_midicontrols.size(); ++i)
				{
					QStandardItem* item = new QStandardItem("0");
					item->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
					rowData.append(item);
					item->setEditable(true);
					if((i > 1 && i < 5))
					{
						item->setEditable(false);
					}
				}
				m_model->appendRow(rowData);/*}}}*/
			}
			break;
		case 7:
			for (ciTrack t = song->waves()->begin(); t != song->waves()->end(); ++t)
			{
				QList<QStandardItem*> rowData;/*{{{*/
				QStandardItem* trk = new QStandardItem((*t)->name());
				trk->setEditable(false);
				rowData.append(trk);
				QStandardItem* port = new QStandardItem(defaultname);
				port->setData(0, MidiPortRole);//TODO: This will be the data from MidiAssignData
				port->setEditable(true);
				rowData.append(port);
				QStandardItem* chan = new QStandardItem("0");
				chan->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
				chan->setEditable(true);
				rowData.append(chan);
				for(int i = 0; i < m_midicontrols.size(); ++i)
				{
					QStandardItem* item = new QStandardItem("0");
					item->setData(0, Qt::EditRole); //TODO: This will be the data from MidiAssignData
					rowData.append(item);
					item->setEditable(true);
					if(i > 1 && i < 5)
					{
						item->setEditable(false);
					}
				}
				m_model->appendRow(rowData);/*}}}*/
			}
			break;
	}/*}}}*/
	updateTableHeader();
}/*}}}*/

void MidiAssignDialog::updateTableHeader()/*{{{*/
{
	int inum = 3;
	QStandardItem* track = new QStandardItem(tr("Track"));
	m_model->setHorizontalHeaderItem(0, track);
	tableView->setColumnWidth(0, 180);
	QStandardItem* port = new QStandardItem(tr("MidiPort"));
	m_model->setHorizontalHeaderItem(1, port);
	tableView->setColumnWidth(1, 120);
	QStandardItem* chan = new QStandardItem(tr("Chan"));
	m_model->setHorizontalHeaderItem(2, chan);
	tableView->setColumnWidth(2, 60);
	foreach(QString ctl, m_midicontrols)
	{
		QStandardItem* item = new QStandardItem(ctl);
		m_model->setHorizontalHeaderItem(inum, item);
		tableView->setColumnWidth(inum, 60);
		inum++;
	}
	tableView->horizontalHeader()->setStretchLastSection(true);
}/*}}}*/
