//=========================================================
//  OOStudio
//  OpenOctave Midi and Audio Editor
//  $Id: plugin.h,v 1.9.2.13 2009/12/06 01:25:21 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#include <QtGui>
#include "config.h"
#include "plugindialog.h"
#include "track.h"
#include "traverso_shared/TConfig.h"

int PluginDialog::selectedPlugType = 0;
QStringList PluginDialog::sortItems = QStringList();

//---------------------------------------------------------
//   PluginDialog
//    select Plugin dialog
//---------------------------------------------------------

PluginDialog::PluginDialog(int type, QWidget* parent)
: QDialog(parent)
{
	m_display_type = PLUGIN_LV2;
	m_trackType = type;

	setWindowTitle(tr("OOStudio: select plugin"));
	QVBoxLayout* vbox = new QVBoxLayout(this);

	QHBoxLayout* layout = new QHBoxLayout();
	vbox->addLayout(layout);

	QVBoxLayout* panelbox = new QVBoxLayout();
	layout->addLayout(panelbox);

	pList = new QTreeWidget(this);
	pList->setColumnCount(3);
	pList->setSortingEnabled(true);
	QStringList headerLabels;
	headerLabels << tr("Stereo");
	headerLabels << tr("Category");
	headerLabels << tr("Name");
	/*headerLabels << tr("AI");
	headerLabels << tr("AO");
	headerLabels << tr("CI");
	headerLabels << tr("CO");
	headerLabels << tr("IP");
	headerLabels << tr("id");
	headerLabels << tr("Maker");
	headerLabels << tr("Copyright");*/

	pList->header()->resizeSection(0, 80);
	pList->header()->resizeSection(1, 120);
	pList->header()->setResizeMode(2, QHeaderView::Stretch);
	//int sizes[] = {110, 110, 0};//, 30, 30, 30, 30, 30, 40, 110, 110};
	//for (int i = 0; i < 11; ++i)
	//{
		/*if (sizes[i] == 0)
		{
			pList->header()->setResizeMode(i, QHeaderView::Stretch);
		}
		else
		{*/
			//if (sizes[i] <= 40) // hack alert!
			//	pList->header()->setResizeMode(i, QHeaderView::Custom);
	//		if(sizes[i])
	//			pList->header()->resizeSection(i, sizes[i]);
		//}
	//}

	pList->setHeaderLabels(headerLabels);

	pList->setSelectionBehavior(QAbstractItemView::SelectRows);
	pList->setSelectionMode(QAbstractItemView::SingleSelection);
	pList->setAlternatingRowColors(true);

	layout->addWidget(pList);

	//---------------------------------------------------
	//  Ok/Cancel Buttons
	//---------------------------------------------------

	QBoxLayout* w5 = new QHBoxLayout;
	vbox->addLayout(w5);

	okB = new QPushButton(tr("Ok"), this);
	okB->setDefault(true);
	QPushButton* cancelB = new QPushButton(tr("Cancel"), this);
	okB->setFixedWidth(80);
	okB->setEnabled(false);
	cancelB->setFixedWidth(80);
	w5->addWidget(okB);
	w5->addSpacing(12);
	w5->addWidget(cancelB);

	//QGroupBox* plugSelGroup = new QGroupBox;
	//plugSelGroup->setTitle("Show plugs:");
	//QVBoxLayout* psl = new QVBoxLayout;
	//plugSelGroup->setLayout(psl);

	QButtonGroup* plugSel = new QButtonGroup(this);
	onlySM = new QRadioButton;
	onlySM->setText(tr("Mono and Stereo"));
	onlySM->setCheckable(true);
	plugSel->addButton(onlySM);
	onlyS = new QRadioButton;
	onlyS->setText(tr("Stereo"));
	onlyS->setCheckable(true);
	plugSel->addButton(onlyS);
	onlyM = new QRadioButton;
	onlyM->setText(tr("Mono"));
	onlyM->setCheckable(true);
	plugSel->addButton(onlyM);
	allPlug = new QRadioButton;
	allPlug->setText(tr("Show All"));
	allPlug->setCheckable(true);
	plugSel->addButton(allPlug);
	plugSel->setExclusive(true);

	switch (selectedPlugType)
	{
		case SEL_SM: onlySM->setChecked(true);
			break;
		case SEL_S: onlyS->setChecked(true);
			break;
		case SEL_M: onlyM->setChecked(true);
			break;
		case SEL_ALL: allPlug->setChecked(true);
			break;
	}

	//plugSelGroup->setToolTip(tr("Select which types of plugins should be visible in the list.<br>"
	//		"Note that using mono plugins on stereo tracks is not a problem, two will be used in parallell.<br>"
	//		"Also beware that the 'all' alternative includes plugins that probably not are usable by OOStudio."));

	//w5->addSpacing(12);
	//w5->addWidget(plugSelGroup);
	//w5->addSpacing(12);

	QLabel *sortLabel = new QLabel;
	sortLabel->setText(tr("Search :"));
	sortLabel->setToolTip(tr("Search in 'Label' and 'Name':"));
	//panelbox->addSpacing(2);

	sortBox = new QComboBox(this);
	sortBox->setEditable(true);
	if (!sortItems.empty())
		sortBox->addItems(sortItems);

	sortBox->setMinimumSize(100, 10);
	//panelbox->addSpacing(12);

	m_cmbType = new QComboBox(this);
	m_cmbType->addItem("LADSPA");
	m_cmbType->addItem("LV2");
        m_cmbType->addItem("VST");
	m_cmbType->setCurrentIndex(1);
	panelbox->addWidget(m_cmbType);
	panelbox->addWidget(sortLabel);
	panelbox->addWidget(sortBox);
	panelbox->addWidget(onlySM);
	panelbox->addWidget(onlyS);
	panelbox->addWidget(onlyM);
	panelbox->addWidget(allPlug);

	//panelbox->addWidget(plugSelGroup);
	panelbox->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

	/*if (!sortBox->currentText().isEmpty())
		fillPlugs(sortBox->currentText());
	else
		fillPlugs(selectedPlugType);*/

	connect(pList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(accept()));
	connect(pList, SIGNAL(itemClicked(QTreeWidgetItem*, int)), SLOT(enableOkB()));
	connect(cancelB, SIGNAL(clicked()), SLOT(reject()));
	connect(okB, SIGNAL(clicked()), SLOT(accept()));
	connect(plugSel, SIGNAL(buttonClicked(QAbstractButton*)), SLOT(fillPlugs(QAbstractButton*)));
	connect(sortBox, SIGNAL(editTextChanged(const QString&)), SLOT(fillPlugs(const QString&)));
	connect(m_cmbType, SIGNAL(currentIndexChanged(int)), SLOT(typeChanged(int)));
	sortBox->setFocus();
	//resize(800, 600);
    
    fillPlugs(selectedPlugType);
}

