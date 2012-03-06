//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: masteredit.cpp,v 1.4.2.5 2009/07/01 22:14:56 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "awl/sigedit.h"

#include "masteredit.h"
#include "mtscale.h"
#include "hitscale.h"
#include "sigscale.h"
#include "scrollscale.h"
#include "poslabel.h"
#include "master.h"
#include "utils.h"
#include "song.h"
#include "tscale.h"
#include "tempolabel.h"
#include "xml.h"
#include "lcombo.h"
#include "doublelabel.h"
#include "shortcuts.h"
///#include "sigedit.h"
#include "globals.h"
#include "traverso_shared/TConfig.h"
#include "icons.h"

#include <values.h>

#include <QActionGroup>
#include <QCloseEvent>
#include <QGridLayout>
#include <QLabel>
#include <QToolBar>
#include <QToolButton>
#include <QSize>
#include <QPoint>

int MasterEdit::_rasterInit = 0;

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MasterEdit::songChanged(int type)
{
	if (type & SC_TEMPO)
	{
		int tempo = tempomap.tempo(song->cpos());
		curTempo->blockSignals(true);
		curTempo->setValue(double(60000000.0 / tempo));

		curTempo->blockSignals(false);
	}
	if (type & SC_SIG)
	{
		int z, n;
		AL::sigmap.timesig(song->cpos(), z, n);
		curSig->blockSignals(true);
		curSig->setValue(AL::TimeSignature(z, n));
		curSig->blockSignals(false);
		sign->redraw();
	}
	if (type & SC_MASTER)
	{
		masterEnableAction->blockSignals(true);
		masterEnableAction->setChecked(song->masterFlag());
		masterEnableAction->blockSignals(false);
	}
}

//---------------------------------------------------------
//   MasterEdit
//---------------------------------------------------------

