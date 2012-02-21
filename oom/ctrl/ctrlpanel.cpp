//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: ctrlpanel.cpp,v 1.10.2.9 2009/06/14 05:24:45 terminator356 Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <list>

#include "ctrlpanel.h"
#include "ctrlcanvas.h"

#include <QMenu>
#include <QToolButton>
#include <QSizePolicy>
#include <QHBoxLayout>
#include <QTimer>
#include <QVBoxLayout>
#include <QString>
#include <QComboBox>

#include <math.h>

#include "globals.h"
#include "midictrl.h"
#include "instruments/minstrument.h"
#include "midiport.h"
#include "xml.h"
#include "icons.h"
#include "event.h"
#include "AbstractMidiEditor.h"
#include "track.h"
#include "part.h"
#include "midiedit/drummap.h"
#include "gconfig.h"
#include "song.h"
#include "knob.h"
#include "doublelabel.h"
#include "midi.h"
#include "audio.h"
#include "midimonitor.h"

//---------------------------------------------------------
//   CtrlPanel
//---------------------------------------------------------

CtrlPanel::CtrlPanel(QWidget* parent, AbstractMidiEditor* e, const char* name)
: QWidget(parent)
{
	setObjectName(name);
	inHeartBeat = true;
	editor = e;
	m_collapsed = false;
	QVBoxLayout* mainLayout = new QVBoxLayout;
	QHBoxLayout* buttonBox = new QHBoxLayout;
	buttonBox->setSpacing(0);
	mainLayout->addLayout(buttonBox);
	//mainLayout->addStretch();
	//QHBoxLayout* knobBox = new QHBoxLayout;
	//QHBoxLayout* labelBox = new QHBoxLayout;
	//mainLayout->addLayout(knobBox);
	//mainLayout->addLayout(labelBox);
	//mainLayout->addStretch();
	mainLayout->setContentsMargins(0, 0, 0, 0);
	buttonBox->setContentsMargins(0, 0, 0, 0);
	//knobBox->setContentsMargins(0, 0, 0, 0);
	//labelBox->setContentsMargins(0, 0, 0, 0);

	btnCollapse = new QToolButton();
	btnCollapse->setCheckable(true);
	btnCollapse->setFixedHeight(20);
	btnCollapse->setAutoRaise(true);
	btnCollapse->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	btnCollapse->setToolTip(tr("Toggle Collapsed"));
	btnCollapse->setIcon(QIcon(*collapseIconSet3));
	btnCollapse->setIconSize(QSize(22,22));
	connect(btnCollapse, SIGNAL(toggled(bool)), SLOT(toggleCollapsed(bool)));

	// destroy button
	QToolButton* btnClose = new QToolButton();
	btnClose->setAutoRaise(true);
	btnClose->setFixedHeight(20);
	btnClose->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	btnClose->setToolTip(tr("Remove Controller Lane"));
	btnClose->setIcon(QIcon(*garbageIconSet3));
	btnClose->setIconSize(QSize(22,22));
	// Cursor Position
	connect(btnClose, SIGNAL(clicked()), SIGNAL(destroyPanel()));

	_track = 0;
	_ctrl = 0;
	_val = CTRL_VAL_UNKNOWN;
    _dnum = CTRL_VELOCITY;

	_knob = new Knob;
	//_knob->setFixedWidth(25);
	//_knob->setFixedHeight(25);
	_knob->setToolTip(tr("manual adjust"));
	_knob->setRange(0.0, 127.0, 1.0);
	_knob->setValue(0.0);
	_knob->setEnabled(false);
	_knob->hide();
	_knob->setAltFaceColor(Qt::red);

	_dl = new DoubleLabel(-1.0, 0.0, +127.0);
	_dl->setPrecision(0);
	_dl->setToolTip(tr("double click on/off"));
	_dl->setSpecialText(tr("off"));
	_dl->setObjectName("ctrlLabelBox");
	_dl->setFont(config.fonts[1]);
	_dl->setBackgroundRole(QPalette::Mid);
	_dl->setFrame(false);
	_dl->setFixedWidth(36);
	_dl->setFixedHeight(15);
	_dl->setEnabled(false);
	_dl->hide();

	connect(_knob, SIGNAL(sliderMoved(double, int)), SLOT(ctrlChanged(double)));
	connect(_knob, SIGNAL(sliderRightClicked(const QPoint&, int)), SLOT(ctrlRightClicked(const QPoint&, int)));
	//connect(_knob, SIGNAL(sliderReleased(int)), SLOT(ctrlReleased(int)));
	connect(_dl, SIGNAL(valueChanged(double, int)), SLOT(ctrlChanged(double)));
	connect(_dl, SIGNAL(doubleClicked(int)), SLOT(labelDoubleClicked()));
    connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));

	buttonBox->addStretch();
	buttonBox->addWidget(btnCollapse);
	buttonBox->addWidget(btnClose);
	buttonBox->addStretch();

	//knobBox->addStretch();
	mainLayout->addWidget(_knob, 0, Qt::AlignHCenter);
	//knobBox->addStretch();
	
	//labelBox->addStretch();
	mainLayout->addWidget(_dl, 0, Qt::AlignHCenter);
	//labelBox->addStretch();
	
	mainLayout->addStretch();
    setLayout(mainLayout);

    inHeartBeat = false;
}

