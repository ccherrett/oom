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
#include "apconfig.h"
#include "song.h"
#include "globals.h"
#include "config.h"
#include "icons.h"
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
#include "confmport.h"
#include "midisyncimpl.h"
#include "traverso_shared/TConfig.h"

MidiAssignDialog::MidiAssignDialog(QWidget* parent):QDialog(parent)
{
	setupUi(this);
	m_lasttype = 0;
	m_selected = 0;
	m_selectport = 0;

	midiPortConfig = new MPConfig(this);
	midiSyncConfig = new MidiSyncConfig(this);
	audioPortConfig = new AudioPortConfig(this);
	m_tabpanel->insertTab(0, audioPortConfig, tr("Audio Routing Manager"));
	m_tabpanel->insertTab(1, midiPortConfig, tr("Midi Port Manager"));
	m_tabpanel->insertTab(4, midiSyncConfig, tr("Midi Sync"));
	m_tabpanel->setCurrentIndex(0);
	m_btnReset = m_buttonBox->button(QDialogButtonBox::Reset);

	m_btnDelete->setIcon(*garbagePCIcon);
	m_btnDelete->setIconSize(garbagePCIcon->size());

	m_btnAdd->setIcon(*addTVIcon);
	m_btnAdd->setIconSize(addTVIcon->size());

	m_btnDeletePreset->setIcon(*garbagePCIcon);
	m_btnDeletePreset->setIconSize(garbagePCIcon->size());

	m_btnAddPreset->setIcon(*addTVIcon);
	m_btnAddPreset->setIconSize(addTVIcon->size());

	m_assignlabels = (QStringList() << "En" << "Track" << "Midi Port" << "Chan" );
	m_cclabels = (QStringList() << "Sel" << "Controller");
	m_mplabels = (QStringList() << "Midi Port");
	m_presetlabels = (QStringList() << "Sel" << "Id" << "Sysex Text");

	m_allowed << CTRL_VOLUME << CTRL_PANPOT << CTRL_REVERB_SEND << CTRL_CHORUS_SEND << CTRL_VARIATION_SEND << CTRL_RECORD << CTRL_MUTE << CTRL_SOLO ;
	
	m_model = new QStandardItemModel(0, 4, this);
	tableView->setModel(m_model);
	m_selmodel = new QItemSelectionModel(m_model);
	tableView->setSelectionModel(m_selmodel);

	m_ccmodel = new QStandardItemModel(0, 2, this);
	m_ccEdit->setModel(m_ccmodel);

	m_mpmodel = new QStandardItemModel(0, 1, this);
	m_porttable->setModel(m_mpmodel);
	m_mpselmodel = new QItemSelectionModel(m_mpmodel);
	m_porttable->setSelectionModel(m_mpselmodel);
	m_presetmodel = new QStandardItemModel(0, 3, this);
	m_presettable->setModel(m_presetmodel);

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
	
	m_ccEdit->setItemDelegateForColumn(1, infodelegate);
	m_cmbControl->addItem(tr("Track Record"), CTRL_RECORD);
	m_cmbControl->addItem(tr("Track Mute"), CTRL_MUTE);
	m_cmbControl->addItem(tr("Track Solo"), CTRL_SOLO);
	for(int i = 0; i < 128; ++i)
	{
		QString ctl(QString::number(i)+": ");
		m_cmbControl->addItem(ctl.append(midiCtrlName(i)), i);
	}

	//populateMidiPorts();

	connect(m_btnReset, SIGNAL(clicked(bool)), SLOT(btnResetClicked()));
	connect(m_model, SIGNAL(itemChanged(QStandardItem*)), SLOT(itemChanged(QStandardItem*)));
	connect(m_selmodel, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), SLOT(itemSelected(const QItemSelection&, const QItemSelection&)));
	connect(m_btnAdd, SIGNAL(clicked()), SLOT(btnAddController()));
	connect(m_btnDelete, SIGNAL(clicked()), SLOT(btnDeleteController()));
	connect(m_btnDefault, SIGNAL(clicked()), SLOT(btnUpdateDefault()));
	connect(m_mpselmodel, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), SLOT(midiPortSelected(const QItemSelection&, const QItemSelection&)));
	connect(m_presetmodel, SIGNAL(itemChanged(QStandardItem*)), SLOT(midiPresetChanged(QStandardItem*)));
	connect(m_btnAddPreset, SIGNAL(clicked()), SLOT(btnAddMidiPreset()));
	connect(m_btnDeletePreset, SIGNAL(clicked()), SLOT(btnDeleteMidiPresets()));
	connect(m_tabpanel, SIGNAL(currentChanged(int)), SLOT(currentTabChanged(int)));
	cmbTypeSelected(m_lasttype);
}

MidiAssignDialog::~MidiAssignDialog()
{
	tconfig().set_property("ConnectionsManager", "size", size());
	tconfig().set_property("ConnectionsManager", "pos", pos());
    // Save the new global settings to the configuration file
    tconfig().save();
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
			}
		}
	}
}/*}}}*/