void PluginDialog::showEvent(QShowEvent*)
{
	QRect geo = tconfig().get_property("PluginDialog", "geometry", QRect(0,0,800, 600)).toRect();
	int type = tconfig().get_property("PluginDialog", "plugin_type", 1).toInt();
	selectedPlugType = tconfig().get_property("PluginDialog", "channel_type", 0).toInt();
	m_cmbType->setCurrentIndex(type);
	setGeometry(geo);
	//resize(size);
}

void PluginDialog::closeEvent(QCloseEvent*)
{
	tconfig().set_property("PluginDialog", "geometry", geometry());
	tconfig().set_property("PluginDialog", "plugin_type", m_display_type);
	tconfig().set_property("PluginDialog", "channel_type", selectedPlugType);
	tconfig().save();
}

void PluginDialog::hideEvent(QHideEvent* e)
{
	if(!e->spontaneous())
	{
		tconfig().set_property("PluginDialog", "geometry", geometry());
		tconfig().set_property("PluginDialog", "plugin_type", m_cmbType->currentIndex());
		tconfig().set_property("PluginDialog", "channel_type", selectedPlugType);
		tconfig().save();
	}
}

//---------------------------------------------------------
//   enableOkB
//---------------------------------------------------------

void PluginDialog::enableOkB()
{
	okB->setEnabled(true);
}

//---------------------------------------------------------
//   value
//---------------------------------------------------------

PluginI* PluginDialog::value()/*{{{*/
{
	QTreeWidgetItem* item = pList->currentItem();
	if (item)
		return plugins.find(item->data(0, Qt::UserRole).toString(), item->text(1));
	printf("plugin not found\n");
	return 0;
}/*}}}*/

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void PluginDialog::accept()/*{{{*/
{
	if (!sortBox->currentText().isEmpty())
	{
		foreach(QString item, sortItems)
		if (item == sortBox->currentText())
		{
			QDialog::accept();
			return;
		}
		sortItems.push_front(sortBox->currentText());
	}
	QDialog::accept();
}/*}}}*/

void PluginDialog::typeChanged(int index)
{
    if (index == 0)
        m_display_type = PLUGIN_LADSPA;
    else if (index == 1)
        m_display_type = PLUGIN_LV2;
    else if (index == 2)
        m_display_type = PLUGIN_VST;

	if (!sortBox->currentText().isEmpty())
		fillPlugs(sortBox->currentText());
	else
		fillPlugs(selectedPlugType);
}

//---------------------------------------------------------
//    fillPlugs
//---------------------------------------------------------

void PluginDialog::fillPlugs(QAbstractButton* ab)/*{{{*/
{
	if (ab == allPlug)
		fillPlugs(SEL_ALL);
	else if (ab == onlyM)
		fillPlugs(SEL_M);
	else if (ab == onlyS)
		fillPlugs(SEL_S);
	else if (ab == onlySM)
		fillPlugs(SEL_SM);
}/*}}}*/