MasterEdit::MasterEdit()
: AbstractMidiEditor(0, _rasterInit, 0)
{
	setWindowTitle(tr("OOStudio: Tempo Editor"));
	setWindowIcon(*oomIcon);
	mainw->setLayout(mainGrid);
	_raster = 0; // measure
	//setMinimumSize(400, 300);
	//resize(500, 350);

	//---------Pulldown Menu----------------------------
	//      QPopupMenu* file = new QPopupMenu(this);
	//      menuBar()->insertItem("&File", file);

	//---------ToolBar----------------------------------

	QToolBar* info = new QToolBar(tr("Info Tools"));
	addToolBar(Qt::BottomToolBarArea, info);

	editbar = new EditToolBar(this, masterTools);

	info->setFloatable(false);
	info->setMovable(false);

	cursorPos = new PosLabel(0);
	cursorPos->setFixedHeight(22);
	cursorPos->setToolTip(tr("time at cursor position"));
	cursorPos->setObjectName("composerCursor");
	info->addWidget(cursorPos);
	tempo = new TempoLabel(0);
	tempo->setFixedHeight(22);
	tempo->setToolTip(tr("tempo at cursor position"));
	tempo->setObjectName("pitchLabel");
	info->addWidget(tempo);

	const char* rastval[] = {
		QT_TRANSLATE_NOOP("@default", "Off"), "Bar", "1/2", "1/4", "1/8", "1/16"
	};
	rasterLabel = new QComboBox(this);
	rasterLabel->setFocusPolicy(Qt::NoFocus);
	for (int i = 0; i < 6; i++)
		rasterLabel->insertItem(i, tr(rastval[i]));
	rasterLabel->setCurrentIndex(1);
	info->addWidget(rasterLabel);
	connect(rasterLabel, SIGNAL(currentIndexChanged(int)), SLOT(_setRaster(int)));

	QWidget* spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	spacer->setFixedWidth(15);
	info->addWidget(spacer);

	info->addWidget(editbar);

	QWidget* spacer15 = new QWidget();
	spacer15->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	spacer15->setFixedWidth(15);
	info->addWidget(spacer15);

	//---------values for current position---------------
	info->addWidget(new QLabel(tr("CurPos ")));
	curTempo = new TempoEdit(0);
	curSig = new SigEdit(0);
	curSig->setValue(AL::TimeSignature(4, 4));
	curTempo->setToolTip(tr("tempo at current position"));
	curSig->setToolTip(tr("time signature at current position"));
	info->addWidget(curTempo);
	info->addWidget(curSig);
	///connect(curSig, SIGNAL(valueChanged(int,int)), song, SLOT(setSig(int,int)));
	connect(curSig, SIGNAL(valueChanged(const AL::TimeSignature&)), song, SLOT(setSig(const AL::TimeSignature&)));

	///connect(curTempo, SIGNAL(valueChanged(double)), song, SLOT(setTempo(double)));
	connect(curTempo, SIGNAL(tempoChanged(double)), song, SLOT(setTempo(double)));

	//---------------------------------------------------
	//    master
	//---------------------------------------------------

	int xscale = -20;
	int yscale = -500;
	hscroll = new ScrollScale(-100, -2, xscale, song->len(), Qt::Horizontal, mainw);
	vscroll = new ScrollScale(-1000, -100, yscale, 120000, Qt::Vertical, mainw);
	vscroll->setRange(30000, 250000);
	time1 = new MTScale(&_raster, mainw, xscale);
	sign = new SigScale(&_raster, mainw, xscale);

	canvas = new Master(this, mainw, xscale, yscale);

	time2 = new MTScale(&_raster, mainw, xscale);
	tscale = new TScale(mainw, yscale);
	time2->setBarLocator(true);

	//---------------------------------------------------
	//    Rest
	//---------------------------------------------------

	//      QSizeGrip* corner   = new QSizeGrip(mainw);

	mainGrid->setRowStretch(5, 100);
	mainGrid->setColumnStretch(1, 100);

	mainGrid->addWidget(hLine(mainw), 0, 1);
	mainGrid->addWidget(time1, 1, 1);
	mainGrid->addWidget(hLine(mainw), 2, 1);
	mainGrid->addWidget(sign, 3, 1);
	mainGrid->addWidget(hLine(mainw), 4, 1);
	mainGrid->addWidget(canvas, 5, 1);
	mainGrid->addWidget(tscale, 5, 0);
	mainGrid->addWidget(hLine(mainw), 6, 1);
	mainGrid->addWidget(time2, 7, 1);
	mainGrid->addWidget(hscroll, 8, 1);
	mainGrid->addWidget(vscroll, 0, 2, 10, 1);


	connect(editbar, SIGNAL(toolChanged(int)), canvas, SLOT(setTool(int)));
	connect(vscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setYPos(int)));
	connect(vscroll, SIGNAL(scaleChanged(float)), canvas, SLOT(setYMag(float)));

	connect(vscroll, SIGNAL(scrollChanged(int)), tscale, SLOT(setYPos(int)));
	connect(vscroll, SIGNAL(scaleChanged(float)), tscale, SLOT(setYMag(float)));

	connect(hscroll, SIGNAL(scrollChanged(int)), time1, SLOT(setXPos(int)));
	connect(hscroll, SIGNAL(scrollChanged(int)), sign, SLOT(setXPos(int)));
	connect(hscroll, SIGNAL(scrollChanged(int)), canvas, SLOT(setXPos(int)));
	connect(hscroll, SIGNAL(scrollChanged(int)), time2, SLOT(setXPos(int)));

	connect(hscroll, SIGNAL(scaleChanged(float)), time1, SLOT(setXMag(float)));
	connect(hscroll, SIGNAL(scaleChanged(float)), sign, SLOT(setXMag(float)));
	connect(hscroll, SIGNAL(scaleChanged(float)), canvas, SLOT(setXMag(float)));
	connect(hscroll, SIGNAL(scaleChanged(float)), time2, SLOT(setXMag(float)));

	connect(time1, SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));
	connect(time2, SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));

	connect(tscale, SIGNAL(tempoChanged(int)), SLOT(setTempo(int)));
	connect(canvas, SIGNAL(tempoChanged(int)), SLOT(setTempo(int)));
	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	connect(song, SIGNAL(posChanged(int, unsigned, bool)), SLOT(posChanged(int, unsigned, bool)));

	connect(canvas, SIGNAL(followEvent(int)), hscroll, SLOT(setOffset(int)));
	connect(canvas, SIGNAL(timeChanged(unsigned)), SLOT(setTime(unsigned)));
	posChanged(0, song->cpos(), false);
}

//---------------------------------------------------------
//   ~MasterEdit
//---------------------------------------------------------

MasterEdit::~MasterEdit()
{
}


void MasterEdit::showEvent(QShowEvent*)
{
	int hScale = tconfig().get_property("MasterEdit", "hscale", -100).toInt();
	int vScale = tconfig().get_property("MasterEdit", "yscale", -1000).toInt();
	int yPos = tconfig().get_property("MasterEdit", "ypos", 0).toInt();
	hscroll->setMag(hScale);
	vscroll->setMag(vScale);
	vscroll->setPos(yPos);
	QSize s = tconfig().get_property("MasterEdit", "size", QSize(640, 480)).toSize();
	resize(s);
	QPoint p = tconfig().get_property("MasterEdit", "pos", QPoint(0,0)).toPoint();
	int snap = tconfig().get_property("MasterEdit", "snap", 0).toInt();
	int item = 0;
	switch (snap)
	{
		case 1: item = 0;
			break;
		case 0: item = 1;
			break;
		case 768: item = 2;
			break;
		case 384: item = 3;
			break;
		case 192: item = 4;
			break;
		case 96: item = 5;
			break;
	}
	_rasterInit = snap;
	rasterLabel->setCurrentIndex(item);
	move(p);
	canvas->setFocus();
}

//---------------------------------------------------------
//   closeEvent
//---------------------------------------------------------

