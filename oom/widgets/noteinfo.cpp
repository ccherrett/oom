//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: noteinfo.cpp,v 1.4.2.1 2008/08/18 00:15:26 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//=========================================================

#include <QtGui>
#include <QTableWidget>

#include "config.h"
#include "noteinfo.h"
#include "awl/posedit.h"
#include "song.h"
#include "globals.h"
#include "pitchedit.h"
#include "gcombo.h"
#include "traverso_shared/TConfig.h"

static int rasterTable[] = {
	//------                8    4     2
	1, 4, 8, 16, 32, 64, 128, 256, 512, 1024,
	1, 6, 12, 24, 48, 96, 192, 384, 768, 1536,
	1, 9, 18, 36, 72, 144, 288, 576, 1152, 2304
};

static const char* rasterStrings[] ={
	QT_TRANSLATE_NOOP("@default", "Off"), "2pp", "5pp", "64T", "32T", "16T", "8T", "4T", "2T", "1T",
	QT_TRANSLATE_NOOP("@default", "Off"), "3pp", "6pp", "64", "32", "16", "8", "4", "2", "1",
	QT_TRANSLATE_NOOP("@default", "Off"), "4pp", "7pp", "64.", "32.", "16.", "8.", "4.", "2.", "1."
};

static int quantTable[] = {
	1, 16, 32, 64, 128, 256, 512, 1024,
	1, 24, 48, 96, 192, 384, 768, 1536,
	1, 36, 72, 144, 288, 576, 1152, 2304
};

static const char* quantStrings[] = {
	QT_TRANSLATE_NOOP("@default", "Off"), "64T", "32T", "16T", "8T", "4T", "2T", "1T",
	QT_TRANSLATE_NOOP("@default", "Off"), "64", "32", "16", "8", "4", "2", "1",
	QT_TRANSLATE_NOOP("@default", "Off"), "64.", "32.", "16.", "8.", "4.", "2.", "1."
};

//---------------------------------------------------
//    NoteInfo
//---------------------------------------------------