//Update the ccEdit table
void MidiAssignDialog::itemSelected(const QItemSelection& isel, const QItemSelection&) /*{{{*/
{
	//printf("Selection changed\n");
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
						QList<QStandardItem*> rowData;
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
						m_ccmodel->appendRow(rowData);
						m_ccEdit->setRowHeight(m_ccmodel->rowCount()-1, 50);
					}
				}
			}
		}
	}
	updateCCTableHeader();
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
					if(m_selected->type() == Track::AUDIO_OUTPUT || m_selected->type() == Track::WAVE)
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
				//printf("Delete clicked\n");
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

void MidiAssignDialog::btnUpdateDefault()/*{{{*/
{
	//printf("MidiAssignDialog::btnUpdateDefault rowCount:%d \n", m_model->rowCount());
	bool override = false;
	if(m_chkOverride->isChecked())
	{
		int btn = QMessageBox::question(this, tr("Midi Assign Change"), tr("You are about to override the settings of pre-assigned tracks.\nAre you sure you want to do this?"),QMessageBox::Ok|QMessageBox::Cancel);
		if(btn == QMessageBox::Ok)
			override = true;
		else
			return; //Dont do anything as they canceled
	}
	for(int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem* trk = m_model->item(i, 1);
		Track* track = song->findTrack(trk->text());
		if(track)
		{
			MidiAssignData* data = track->midiAssign();
			bool unassigned = true;
			int p = data->port;
			QHashIterator<int, CCInfo*> iter(data->midimap);
			if(!override)
			{
				while(iter.hasNext())
				{
					iter.next();
					CCInfo* info = iter.value();
					if(info->assignedControl() >= 0)
					{
						unassigned = false;
						break;
					}
				}
			}
			if(unassigned)
			{
				iter.toFront();
				data->port = m_cmbPort->currentIndex();
				while(iter.hasNext())
				{
					iter.next();
					CCInfo* info = iter.value();
					info->setPort(data->port);
				}
				midiMonitor->msgModifyTrackPort(track, p);
			}
		}
	}
	//Update the view
	m_ccmodel->clear();
	m_trackname->setText("");
	cmbTypeSelected(m_lasttype);
}/*}}}*/

void MidiAssignDialog::cmbTypeSelected(int type)/*{{{*/
{
	//Perform actions to populate list below based on selected type
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
	tableView->setColumnWidth(0, 25);
	tableView->setColumnWidth(1, 180);
	tableView->setColumnWidth(2, 120);
	tableView->setColumnWidth(3, 60);
	m_model->setHorizontalHeaderLabels(m_assignlabels);
	tableView->horizontalHeader()->setStretchLastSection(true);
}/*}}}*/

void MidiAssignDialog::updateCCTableHeader()/*{{{*/
{
	m_ccEdit->setColumnWidth(0, 30);
	m_ccEdit->setColumnWidth(1, 180);
	m_ccmodel->setHorizontalHeaderLabels(m_cclabels);
	m_ccEdit->horizontalHeader()->setStretchLastSection(true);
}/*}}}*/

void MidiAssignDialog::updateMPTableHeader()/*{{{*/
{
	m_mpmodel->setHorizontalHeaderLabels(m_mplabels);
	m_porttable->horizontalHeader()->setStretchLastSection(true);

	m_presettable->setColumnWidth(0, 30);
	m_presettable->setColumnWidth(1, 30);
	m_presettable->setColumnWidth(2, 200);
	m_presetmodel->setHorizontalHeaderLabels(m_presetlabels);
	m_presettable->horizontalHeader()->setStretchLastSection(true);
}/*}}}*/

//MidiPort Presets functions

void MidiAssignDialog::midiPresetChanged(QStandardItem* item)/*{{{*/
{
	if(!m_selectport)
		return;
	if(item && item->column() == 2)
	{
		QStandardItem* num = m_presetmodel->item(item->row(), 1);
		if(num)
		{
			int id = num->text().toInt();
			m_selectport->addPreset(id, item->text());
		//TODO: Call song->dirty = true;
		}
	}
}/*}}}*/

void MidiAssignDialog::midiPortSelected(const QItemSelection& isel, const QItemSelection&)/*{{{*/
{
	m_presetmodel->clear();
	m_portlabel->setText("");
	QModelIndexList list = isel.indexes();
	if(list.size() > 0)
	{
		QModelIndex index = list.at(0);
		int row = index.row();
		QStandardItem* item = m_mpmodel->item(row, 0);
		if(item)
		{
			MidiPort* mport = &midiPorts[item->data(MidiPortRole).toInt()];
			if(mport)
			{
				m_selectport = mport;
				m_portlabel->setText(item->text());
				QHash<int, QString> *presets = mport->presets();
				QHashIterator<int, QString> iter(*presets);
				while(iter.hasNext())
				{
					iter.next();
					QList<QStandardItem*> rowData;
					QStandardItem* chk = new QStandardItem(true);
					chk->setCheckable(true);
					chk->setEditable(false);
					rowData.append(chk);
					QStandardItem* num = new QStandardItem(QString::number(iter.key()));
					num->setEditable(false);
					rowData.append(num);
					QStandardItem* sysex = new QStandardItem(iter.value());
					rowData.append(sysex);
					m_presetmodel->appendRow(rowData);
				}
			}
		}
	}
	updateMPTableHeader();
}/*}}}*/