void PluginDialog::fillPlugs(int nbr)/*{{{*/
{
	pList->clear();
    QString stringValue = sortBox->currentText().toLower();
	for (iPlugin i = plugins.begin(); i != plugins.end(); ++i)
	{
        if ((i->hints() & PLUGIN_IS_FX) == 0)
            continue;

		int ai = i->getAudioInputCount();
		int ao = i->getAudioOutputCount();
		//int ci = i->controlInPorts();
		//int co = i->controlOutPorts();

        //if (ai != ao)
            // we can only safely process if inputs match outputs (of fx, not synth plugins)
            //continue;

		bool addFlag = false;
        bool addFlagString = false;
		bool stereo = false;
		if ((ai == 1 || ai == 2 || (m_trackType == Track::AUDIO_BUSS && ai == 3)) && ao == 2)
			stereo = true;
        else if (ai == 1 && ao == 1)
			stereo = false;
        
        if (i->label().toLower().contains(stringValue))
			addFlagString = true;
		else if (i->name().toLower().contains(stringValue))
			addFlagString = true;

		switch (nbr)
		{
			case SEL_SM: // stereo & mono
				if ((ai == 1 || ai == 2 || (m_trackType == Track::AUDIO_BUSS && ai == 3)) && (ao == 1 || ao == 2))
				{
					addFlag = true;
				}
				break;
			case SEL_S: // stereo
				if ((ai == 1 || ai == 2 || (m_trackType == Track::AUDIO_BUSS && ai == 3)) && ao == 2)
				{
					addFlag = true;
				}
				break;
			case SEL_M: // mono
				if (ai == 1 && ao == 1)
				{
					addFlag = true;
				}
				break;
			case SEL_ALL: // all
				addFlag = true;
				break;
		}
		if(m_display_type != i->type())
			addFlag = false;
		if (addFlag && addFlagString)
		{
			QTreeWidgetItem* item = new QTreeWidgetItem;
			item->setText(0, stereo ? "True" : "False");
			item->setData(0, Qt::UserRole, i->filename(false));
			item->setText(1, i->label());
			item->setText(2, i->name());
			QString tip(i->name());
			tip.append("\n  by ").append(i->maker());
			item->setData(2, Qt::ToolTipRole, tip);
			/*item->setText(3, QString().setNum(ai));
			item->setText(4, QString().setNum(ao));
			item->setText(5, QString().setNum(ci));
			item->setText(6, QString().setNum(co));
			item->setText(7, QString().setNum(i->inPlaceCapable()));
			item->setText(8, QString().setNum(i->id()));
			item->setText(9, i->maker());
			item->setText(10, i->copyright());*/
			pList->addTopLevelItem(item);
		}

	}
	selectedPlugType = nbr;
}/*}}}*/

void PluginDialog::fillPlugs(const QString &sortValue)/*{{{*/
{
	pList->clear();
	for (iPlugin i = plugins.begin(); i != plugins.end(); ++i)
	{
		int ai = i->getAudioInputCount();
		int ao = i->getAudioOutputCount();
		//int ci = i->controlInPorts();
		//int co = i->controlOutPorts();

        //if (ai != ao)
            // we can only safely process if inputs match outputs (of fx, not synth plugins)
        //    continue;

		bool addFlag = false;
		bool stereo = false;
		if ((ai == 1 || ai == 2 || (m_trackType == Track::AUDIO_BUSS && ai == 3)) && ao == 2)
			stereo = true;
		else if(ai == 1 && ao == 1)
			stereo = false;

		if (i->label().toLower().contains(sortValue.toLower()))
			addFlag = true;
		else if (i->name().toLower().contains(sortValue.toLower()))
			addFlag = true;
		if(m_display_type != i->type())
			addFlag = false;
		if (addFlag)
		{
			QTreeWidgetItem* item = new QTreeWidgetItem;
			item->setText(0, stereo ? "True" : "False"); //i->lib()
			item->setData(0, Qt::UserRole, i->filename(false));
			item->setText(1, i->label());
			item->setText(2, i->name());
			QString tip(i->name());
			tip.append("\n by ").append(i->maker());
			item->setData(2, Qt::ToolTipRole, tip);
			/*item->setText(3, QString().setNum(ai));
			item->setText(4, QString().setNum(ao));
			item->setText(5, QString().setNum(ci));
			item->setText(6, QString().setNum(co));
			item->setText(7, QString().setNum(i->inPlaceCapable()));
			item->setText(8, QString().setNum(i->id()));
			item->setText(9, i->maker());
			item->setText(10, i->copyright());*/
			pList->addTopLevelItem(item);
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   getPlugin
//---------------------------------------------------------

PluginI* PluginDialog::getPlugin(int type, QWidget* parent)/*{{{*/
{
	PluginDialog* dialog = new PluginDialog(type, parent);
	if (dialog->exec())
		return dialog->value();
	return 0;
}/*}}}*/