NoteInfo::NoteInfo(QWidget* parent)
: QWidget(parent)
{
	deltaMode = false;

	m_layout = new QVBoxLayout(this);

	selTime = new Awl::PosEdit;
	selTime->setObjectName("Start");
	addTool(tr("Start"), selTime);

	selLen = new QSpinBox();
	selLen->setRange(0, 100000);
	selLen->setSingleStep(1);
	addTool(tr("Len"), selLen);

	selPitch = new PitchEdit;
	addTool(tr("Pitch"), selPitch);

	selVelOn = new QSpinBox();
	selVelOn->setRange(0, 127);
	selVelOn->setSingleStep(1);
	addTool(tr("Velo On"), selVelOn);

	selVelOff = new QSpinBox();
	selVelOff->setRange(0, 127);
	selVelOff->setSingleStep(1);
	addTool(tr("Velo Off"), selVelOff);

	m_renderAlpha = new QSpinBox();
	m_renderAlpha->setRange(0, 255);
	m_renderAlpha->setSingleStep(1);
	int alpha = tconfig().get_property("PerformerEdit", "renderalpha", 50).toInt();
	m_renderAlpha->setValue(alpha);

	addTool(tr("BG Brightness"), m_renderAlpha);

	m_partLines = new QCheckBox(this);
	bool pl = tconfig().get_property("PerformerEdit", "partLines", true).toBool();
	m_partLines->setChecked(pl);
	addTool(tr("Part End Marker"), m_partLines);

	//start tb1 merge/*{{{*/
	//---------------------------------------------------
	//  Raster, Quant.
	//---------------------------------------------------

    rasterLabel = new GridCombo(this);
    quantLabel = new GridCombo(this);

	rlist = new QTableWidget(10, 3);
	rlist->setObjectName("listSnap");
	qlist = new QTableWidget(8, 3);
	qlist->setObjectName("listQuant");
	rlist->verticalHeader()->setDefaultSectionSize(22);
	rlist->horizontalHeader()->setDefaultSectionSize(32);
	rlist->setSelectionMode(QAbstractItemView::SingleSelection);
	rlist->verticalHeader()->hide();
	rlist->horizontalHeader()->hide();
	qlist->verticalHeader()->setDefaultSectionSize(22);
	qlist->horizontalHeader()->setDefaultSectionSize(32);
	qlist->setSelectionMode(QAbstractItemView::SingleSelection);
	qlist->verticalHeader()->hide();
	qlist->horizontalHeader()->hide();

	rlist->setMinimumWidth(96);
	qlist->setMinimumWidth(96);

	rasterLabel->setView(rlist);
	quantLabel->setView(qlist);

	for (int j = 0; j < 3; j++)
		for (int i = 0; i < 10; i++)
			rlist->setItem(i, j, new QTableWidgetItem(tr(rasterStrings[i + j * 10])));
	for (int j = 0; j < 3; j++)
		for (int i = 0; i < 8; i++)
			qlist->setItem(i, j, new QTableWidgetItem(tr(quantStrings[i + j * 8])));

	setRaster(96);
	setQuant(96);

    rasterLabel->setMinimumSize(QSize(80, 22));
    quantLabel->setMinimumSize(QSize(80, 22));

	QHBoxLayout *hbox1 = new QHBoxLayout();
	hbox1->addWidget(new QLabel(tr("Snap")));
	hbox1->addWidget(rasterLabel);
	QHBoxLayout *hbox2 = new QHBoxLayout();
	hbox2->addWidget(new QLabel(tr("Quant.")));
	hbox2->addWidget(quantLabel);
	m_layout->addLayout(hbox1);
	m_layout->addLayout(hbox2);

	//---------------------------------------------------
	//  To Menu
	//---------------------------------------------------
	QComboBox* toList = new QComboBox;
	toList->insertItem(0, tr("All Events"));
	toList->insertItem(CMD_RANGE_LOOP, tr("Looped Ev."));
	toList->insertItem(CMD_RANGE_SELECTED, tr("Selected Ev."));
	toList->insertItem(CMD_RANGE_LOOP | CMD_RANGE_SELECTED, tr("Looped+Sel."));
    toList->setMinimumSize(QSize(80, 22));
	QHBoxLayout *hbox3 = new QHBoxLayout();
	hbox3->addWidget(new QLabel(tr("To")));
	hbox3->addWidget(toList);
	m_layout->addLayout(hbox3);

    connect(rasterLabel, SIGNAL(activated(int)), SLOT(_rasterChanged(int)));
    connect(quantLabel, SIGNAL(activated(int)), SLOT(_quantChanged(int)));
    connect(toList, SIGNAL(activated(int)), SIGNAL(toChanged(int)));
	//end tb1/*}}}*/
	QSpacerItem* vSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	m_layout->addItem(vSpacer);

	connect(selLen, SIGNAL(valueChanged(int)), SLOT(lenChanged(int)));
	connect(selPitch, SIGNAL(valueChanged(int)), SLOT(pitchChanged(int)));
	connect(selVelOn, SIGNAL(valueChanged(int)), SLOT(velOnChanged(int)));
	connect(selVelOff, SIGNAL(valueChanged(int)), SLOT(velOffChanged(int)));
	connect(m_renderAlpha, SIGNAL(valueChanged(int)), SLOT(alphaChanged(int)));
	connect(selTime, SIGNAL(valueChanged(const Pos&)), SLOT(timeChanged(const Pos&)));
	connect(m_partLines, SIGNAL(toggled(bool)), SLOT(partLinesChanged(bool)));
}

void NoteInfo::addTool(QString label, QWidget *tool)
{
	QHBoxLayout *box = new QHBoxLayout();
	box->addWidget(new QLabel(label));
	box->addWidget(tool);
	m_layout->addLayout(box);
}

//---------------------------------------------------------
// alphaChanged
//---------------------------------------------------------
void NoteInfo::alphaChanged(int alpha)
{
	tconfig().set_property("PerformerEdit", "renderalpha", alpha);
	tconfig().save();
	emit alphaChanged();
}
void NoteInfo::partLinesChanged(bool checked)
{
	tconfig().set_property("PerformerEdit", "partLines", m_partLines->isChecked());
	tconfig().save();
	emit enablePartLines(checked);
}

void NoteInfo::enableTools(bool state)
{
	selTime->setEnabled(state);
	selLen->setEnabled(state);
	selPitch->setEnabled(state);
	selVelOn->setEnabled(state);
	selVelOff->setEnabled(state);
}

