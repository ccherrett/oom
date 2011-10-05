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
#include "midipresetdelegate.h"
#include "ccdelegate.h"
#include "ccinfo.h"
#include "midimonitor.h"
#include "confmport.h"
//#include "midisyncimpl.h"
#include "sync.h"
#include "driver/audiodev.h"
#include "traverso_shared/TConfig.h"

MidiAssignDialog::MidiAssignDialog(QWidget* parent):QDialog(parent)
{
	setupUi(this);
	m_lasttype = 0;
	m_selected = 0;
	m_selectport = 0;

	midiPortConfig = new MPConfig(this);
	//midiSyncConfig = new MidiSyncConfig(this);
	audioPortConfig = new AudioPortConfig(this);
	m_tabpanel->insertTab(0, audioPortConfig, tr("Audio Routing Manager"));
	m_tabpanel->insertTab(1, midiPortConfig, tr("Midi Port Manager"));
	//m_tabpanel->insertTab(4, midiSyncConfig, tr("Midi Sync"));
	//m_tabpanel->setCurrentIndex(0);
	m_btnReset = m_buttonBox->button(QDialogButtonBox::Reset);

	m_btnDelete->setIcon(*garbageIconSet3);

	m_btnAdd->setIcon(*plusIconSet3);

	m_btnDeletePreset->setIcon(*garbageIconSet3);

	m_btnAddPreset->setIcon(*plusIconSet3);

	m_assignlabels = (QStringList() << "En" << "Track" << "Midi Port" << "Chan" << "Preset" );
	m_cclabels = (QStringList() << "Sel" << "Controller");
	m_mplabels = (QStringList() << "Midi Port");
	m_presetlabels = (QStringList() << "Sel" << "Id" << "Sysex Text");

	m_allowed << CTRL_VOLUME << CTRL_PANPOT << CTRL_REVERB_SEND << CTRL_CHORUS_SEND << CTRL_VARIATION_SEND << CTRL_RECORD << CTRL_MUTE << CTRL_SOLO ;
	
	m_model = new QStandardItemModel(0, 5, this);
	tableView->setModel(m_model);
	m_selmodel = new QItemSelectionModel(m_model);
	tableView->setSelectionModel(m_selmodel);

	m_ccmodel = new QStandardItemModel(0, 2, this);
	m_ccEdit->setModel(m_ccmodel);
	m_ccEdit->setSortingEnabled(true);

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
	TableSpinnerDelegate* chandelegate = new TableSpinnerDelegate(this, 1, 16);
	MidiPortDelegate* mpdelegate = new MidiPortDelegate(this);
	MidiPresetDelegate* presetdelegate = new MidiPresetDelegate(this);
	CCInfoDelegate *infodelegate = new CCInfoDelegate(this);
	tableView->setItemDelegateForColumn(2, mpdelegate);
	tableView->setItemDelegateForColumn(3, chandelegate);
	tableView->setItemDelegateForColumn(4, presetdelegate);
	m_ccmodel->setSortRole(CCSortRole);
	
	m_ccEdit->setItemDelegateForColumn(1, infodelegate);
	m_cmbControl->addItem(midiControlToString(CTRL_RECORD), CTRL_RECORD);
	m_cmbControl->addItem(midiControlToString(CTRL_MUTE), CTRL_MUTE);
	m_cmbControl->addItem(midiControlToString(CTRL_SOLO), CTRL_SOLO);
	m_cmbControl->addItem(midiControlToString(CTRL_AUX1), CTRL_AUX1);
	m_cmbControl->addItem(midiControlToString(CTRL_AUX2), CTRL_AUX2);
	m_cmbControl->addItem(midiControlToString(CTRL_AUX3), CTRL_AUX3);
	m_cmbControl->addItem(midiControlToString(CTRL_AUX4), CTRL_AUX4);
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
	
	//TODO: Prepare the midi sync connections
	connect(useJackTransportCheckbox, SIGNAL(stateChanged(int)), SLOT(updateUseJackTransport(int)));
	connect(jackTransportMasterCheckbox, SIGNAL(stateChanged(int)), SLOT(updateJackMaster(int)));
	connect(extSyncCheckbox, SIGNAL(stateChanged(int)), SLOT(updateSlaveSync(int)));
	connect(syncDelaySpinBox, SIGNAL(valueChanged(int)), SLOT(updateSyncDelay(int)));
	connect(mtcSyncType, SIGNAL(currentIndexChanged(int)), SLOT(updateMTCType(int)));
	connect(mtcOffH, SIGNAL(valueChanged(int)), SLOT(updateMTCHour(int)));
	connect(mtcOffM, SIGNAL(valueChanged(int)), SLOT(updateMTCMinute(int)));
	connect(mtcOffS, SIGNAL(valueChanged(int)), SLOT(updateMTCSecond(int)));
	connect(mtcOffF, SIGNAL(valueChanged(int)), SLOT(updateMTCFrame(int)));
	connect(mtcOffSf, SIGNAL(valueChanged(int)), SLOT(updateMTCSubFrame(int)));
	connect(m_midirewplay, SIGNAL(stateChanged(int)), SLOT(updateInputRewindBeforePlay(int)));
	connect(m_imididevid, SIGNAL(valueChanged(int)), SLOT(updateInputDeviceId(int)));
	connect(m_imidiclock, SIGNAL(stateChanged(int)), SLOT(updateInputClock(int)));
	connect(m_imidirtinput, SIGNAL(stateChanged(int)), SLOT(updateInputRealtime(int)));
	connect(m_imidimmc, SIGNAL(stateChanged(int)), SLOT(updateInputMMC(int)));
	connect(m_imidimtc, SIGNAL(stateChanged(int)), SLOT(updateInputMTC(int)));
	connect(m_omididevid, SIGNAL(valueChanged(int)), SLOT(updateOutputDeviceId(int)));
	connect(m_omidiclock, SIGNAL(stateChanged(int)), SLOT(updateOutputClock(int)));
	connect(m_omidirtoutput, SIGNAL(stateChanged(int)), SLOT(updateOutputRealtime(int)));
	connect(m_omidimmc, SIGNAL(stateChanged(int)), SLOT(updateOutputMMC(int)));
	connect(m_omidimtc,SIGNAL(stateChanged(int)), SLOT(updateOutputMTC(int)));
	
	cmbTypeSelected(m_lasttype);
	switchTabs(0);
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
					data->channel = item->data(Qt::EditRole).toInt()-1;
				break;
				case 4: //Preset
					data->preset = item->data(MidiPresetRole).toInt();
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
						control->setData(midiControlSortIndex(info->controller()), CCSortRole);
						QString str;
						str.append("( ").append(midiControlToString(info->controller())).append(" )");
						if(info->assignedControl() >= 0)
							str.append(" Assigned to CC: ").append(QString::number(info->assignedControl())).append(" on Chan: ").append(QString::number(info->channel()+1));
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
				case CTRL_AUX1:
				case CTRL_AUX2:
				case CTRL_AUX3:
				case CTRL_AUX4:
					allowed = ((AudioTrack*)m_selected)->hasAuxSend();
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
			control->setData(midiControlSortIndex(info->controller()), CCSortRole);
			QString str;
			str.append("( ").append(midiControlToString(info->controller())).append(" )");
			if(info->assignedControl() >= 0)
				str.append(" Assigned to CC: ").append(QString::number(info->assignedControl())).append(" on Chan: ").append(QString::number(info->channel()));
			control->setData(str, Qt::DisplayRole);
			rowData.append(control);
			m_ccmodel->appendRow(rowData);
			m_ccEdit->setRowHeight(m_ccmodel->rowCount()-1, 50);
			song->dirty = true;
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
				song->dirty = true;
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
		QStandardItem* chan = new QStandardItem(QString::number(data->channel+1));
		chan->setData(data->channel+1, Qt::EditRole);
		chan->setEditable(true);
		rowData.append(chan);
		QStandardItem* preset = new QStandardItem(QString::number(data->preset));
		if(!data->preset)
			preset->setData(tr("None"), Qt::DisplayRole);
		preset->setData(data->preset, MidiPresetRole);
		preset->setEditable(true);
		rowData.append(preset);
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
	tableView->setColumnWidth(4, 50);
	m_model->setHorizontalHeaderLabels(m_assignlabels);
	tableView->horizontalHeader()->setStretchLastSection(true);
}/*}}}*/