void MasterEdit::closeEvent(QCloseEvent* e)
{
	tconfig().set_property("MasterEdit", "size", size());
	tconfig().set_property("MasterEdit", "hscale", hscroll->mag());
	tconfig().set_property("MasterEdit", "yscale", vscroll->mag());
	tconfig().set_property("MasterEdit", "ypos", vscroll->pos());
	tconfig().set_property("MasterEdit", "pos", pos());
	tconfig().set_property("MasterEdit", "snap", _raster);
    tconfig().save();
	emit deleted((unsigned long) this);
	e->accept();
}

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void MasterEdit::readStatus(Xml& xml)
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (tag == "midieditor")
					AbstractMidiEditor::readStatus(xml);
				else if (tag == "ypos")
					vscroll->setPos(xml.parseInt());
				else if (tag == "ymag")
				{
					// vscroll->setMag(xml.parseInt());
					int mag = xml.parseInt();
					vscroll->setMag(mag);
				}
				else
					xml.unknown("MasterEdit");
				break;
			case Xml::TagEnd:
				if (tag == "master")
				{
					// raster setzen
					int item = 0;
					switch (_raster)
					{
						case 1: item = 0;
							break;
						case 0: item = 1;
							break;
						case 768: item = 2;
							break;
						case 384: item = 3;
							break;
						case 192: item = 4;
							break;
						case 96: item = 5;
							break;
					}
					_rasterInit = _raster;
					rasterLabel->setCurrentIndex(item);
					return;
				}
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void MasterEdit::writeStatus(int level, Xml& xml) const
{
	xml.tag(level++, "master");
	xml.intTag(level, "ypos", vscroll->pos());
	xml.intTag(level, "ymag", vscroll->mag());
	AbstractMidiEditor::writeStatus(level, xml);
	xml.tag(level, "/master");
}

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void MasterEdit::readConfiguration(Xml& xml)
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (tag == "raster")
					_rasterInit = xml.parseInt();
				else
					xml.unknown("MasterEdit");
				break;
			case Xml::TagEnd:
				if (tag == "masteredit")
					return;
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void MasterEdit::writeConfiguration(int level, Xml& xml)
{
	xml.tag(level++, "masteredit");
	xml.intTag(level, "raster", _rasterInit);
    xml.tag(--level, "/masteredit");
}

//---------------------------------------------------------
//   _setRaster
//---------------------------------------------------------

void MasterEdit::_setRaster(int index)
{
	//printf("MasterEdit::_setRaster(%d)\n", index);
	static int rasterTable[] = {
		1, 0, 768, 384, 192, 96
	};
	_raster = rasterTable[index];
	_rasterInit = _raster;
}

//---------------------------------------------------------
//   posChanged
//---------------------------------------------------------

void MasterEdit::posChanged(int idx, unsigned val, bool)
{
	if (idx == 0)
	{
		int z, n;
		int tempo = tempomap.tempo(val);
		AL::sigmap.timesig(val, z, n);
		curTempo->blockSignals(true);
		curSig->blockSignals(true);

		curTempo->setValue(double(60000000.0 / tempo));
		curSig->setValue(AL::TimeSignature(z, n));

		curTempo->blockSignals(false);
		curSig->blockSignals(false);
	}
}

//---------------------------------------------------------
//   setTime
//---------------------------------------------------------

void MasterEdit::setTime(unsigned tick)
{
	if (tick != MAXINT)
	{
		cursorPos->setValue(tick);
		time1->setPos(3, tick, false);
		time2->setPos(3, tick, false);
	}
}

//---------------------------------------------------------
//   setTempo
//---------------------------------------------------------

void MasterEdit::setTempo(int val)
{
	if (val != -1)
	{
		tempo->setValue(val);
	}
}

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------

void MasterEdit::keyPressEvent(QKeyEvent* event)
{
	int key = event->key();
	if (key == Qt::Key_Escape)
	{
		close();
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_POINTER].key)
	{
		editbar->set(PointerTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_RUBBER].key)
	{
		editbar->set(RubberTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_PENCIL].key)
	{
		editbar->set(PencilTool);
		return;
	}
	else if (key == shortcuts[SHRT_SET_QUANT_1].key)
	{
		//_setRaster(0)
		rasterLabel->setCurrentIndex(0);
	}
	else if (key == shortcuts[SHRT_SET_QUANT_2].key)
	{
		rasterLabel->setCurrentIndex(1);
	}
	else if (key == shortcuts[SHRT_SET_QUANT_3].key)
	{
		rasterLabel->setCurrentIndex(2);
	}
	else if (key == shortcuts[SHRT_SET_QUANT_4].key)
	{
		rasterLabel->setCurrentIndex(3);
	}
	else if (key == shortcuts[SHRT_SET_QUANT_5].key)
	{
		rasterLabel->setCurrentIndex(4);
	}
	else if (key == shortcuts[SHRT_SET_QUANT_6].key)
	{
		rasterLabel->setCurrentIndex(5);
	}
	else if (key == shortcuts[SHRT_TOGGLE_ENABLE].key)
	{
		masterEnableAction->toggle();
	}
	else if(shortcuts[SHRT_UNDO].key)
	{
		undoAction->trigger();
	}
	else if(shortcuts[SHRT_REDO].key)
	{
		redoAction->trigger();
	}
	else
	{
		//emit keyPressExt(e); //redirect keypress events to main app
	}

}