//---------------------------------------------------------
//   setDeltaMode
//---------------------------------------------------------

void NoteInfo::setDeltaMode(bool val)
{
	deltaMode = val;
	selPitch->setDeltaMode(val);
	if (val)
	{
		selLen->setRange(-100000, 100000);
		selVelOn->setRange(-127, 127);
		selVelOff->setRange(-127, 127);
	}
	else
	{
		selLen->setRange(0, 100000);
		selVelOn->setRange(0, 127);
		selVelOff->setRange(0, 127);
	}
}

//---------------------------------------------------------
//   lenChanged
//---------------------------------------------------------

void NoteInfo::lenChanged(int val)
{
	if (!signalsBlocked())
		emit valueChanged(VAL_LEN, val);
}

//---------------------------------------------------------
//   velOnChanged
//---------------------------------------------------------

void NoteInfo::velOnChanged(int val)
{
	if (!signalsBlocked())
		emit valueChanged(VAL_VELON, val);
}

//---------------------------------------------------------
//   velOffChanged
//---------------------------------------------------------

void NoteInfo::velOffChanged(int val)
{
	if (!signalsBlocked())
		emit valueChanged(VAL_VELOFF, val);
}

//---------------------------------------------------------
//   pitchChanged
//---------------------------------------------------------

void NoteInfo::pitchChanged(int val)
{
	if (!signalsBlocked())
		emit valueChanged(VAL_PITCH, val);
}

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void NoteInfo::setValue(ValType type, int val)
{
	blockSignals(true);
	switch (type)
	{
		case VAL_TIME:
			selTime->setValue(val);
			break;
		case VAL_LEN:
			selLen->setValue(val);
			break;
		case VAL_VELON:
			selVelOn->setValue(val);
			break;
		case VAL_VELOFF:
			selVelOff->setValue(val);
			break;
		case VAL_PITCH:
			selPitch->setValue(val);
			break;
	}
	blockSignals(false);
}

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void NoteInfo::setValues(unsigned tick, int val2, int val3, int val4,
		int val5)
{
	blockSignals(true);
	if (selTime->pos().tick() != tick)
		selTime->setValue(tick);
	if (selLen->value() != val2)
		selLen->setValue(val2);
	if (selPitch->value() != val3)
		selPitch->setValue(val3);
	if (selVelOn->value() != val4)
		selVelOn->setValue(val4);
	if (selVelOff->value() != val5)
		selVelOff->setValue(val5);
	blockSignals(false);
}

//---------------------------------------------------------
//   timeChanged
//---------------------------------------------------------

void NoteInfo::timeChanged(const Pos& pos)
{
	if (!signalsBlocked())
		emit valueChanged(VAL_TIME, pos.tick());
}

//---------------------------------------------------------/*{{{*/
//   rasterChanged
//---------------------------------------------------------

void NoteInfo::_rasterChanged(int /*i*/)
{
	emit rasterChanged(rasterTable[rlist->currentRow() + rlist->currentColumn() * 10]);
}

//---------------------------------------------------------
//   quantChanged
//---------------------------------------------------------

void NoteInfo::_quantChanged(int /*i*/)
{
	emit quantChanged(quantTable[qlist->currentRow() + qlist->currentColumn() * 8]);
}

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void NoteInfo::setRaster(int val)
{
	for (unsigned i = 0; i < sizeof (rasterTable) / sizeof (*rasterTable); i++)
	{
		if (val == rasterTable[i])
		{
			rasterLabel->setCurrentIndex(i);
			return;
		}
	}
	printf("setRaster(%d) not defined\n", val);
	rasterLabel->setCurrentIndex(0);
}

//---------------------------------------------------------
//   setQuant
//---------------------------------------------------------

void NoteInfo::setQuant(int val)
{
	for (unsigned i = 0; i < sizeof (quantTable) / sizeof (*quantTable); i++)
	{
		if (val == quantTable[i])
		{
			quantLabel->setCurrentIndex(i);
			return;
		}
	}
	printf("setQuant(%d) not defined\n", val);
	quantLabel->setCurrentIndex(0);
}/*}}}*/