//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void CtrlPanel::heartBeat()/*{{{*/
{
	inHeartBeat = true;

	if (_track && _ctrl && _dnum != -1)
	{
		//if(_dnum != CTRL_VELOCITY && _dnum != CTRL_PROGRAM)
		if (_dnum != CTRL_VELOCITY)
		{
			int outport;
			int chan;
			int cdi = editor->curDrumInstrument();
			if (_track->type() == Track::DRUM && ((_ctrl->num() & 0xff) == 0xff) && cdi != -1)
			{
				outport = drumMap[cdi].port;
				chan = drumMap[cdi].channel;
			}
			else
			{
				outport = _track->outPort();
				chan = _track->outChannel();
			}
			MidiPort* mp = &midiPorts[outport];

			int v = mp->hwCtrlState(chan, _dnum);
			if (v == CTRL_VAL_UNKNOWN)
			{
				// DoubleLabel ignores the value if already set...
				_dl->setValue(_dl->off() - 1.0);
				_val = CTRL_VAL_UNKNOWN;
				v = mp->lastValidHWCtrlState(chan, _dnum);
				if (v != CTRL_VAL_UNKNOWN && ((_dnum != CTRL_PROGRAM) || ((v & 0xff) != 0xff)))
				{
					if (_dnum == CTRL_PROGRAM)
						v = (v & 0x7f) + 1;
					else
						// Auto bias...
						v -= _ctrl->bias();
					if (double(v) != _knob->value())
					{
						// Added by Tim. p3.3.6
						//printf("CtrlPanel::heartBeat setting knob\n");

						_knob->setValue(double(v));
					}
				}
			}
			else
				if (v != _val)
			{
				_val = v;
				if (v == CTRL_VAL_UNKNOWN || ((_dnum == CTRL_PROGRAM) && ((v & 0xff) == 0xff)))
				{
					// DoubleLabel ignores the value if already set...
					//_dl->setValue(double(_ctrl->minVal() - 1));
					_dl->setValue(_dl->off() - 1.0);
				}
				else
				{
					if (_dnum == CTRL_PROGRAM)
						v = (v & 0x7f) + 1;
					else
						// Auto bias...
						v -= _ctrl->bias();

					// Added by Tim. p3.3.6
					//printf("CtrlPanel::heartBeat setting knob and label\n");

					_knob->setValue(double(v));
					_dl->setValue(double(v));
				}
			}
		}
	}

	inHeartBeat = false;
}/*}}}*/