void MidiAssignDialog::updateCCTableHeader()/*{{{*/
{
	m_ccEdit->setColumnWidth(0, 30);
	m_ccEdit->setColumnWidth(1, 180);
	m_ccmodel->setHorizontalHeaderLabels(m_cclabels);
	m_ccEdit->horizontalHeader()->setStretchLastSection(true);
	m_ccEdit->horizontalHeader()->setSortIndicatorShown(false);
	m_ccEdit->sortByColumn(1, Qt::AscendingOrder);
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
			song->dirty = true;
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
	populateMMCSettings();
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
		song->dirty = true;
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
			song->dirty = true;
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
	populateSyncInfo();
	populateMMCSettings();
	//if(midiSyncConfig)
	//	midiSyncConfig->songChanged(-1);
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
			populateSyncInfo();
			populateMMCSettings();
		break;
		//case 4: //MidiSync
		//	midiSyncConfig->songChanged(-1);
		//break;
	}
}/*}}}*/

void MidiAssignDialog::switchTabs(int tab)
{
	m_tabpanel->setCurrentIndex(tab);
}

//midi sync transport
void MidiAssignDialog::populateSyncInfo()/*{{{*/
{
	extSyncCheckbox->blockSignals(true);
	useJackTransportCheckbox->blockSignals(true);
	jackTransportMasterCheckbox->blockSignals(true);
	syncDelaySpinBox->blockSignals(true);
	extSyncCheckbox->setChecked(extSyncFlag.value());
	useJackTransportCheckbox->setChecked(useJackTransport.value());
	jackTransportMasterCheckbox->setChecked(jackTransportMaster);
	syncDelaySpinBox->setValue(syncSendFirstClockDelay);
	syncDelaySpinBox->blockSignals(false);
	jackTransportMasterCheckbox->blockSignals(false);
	useJackTransportCheckbox->blockSignals(false);
	extSyncCheckbox->blockSignals(false);

	mtcSyncType->blockSignals(true);
	mtcSyncType->setCurrentIndex(mtcType);
	mtcSyncType->blockSignals(false);

	mtcOffH->blockSignals(true);
	mtcOffM->blockSignals(true);
	mtcOffS->blockSignals(true);
	mtcOffF->blockSignals(true);
	mtcOffSf->blockSignals(true);
	mtcOffH->setValue(mtcOffset.h());
	mtcOffM->setValue(mtcOffset.m());
	mtcOffS->setValue(mtcOffset.s());
	mtcOffF->setValue(mtcOffset.f());
	mtcOffSf->setValue(mtcOffset.sf());
	mtcOffH->blockSignals(false);
	mtcOffM->blockSignals(false);
	mtcOffS->blockSignals(false);
	mtcOffF->blockSignals(false);
	mtcOffSf->blockSignals(false);
}/*}}}*/

