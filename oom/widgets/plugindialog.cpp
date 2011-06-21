//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: plugin.h,v 1.9.2.13 2009/12/06 01:25:21 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#include <QtGui>
#include "plugin.h"
#ifdef LV2_SUPPORT
#include "lv2_plugin.h"
#endif
#include "config.h"
#include "plugindialog.h"

int PluginDialog::selectedPlugType = 0;
QStringList PluginDialog::sortItems = QStringList();

//---------------------------------------------------------
//   PluginDialog
//    select Plugin dialog
//---------------------------------------------------------

PluginDialog::PluginDialog(QWidget* parent)
: QDialog(parent)
{
	setWindowTitle(tr("OOMidi: select plugin"));
	QVBoxLayout* vbox = new QVBoxLayout(this);

	QHBoxLayout* layout = new QHBoxLayout();
	vbox->addLayout(layout);

	QVBoxLayout* panelbox = new QVBoxLayout();
	layout->addLayout(panelbox);

	pList = new QTreeWidget(this);
	pList->setColumnCount(11);
	pList->setSortingEnabled(true);
	QStringList headerLabels;
	headerLabels << tr("Lib");
	headerLabels << tr("Label");
	headerLabels << tr("Name");
	headerLabels << tr("AI");
	headerLabels << tr("AO");
	headerLabels << tr("CI");
	headerLabels << tr("CO");
	headerLabels << tr("IP");
	headerLabels << tr("id");
	headerLabels << tr("Maker");
	headerLabels << tr("Copyright");

	int sizes[] = {110, 110, 0, 30, 30, 30, 30, 30, 40, 110, 110};
	for (int i = 0; i < 11; ++i)
	{
		/*if (sizes[i] == 0)
		{
			pList->header()->setResizeMode(i, QHeaderView::Stretch);
		}
		else
		{*/
			//if (sizes[i] <= 40) // hack alert!
			//	pList->header()->setResizeMode(i, QHeaderView::Custom);
			if(sizes[i])
				pList->header()->resizeSection(i, sizes[i]);
		//}
	}

	pList->setHeaderLabels(headerLabels);

	pList->setSelectionBehavior(QAbstractItemView::SelectRows);
	pList->setSelectionMode(QAbstractItemView::SingleSelection);
	pList->setAlternatingRowColors(true);

	fillPlugs(selectedPlugType);
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

	QGroupBox* plugSelGroup = new QGroupBox;
	plugSelGroup->setTitle("Show plugs:");
	QVBoxLayout* psl = new QVBoxLayout;
	plugSelGroup->setLayout(psl);

	QButtonGroup* plugSel = new QButtonGroup(plugSelGroup);
	onlySM = new QRadioButton;
	onlySM->setText(tr("Mono and Stereo"));
	onlySM->setCheckable(true);
	plugSel->addButton(onlySM);
	psl->addWidget(onlySM);
	onlyS = new QRadioButton;
	onlyS->setText(tr("Stereo"));
	onlyS->setCheckable(true);
	plugSel->addButton(onlyS);
	psl->addWidget(onlyS);
	onlyM = new QRadioButton;
	onlyM->setText(tr("Mono"));
	onlyM->setCheckable(true);
	plugSel->addButton(onlyM);
	psl->addWidget(onlyM);
	allPlug = new QRadioButton;
	allPlug->setText(tr("Show All"));
	allPlug->setCheckable(true);
	plugSel->addButton(allPlug);
	psl->addWidget(allPlug);
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

	plugSelGroup->setToolTip(tr("Select which types of plugins should be visible in the list.<br>"
			"Note that using mono plugins on stereo tracks is not a problem, two will be used in parallell.<br>"
			"Also beware that the 'all' alternative includes plugins that probably not are usable by OOMidi."));

	//w5->addSpacing(12);
	//w5->addWidget(plugSelGroup);
	//w5->addSpacing(12);

	QLabel *sortLabel = new QLabel;
	sortLabel->setText(tr("Search :"));
	sortLabel->setToolTip(tr("Search in 'Label' and 'Name':"));
	panelbox->addWidget(sortLabel);
	panelbox->addSpacing(2);

	sortBox = new QComboBox(this);
	sortBox->setEditable(true);
	if (!sortItems.empty())
		sortBox->addItems(sortItems);

	sortBox->setMinimumSize(100, 10);
	panelbox->addWidget(sortBox);
	panelbox->addSpacing(12);
	panelbox->addWidget(plugSelGroup);
	panelbox->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

	if (!sortBox->currentText().isEmpty())
		fillPlugs(sortBox->currentText());
	else
		fillPlugs(selectedPlugType);

	connect(pList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(accept()));
	connect(pList, SIGNAL(itemClicked(QTreeWidgetItem*, int)), SLOT(enableOkB()));
	connect(cancelB, SIGNAL(clicked()), SLOT(reject()));
	connect(okB, SIGNAL(clicked()), SLOT(accept()));
	connect(plugSel, SIGNAL(buttonClicked(QAbstractButton*)), SLOT(fillPlugs(QAbstractButton*)));
	connect(sortBox, SIGNAL(editTextChanged(const QString&)), SLOT(fillPlugs(const QString&)));
	sortBox->setFocus();
	resize(800, 600);
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

Plugin* PluginDialog::value()/*{{{*/
{
	QTreeWidgetItem* item = pList->currentItem();
	if (item)
		return plugins.find(item->text(0), item->text(1));
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
	for (iPlugin i = plugins.begin(); i != plugins.end(); ++i)
	{
		int ai = i->inports();
		int ao = i->outports();
		int ci = i->controlInPorts();
		int co = i->controlOutPorts();
		bool addFlag = false;
		switch (nbr)
		{
			case SEL_SM: // stereo & mono
				if ((ai == 1 || ai == 2) && (ao == 1 || ao == 2))
				{
					addFlag = true;
				}
				break;
			case SEL_S: // stereo
				if ((ai == 1 || ai == 2) && ao == 2)
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
		if (addFlag)
		{
			QTreeWidgetItem* item = new QTreeWidgetItem;
			item->setText(0, i->lib());
			item->setText(1, i->label());
			item->setText(2, i->name());
			item->setText(3, QString().setNum(ai));
			item->setText(4, QString().setNum(ao));
			item->setText(5, QString().setNum(ci));
			item->setText(6, QString().setNum(co));
			item->setText(7, QString().setNum(i->inPlaceCapable()));
			item->setText(8, QString().setNum(i->id()));
			item->setText(9, i->maker());
			item->setText(10, i->copyright());
			pList->addTopLevelItem(item);
		}

	}
#if 0//def LV2_SUPPORT
	for(iLV2Plugin i = lv2plugins.begin(); i != lv2plugins.end(); ++i)
	{
		int ai = i->inports();
		int ao = i->outports();
		int ci = i->controlInPorts();
		int co = i->controlOutPorts();
		bool addFlag = false;
		switch (nbr)
		{
			case SEL_SM: // stereo & mono
				if ((ai == 1 || ai == 2) && (ao == 1 || ao == 2))
				{
					addFlag = true;
				}
				break;
			case SEL_S: // stereo
				if ((ai == 1 || ai == 2) && ao == 2)
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
		if (addFlag)
		{
			QTreeWidgetItem* item = new QTreeWidgetItem;
			item->setText(0, i->uri());
			item->setText(1, i->label());
			item->setText(2, i->name());
			item->setText(3, QString().setNum(ai));
			item->setText(4, QString().setNum(ao));
			item->setText(5, QString().setNum(ci));
			item->setText(6, QString().setNum(co));
			item->setText(7, QString().setNum(i->inPlaceCapable()));
			item->setText(8, QString().setNum(i->id()));
			item->setText(9, i->maker());
			item->setText(10, i->copyright());
			pList->addTopLevelItem(item);
		}
	}
#endif
	selectedPlugType = nbr;
}/*}}}*/

void PluginDialog::fillPlugs(const QString &sortValue)/*{{{*/
{
	pList->clear();
	for (iPlugin i = plugins.begin(); i != plugins.end(); ++i)
	{
		int ai = i->inports();
		int ao = i->outports();
		int ci = i->controlInPorts();
		int co = i->controlOutPorts();

		bool addFlag = false;

		if (i->label().toLower().contains(sortValue.toLower()))
			addFlag = true;
		else if (i->name().toLower().contains(sortValue.toLower()))
			addFlag = true;
		if (addFlag)
		{
			QTreeWidgetItem* item = new QTreeWidgetItem;
			item->setText(0, i->lib());
			item->setText(1, i->label());
			item->setText(2, i->name());
			item->setText(3, QString().setNum(ai));
			item->setText(4, QString().setNum(ao));
			item->setText(5, QString().setNum(ci));
			item->setText(6, QString().setNum(co));
			item->setText(7, QString().setNum(i->inPlaceCapable()));
			item->setText(8, QString().setNum(i->id()));
			item->setText(9, i->maker());
			item->setText(10, i->copyright());
			pList->addTopLevelItem(item);
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   getPlugin
//---------------------------------------------------------

Plugin* PluginDialog::getPlugin(QWidget* parent)/*{{{*/
{
	PluginDialog* dialog = new PluginDialog(parent);
	if (dialog->exec())
		return dialog->value();
	return 0;
}/*}}}*/