void CtrlPanel::toggleCollapsed(bool val)
{
	btnCollapse->blockSignals(true);
	btnCollapse->setChecked(val);
	btnCollapse->blockSignals(false);

	emit collapsed(val);
	
	m_collapsed = val;
	if(val)
	{
		_knob->setVisible(!val);
		_dl->setVisible(!val);
	}
	else
	{
		if (_dnum != CTRL_VELOCITY)
		{
			_knob->setVisible(!val);
			_dl->setVisible(!val);
		}
	}
}

//---------------------------------------------------------
//   labelDoubleClicked
//---------------------------------------------------------

void CtrlPanel::labelDoubleClicked()/*{{{*/
{
	if (!_track || !_ctrl || _dnum == -1)
		return;

	int outport;
	int chan;
	int cdi = editor->curDrumInstrument();
	if (_track->type() == Track::DRUM && ((_ctrl->num() & 0xff) == 0xff) && cdi != -1)
	{
		outport = drumMap[cdi].port;
		chan = drumMap[cdi].channel;
	}
	else
	{
		outport = _track->outPort();
		chan = _track->outChannel();
	}
	MidiPort* mp = &midiPorts[outport];
	int lastv = mp->lastValidHWCtrlState(chan, _dnum);

	int curv = mp->hwCtrlState(chan, _dnum);

	if (_dnum == CTRL_PROGRAM)
	{
		if (curv == CTRL_VAL_UNKNOWN || ((curv & 0xffffff) == 0xffffff))
		{
			// If no value has ever been set yet, use the current knob value
			//  (or the controller's initial value?) to 'turn on' the controller.
			if (lastv == CTRL_VAL_UNKNOWN || ((lastv & 0xffffff) == 0xffffff))
			{
				//int kiv = _ctrl->initVal());
				int kiv = lrint(_knob->value());
				--kiv;
				kiv &= 0x7f;
				kiv |= 0xffff00;
				//MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, _dnum, kiv);
				MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, _dnum, kiv, (Track*)_track);
				audio->msgPlayMidiEvent(&ev);
			}
			else
			{
				//MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, _dnum, lastv);
				MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, _dnum, lastv, (Track*)_track);
				audio->msgPlayMidiEvent(&ev);
			}
		}
		else
		{
			//if((curv & 0xffff00) == 0xffff00)
			//{
			////if(mp->hwCtrlState(chan, _dnum) != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, chan, _dnum, CTRL_VAL_UNKNOWN);
			//}
			//else
			//{
			//  MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, _dnum, (curv & 0xffff00) | 0xff);
			//  audio->msgPlayMidiEvent(&ev);
			//}
		}
	}
	else
	{
		if (curv == CTRL_VAL_UNKNOWN)
		{
			// If no value has ever been set yet, use the current knob value
			//  (or the controller's initial value?) to 'turn on' the controller.
			if (lastv == CTRL_VAL_UNKNOWN)
			{
				int kiv = lrint(_knob->value());
				if (kiv < _ctrl->minVal())
					kiv = _ctrl->minVal();
				if (kiv > _ctrl->maxVal())
					kiv = _ctrl->maxVal();
				kiv += _ctrl->bias();
				MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, _dnum, kiv, (Track*)_track);
				audio->msgPlayMidiEvent(&ev);
				midiMonitor->msgSendMidiOutputEvent((Track*)_track, _dnum, kiv);
			}
			else
			{
				MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, _dnum, lastv, (Track*)_track);
				audio->msgPlayMidiEvent(&ev);
				midiMonitor->msgSendMidiOutputEvent((Track*)_track, _dnum, lastv);
			}
		}
		else
		{
			audio->msgSetHwCtrlState(mp, chan, _dnum, CTRL_VAL_UNKNOWN);
		}
	}
	song->update(SC_MIDI_CONTROLLER);
}/*}}}*/

//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