void MidiAssignDialog::updateUseJackTransport(int val)
{
	useJackTransport.setValue(val == Qt::Checked);
}
void MidiAssignDialog::updateJackMaster(int val)
{
	if (audioDevice)
		audioDevice->setMaster(val == Qt::Checked);
}
void MidiAssignDialog::updateSlaveSync(int val)
{
	extSyncFlag.setValue(val == Qt::Checked);
}
void MidiAssignDialog::updateSyncDelay(int val)
{
	syncSendFirstClockDelay = val;
}
//MTC timing
void MidiAssignDialog::updateMTCType(int val)
{
	mtcType = val;
}
void MidiAssignDialog::updateMTCHour(int h)
{
	mtcOffset.setH(h);
}
void MidiAssignDialog::updateMTCMinute(int m)
{
	mtcOffset.setM(m);
}
void MidiAssignDialog::updateMTCSecond(int s)
{
	mtcOffset.setS(s);
}
void MidiAssignDialog::updateMTCFrame(int f)
{
	mtcOffset.setF(f);
}
void MidiAssignDialog::updateMTCSubFrame(int sf)
{
	mtcOffset.setSf(sf);
}

void MidiAssignDialog::populateMMCSettings()/*{{{*/
{
	m_midirewplay->blockSignals(true);

	m_imididevid->blockSignals(true);
	m_imidiclock->blockSignals(true);
	m_imidirtinput->blockSignals(true);
	m_imidimmc->blockSignals(true);
	m_imidimtc->blockSignals(true);

	m_omididevid->blockSignals(true);
	m_omidiclock->blockSignals(true);
	m_omidirtoutput->blockSignals(true);
	m_omidimmc->blockSignals(true);
	m_omidimtc->blockSignals(true);
	if(!m_selectport)
	{
		m_midirewplay->setChecked(false);

		m_imididevid->setValue(127);
		m_imidiclock->setChecked(false);
		m_imidirtinput->setChecked(false);
		m_imidimmc->setChecked(false);
		m_imidimtc->setChecked(false);

		m_omididevid->setValue(127);
		m_omidiclock->setChecked(false);
		m_omidirtoutput->setChecked(false);
		m_omidimmc->setChecked(false);
		m_omidimtc->setChecked(false);
	}
	else
	{
		MidiSyncInfo& si = m_selectport->syncInfo();
		m_midirewplay->setChecked(si.recRewOnStart());

		m_imididevid->setValue(si.idIn());
		m_imidiclock->setChecked(si.MCIn());
		m_imidirtinput->setChecked(si.MRTIn());
		m_imidimmc->setChecked(si.MMCIn());
		m_imidimtc->setChecked(si.MTCIn());

		m_omididevid->setValue(si.idOut());
		m_omidiclock->setChecked(si.MCOut());
		m_omidirtoutput->setChecked(si.MRTOut());
		m_omidimmc->setChecked(si.MMCOut());
		m_omidimtc->setChecked(si.MTCOut());
	}
	m_midirewplay->blockSignals(false);

	m_imididevid->blockSignals(false);
	m_imidiclock->blockSignals(false);
	m_imidirtinput->blockSignals(false);
	m_imidimmc->blockSignals(false);
	m_imidimtc->blockSignals(false);

	m_omididevid->blockSignals(false);
	m_omidiclock->blockSignals(false);
	m_omidirtoutput->blockSignals(false);
	m_omidimmc->blockSignals(false);
	m_omidimtc->blockSignals(false);
}/*}}}*/