void MidiAssignDialog::btnAddMidiPreset()/*{{{*/
{
	if(!m_selectport)
		return;
	if(!m_txtPreset->text().isEmpty())
	{
		int id = m_txtPresetID->value();
		QString sysex = m_txtPreset->text();
		if(m_selectport->hasPreset(id))
		{
			int btn = QMessageBox::question(this, tr("Midi Preset Change"), tr("There is already a preset with the selected id \nAre you sure you want to do overwrite this preset?"),QMessageBox::Ok|QMessageBox::Cancel);
			if(btn != QMessageBox::Ok)
				return; //Dont do anything as they canceled
		}
		QList<QStandardItem*> rowData;
		QStandardItem* chk = new QStandardItem(true);
		chk->setCheckable(true);
		chk->setEditable(false);
		rowData.append(chk);
		QStandardItem* num = new QStandardItem(QString::number(id));
		num->setEditable(false);
		rowData.append(num);
		QStandardItem* sys = new QStandardItem(sysex);
		rowData.append(sys);
		m_selectport->addPreset(id, sysex);
		m_presetmodel->appendRow(rowData);
		updateMPTableHeader();
		//TODO: Call song->dirty = true;
	}
}/*}}}*/

void MidiAssignDialog::btnDeleteMidiPresets()/*{{{*/
{
	if(!m_selectport)
		return;
	bool del = false;
	for(int i = 0; i < m_presetmodel->rowCount(); ++i)
	{
		QStandardItem* chk = m_presetmodel->item(i, 0);
		if(chk->checkState() == Qt::Checked)
		{
			QStandardItem* num = m_presetmodel->item(i, 1);
			m_selectport->removePreset(num->text().toInt());
			m_presetmodel->takeRow(i);
			del = true;
			//TODO: Call song dirty
		}
	}
	if(del)
		updateMPTableHeader();
}/*}}}*/

void MidiAssignDialog::btnResetClicked()/*{{{*/
{
	m_selectport = 0;
	m_presetmodel->clear();
	populateMidiPorts();
	m_ccmodel->clear();
	m_trackname->setText("");
	cmbTypeSelected(m_lasttype);

	m_portlabel->setText("");
	updateMPTableHeader();
	if(midiSyncConfig)
		midiSyncConfig->songChanged(-1);
	if(midiPortConfig)
		midiPortConfig->songChanged(-1);
}/*}}}*/

void MidiAssignDialog::populateMidiPorts()/*{{{*/
{
	m_mpmodel->clear();
	QAbstractItemModel* mod = m_cmbPort->model();
	if(mod && mod->rowCount() > 0)
		mod->removeRows(0, mod->rowCount());
	for (int i = 0; i < MIDI_PORTS; ++i)
	{
		QString name;
		name.sprintf("%d:%s", i + 1, midiPorts[i].portname().toLatin1().constData());
		//Populate the midiport combo in midiassign
		m_cmbPort->insertItem(i, name);
		//Populate the midiPort table in midi presets
		QStandardItem* port = new QStandardItem(name);
		port->setData(i, MidiPortRole);
		m_mpmodel->appendRow(port);
	}
	updateMPTableHeader();
}/*}}}*/

void MidiAssignDialog::currentTabChanged(int flags)/*{{{*/
{
	switch(flags)
	{
		case 0: //AudioPortConfig
		break;
		case 1: //MidiPortConfig
			midiPortConfig->songChanged(-1);
		break;
		case 2: //MidiPortPreset
		case 3: //MidiAssign
			m_selectport = 0;
			m_presetmodel->clear();
			populateMidiPorts();
			m_ccmodel->clear();
			m_trackname->setText("");
			cmbTypeSelected(m_lasttype);

			m_portlabel->setText("");
			updateMPTableHeader();
		break;
		case 4: //MidiSync
			midiSyncConfig->songChanged(-1);
		break;
	}
}/*}}}*/

void MidiAssignDialog::switchTabs(int tab)
{
	m_tabpanel->setCurrentIndex(tab);
}

//Virtuals
void MidiAssignDialog::showEvent(QShowEvent*)
{
	currentTabChanged(m_tabpanel->currentIndex());
	resize(tconfig().get_property("ConnectionsManager", "size", QSize(891, 691)).toSize());
	move(tconfig().get_property("ConnectionsManager", "pos", QPoint(0, 0)).toPoint());
	//btnResetClicked();
	//printf("Midi Buffer size: %d\n", MIDI_FIFO_SIZE);
	//QString idstr = QString::number(genId());
	//QByteArray ba(idstr.toUtf8().constData());
	//qDebug() << "ID Size: " << ba.size() << " ID: " << idstr;
}

void MidiAssignDialog::closeEvent(QCloseEvent*)
{
	tconfig().set_property("ConnectionsManager", "size", size());
	tconfig().set_property("ConnectionsManager", "pos", pos());
    // Save the new global settings to the configuration file
    tconfig().save();
}