void CtrlPanel::ctrlChanged(double val)/*{{{*/
{
	if (inHeartBeat)
		return;
	if (!_track || !_ctrl || _dnum == -1)
		return;

	int ival = lrint(val);

	int outport;
	int chan;
	int cdi = editor->curDrumInstrument();
	if (_track->type() == Track::DRUM && ((_ctrl->num() & 0xff) == 0xff) && cdi != -1)
	{
		outport = drumMap[cdi].port;
		chan = drumMap[cdi].channel;
	}
	else
	{
		outport = _track->outPort();
		chan = _track->outChannel();
	}
	MidiPort* mp = &midiPorts[outport];
	int curval = mp->hwCtrlState(chan, _dnum);

	if (_dnum == CTRL_PROGRAM)
	{
		--ival;
		ival &= 0x7f;

		if (curval == CTRL_VAL_UNKNOWN)
			ival |= 0xffff00;
		else
			ival |= (curval & 0xffff00);
		MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, _dnum, ival, (Track*)_track);
		audio->msgPlayMidiEvent(&ev);
	}
	else
		// Shouldn't happen, but...
		if ((ival < _ctrl->minVal()) || (ival > _ctrl->maxVal()))
	{
		if (curval != CTRL_VAL_UNKNOWN)
			audio->msgSetHwCtrlState(mp, chan, _dnum, CTRL_VAL_UNKNOWN);
	}
	else
	{
		// Auto bias...
		ival += _ctrl->bias();

		//MidiPlayEvent ev(song->cpos(), outport, chan, ME_CONTROLLER, _dnum, ival);
		MidiPlayEvent ev(0, outport, chan, ME_CONTROLLER, _dnum, ival, (Track*)_track);
		audio->msgPlayMidiEvent(&ev);
		midiMonitor->msgSendMidiOutputEvent((Track*)_track, _dnum, ival);
	}
	song->update(SC_MIDI_CONTROLLER);
}/*}}}*/

//---------------------------------------------------------
//   setHWController
//---------------------------------------------------------

void CtrlPanel::setHWController(MidiTrack* t, MidiController* ctrl)/*{{{*/
{
	inHeartBeat = true;

	_track = t;
	_ctrl = ctrl;

	if (!_track || !_ctrl)
	{
		_knob->setEnabled(false);
		_dl->setEnabled(false);
		_knob->hide();
		_dl->hide();
		inHeartBeat = false;
		return;
	}

	MidiPort* mp;
	int ch;
	int cdi = editor->curDrumInstrument();
	_dnum = _ctrl->num();
	if (_track->type() == Track::DRUM && ((_dnum & 0xff) == 0xff) && cdi != -1)
	{
		_dnum = (_dnum & ~0xff) | drumMap[cdi].anote;
		mp = &midiPorts[drumMap[cdi].port];
		ch = drumMap[cdi].channel;
	}
	else
	{
		mp = &midiPorts[_track->outPort()];
		ch = _track->outChannel();
	}

	//if(_dnum == CTRL_VELOCITY || _dnum == CTRL_PROGRAM)
	if (_dnum == CTRL_VELOCITY)
	{
		_knob->setEnabled(false);
		_dl->setEnabled(false);
		_knob->hide();
		_dl->hide();
	}
	else
	{
		_knob->setEnabled(true);
		_dl->setEnabled(true);
		double dlv;
		int mn;
		int mx;
		int v;
		if (_dnum == CTRL_PROGRAM)
		{
			mn = 1;
			mx = 128;
			v = mp->hwCtrlState(ch, _dnum);
			_val = v;
			_knob->setRange(double(mn), double(mx), 1.0);
			_dl->setRange(double(mn), double(mx));
			//_dl->setOff(double(mn - 1));
			if (v == CTRL_VAL_UNKNOWN || ((v & 0xffffff) == 0xffffff))
			{
				int lastv = mp->lastValidHWCtrlState(ch, _dnum);
				if (lastv == CTRL_VAL_UNKNOWN || ((lastv & 0xffffff) == 0xffffff))
				{
					int initv = _ctrl->initVal();
					if (initv == CTRL_VAL_UNKNOWN || ((initv & 0xffffff) == 0xffffff))
						v = 1;
					else
						v = (initv + 1) & 0xff;
				}
				else
					v = (lastv + 1) & 0xff;

				if (v > 128)
					v = 128;
				//dlv = mn - 1;
				dlv = _dl->off() - 1.0;
			}
			else
			{
				v = (v + 1) & 0xff;
				if (v > 128)
					v = 128;
				dlv = double(v);
			}
		}
		else
		{
			mn = _ctrl->minVal();
			mx = _ctrl->maxVal();
			v = mp->hwCtrlState(ch, _dnum);
			_val = v;
			_knob->setRange(double(mn), double(mx), 1.0);
			_dl->setRange(double(mn), double(mx));
			//_dl->setOff(double(mn - 1));
			if (v == CTRL_VAL_UNKNOWN)
			{
				int lastv = mp->lastValidHWCtrlState(ch, _dnum);
				if (lastv == CTRL_VAL_UNKNOWN)
				{
					if (_ctrl->initVal() == CTRL_VAL_UNKNOWN)
						v = 0;
					else
						v = _ctrl->initVal();
				}
				else
					v = lastv - _ctrl->bias();
				//dlv = mn - 1;
				dlv = _dl->off() - 1.0;
			}
			else
			{
				// Auto bias...
				v -= _ctrl->bias();
				dlv = double(v);
			}
		}
		_knob->setValue(double(v));
		_dl->setValue(dlv);

		if(!m_collapsed)
		{
			_knob->show();
			_dl->show();
			// Incomplete drawing sometimes. Update fixes it.
			_knob->update();
			_dl->update();
		}
	}

	inHeartBeat = false;
}/*}}}*/