//midi sync slots Input
void MidiAssignDialog::updateInputRewindBeforePlay(int val)
{
	if(!m_selectport)
		return;
	m_selectport->syncInfo().setRecRewOnStart(val == Qt::Checked);
}

void MidiAssignDialog::updateInputDeviceId(int val)
{
	if(!m_selectport)
		return;
	m_selectport->syncInfo().setIdIn(val);
}
void MidiAssignDialog::updateInputClock(int val)
{
	if(!m_selectport)
		return;
	m_selectport->syncInfo().setMCIn(val == Qt::Checked);
}
void MidiAssignDialog::updateInputRealtime(int val)
{
	if(!m_selectport)
		return;
	m_selectport->syncInfo().setMRTIn(val == Qt::Checked);
}
void MidiAssignDialog::updateInputMMC(int val)
{
	if(!m_selectport)
		return;
	m_selectport->syncInfo().setMMCIn(val == Qt::Checked);
}
void MidiAssignDialog::updateInputMTC(int val)
{
	if(!m_selectport)
		return;
	m_selectport->syncInfo().setMTCIn(val == Qt::Checked);
}

//midi sync slots Output
void MidiAssignDialog::updateOutputDeviceId(int val)
{
	if(!m_selectport)
		return;
	m_selectport->syncInfo().setIdOut(val);
}
void MidiAssignDialog::updateOutputClock(int val)
{
	if(!m_selectport)
		return;
	m_selectport->syncInfo().setMCOut(val == Qt::Checked);
}
void MidiAssignDialog::updateOutputRealtime(int val)
{
	if(!m_selectport)
		return;
	m_selectport->syncInfo().setMRTOut(val == Qt::Checked);
}
void MidiAssignDialog::updateOutputMMC(int val)
{
	if(!m_selectport)
		return;
	m_selectport->syncInfo().setMMCOut(val == Qt::Checked);
}
void MidiAssignDialog::updateOutputMTC(int val)
{
	if(!m_selectport)
		return;
	m_selectport->syncInfo().setMTCOut(val == Qt::Checked);
}

//Virtuals
void MidiAssignDialog::showEvent(QShowEvent*)
{
	currentTabChanged(m_tabpanel->currentIndex());
	resize(tconfig().get_property("ConnectionsManager", "size", QSize(891, 691)).toSize());
	move(tconfig().get_property("ConnectionsManager", "pos", QPoint(0, 0)).toPoint());

	/*QString qsrc("F0 41 10 42 12 40 00 7F 00 41 F7");
	QByteArray ba = qsrc.toLatin1();
	const char* src = ba.constData();

	int len;
	int stat;
	unsigned char* sysex = (unsigned char*) hex2string(src, len, stat);
	QString rev = string2hex(sysex, len);
	qDebug() << "HexToString: " << sysex << " Length: " << len << " StringToHex: " << rev;*/
	//btnResetClicked();
}

void MidiAssignDialog::closeEvent(QCloseEvent*)
{
	tconfig().set_property("ConnectionsManager", "size", size());
	tconfig().set_property("ConnectionsManager", "pos", pos());
    // Save the new global settings to the configuration file
    tconfig().save();
}