//---------------------------------------------------------
//   setHeight
//---------------------------------------------------------

void CtrlPanel::setHeight(int h)
{
	setFixedHeight(h);
}

bool CtrlPanel::ctrlSetTypeByName(QString s)/*{{{*/
{
	bool rv = false;
	if(s.isEmpty())
		return rv;

	if (s == "Velocity")
	{
		rv = true;
		emit controllerChanged(CTRL_VELOCITY);
	}
	else if(s == "Modulation")
	{
		rv = true;
		emit controllerChanged(CTRL_MODULATION);
	}
	else
	{
		Part* part = editor->curCanvasPart();
		MidiTrack* track = (MidiTrack*) (part->track());
		MidiPort* port = &midiPorts[track->outPort()];

		MidiCtrlValListList* cll = port->controller();

		iMidiCtrlValList i = cll->begin();
		for (; i != cll->end(); ++i)
		{
			MidiCtrlValList* cl = i->second;
			MidiController* c = port->midiController(cl->num());
			if (c->name() == s)
			{
				emit controllerChanged(c->num());
				rv = true;
				break;
			}
		}
		if (i == cll->end())
		{
			printf("CtrlPanel: controller %s not found!\n", s.toLatin1().constData());
		}
	}
	return rv;
}/*}}}*/

//---------------------------------------------------------
//   ctrlRightClicked
//---------------------------------------------------------

void CtrlPanel::ctrlRightClicked(const QPoint& p, int /*id*/)
{
	if (!editor->curCanvasPart())
		return;

	int cdi = editor->curDrumInstrument();
	int ctlnum = _ctrl->num();
	if (_track->type() == Track::DRUM && ((ctlnum & 0xff) == 0xff) && cdi != -1)
		//ctlnum = (ctlnum & ~0xff) | drumMap[cdi].enote;
		ctlnum = (ctlnum & ~0xff) | cdi;

	MidiPart* part = dynamic_cast<MidiPart*> (editor->curCanvasPart());
	song->execMidiAutomationCtlPopup(0, part, p, ctlnum);
}

//---------------------------------------------------------
//   feedbackModeChanged
//---------------------------------------------------------

void CtrlPanel::feedbackModeChanged(int value)
{
    // index matches FeedbackMode values
    midiMonitor->setFeedbackMode(static_cast<FeedbackMode>(value));
}
