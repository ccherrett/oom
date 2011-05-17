//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: midictrl.cpp,v 1.17.2.10 2009/06/10 00:34:59 terminator356 Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <assert.h>
#include <stdio.h>

#include "midictrl.h"
#include "xml.h"
#include "globals.h"

static const char* ctrlName[] = {
	"Bank Select MSB", "Modulation", "Breath Control", "Controller 3", "Foot Control",
	"Porta Time", "Data Entry MSB", "MainVolume", "Balance", "Controller 9",
	/*10*/ "Pan", "Expression", "Effects Controller 1", "Effects Controller 2", "Controller 14",
	"Controller 15", "Gen Purpose 1", "Gen Purpose 2", "Gen Purpose 3", "Gen Purpose 4",
	/*20*/ "Controller 20", "Controller 21", "Controller 22", "Controller 23", "Controller 24",
	"Controller 25", "Controller 26", "Controller 27", "Controller 28", "Controller 29",
	/*30*/ "Controller 30", "Controller 31", "Bank Select LSB", "Modulation LSB", "Breath Control LSB",
	"Controller 35", "Foot Control LSB", "Porta Time LSB", "Data Entry LSB", "Channel Volume LSB",
	"Balance LSB", "Controller 41", "Pan LSB", "Expression LSB", "Controller 44",
	"Controller 45", "Controller 46", "Controller 47", "Gen Purpose 1 LSB", "Gen Purpose 2 LSB",
	/*50*/ "Gen Purpose 3 LSB", "Gen Purpose 4 LSB", "Controller 52", "Controller 53", "Controller 54",
	"Controller 55", "Controller 56", "Controller 57", "Controller 58", "Controller 59",
	"Controller 60", "Controller 61", "Controller 62", "Controller 63", "Sustain Pedal",
	"Portamento", "Sostenuto", "Soft Pedal", "Legato Pedal", "Hold 2",
	"Sound Variation", "Resonance", "Release Time", "Attack Time", "Cut-off Frequency",
	"Controller 75", "Controller 76", "Controller 77", "Controller 78", "Controller 79",
	"Gen Purpose 5", "Gen Purpose 6", "Gen Purpose 7", "Gen Purpose 8", "Portamento Control",
	"Controller 85", "Controller 86", "Controller 87", "Controller 88", "Controller 89",
	"Controller 90", "Reverb Depth", "Tremolo Depth", "Chorus Depth", "Celeste (De-tune)",
	"Phaser Depth", "Data Increment", "Data Decrement", "NRPN LSB", "NRPN MSB",
	/*100*/ "RPN LSB", "RPN MSB", "Controller 102", "Controller 103", "Controller 104",
	"Controller 105", "Controller 106", "Controller 107", "Controller 108", "Controller 109",
	"Controller 110", "Controller 111", "Controller 112", "Controller 113", "Controller 114",
	"Controller 115", "Controller 116", "Controller 117", "Controller 118", "Controller 119",
	"All Sound Off", "Reset All Controllers", "Local Control", "All Notes Off", "Omni Off",
	"Omni On", "Mono On (Poly off)", "Poly On (Mono off)"
};

#if 0
static const char* ctrl14Name[] = {
	"BankSel", "Modulation", "BreathCtrl",
	"Control 3", "Foot Ctrl", "Porta Time", "DataEntry",
	"MainVolume", "Balance", "Control 9", "Pan",
	"Expression", "Control 12", "Control 13", "Control 14",
	"Control 15", "Gen.Purp.1", "Gen.Purp.2", "Gen.Purp.3",
	"Gen.Purp.4", "Control 20", "Control 21", "Control 22",
	"Control 23", "Control 24", "Control 25", "Control 26",
	"Control 27", "Control 28", "Control 29", "Control 30",
	"Control 31",
};
#endif

enum
{
	COL_NAME = 0, COL_TYPE,
	COL_HNUM, COL_LNUM, COL_MIN, COL_MAX
};

MidiControllerList defaultMidiController;
//
// some global controller which are always available:
//
MidiController veloCtrl("Velocity", CTRL_VELOCITY, 0, 127, 0);
static MidiController pitchCtrl("PitchBend", CTRL_PITCH, -8192, +8191, 0);
static MidiController programCtrl("Program", CTRL_PROGRAM, 0, 0xffffff, 0);
// Removed p3.3.37
//static MidiController mastervolCtrl("MasterVolume", CTRL_MASTER_VOLUME, 0, 0x3fff, 0x3000);
static MidiController volumeCtrl("MainVolume", CTRL_VOLUME, 0, 127, 100);
static MidiController modCtrl("Modulation", CTRL_MODULATION, 0, 127, 0);
static MidiController panCtrl("Pan", CTRL_PANPOT, -64, 63, 0);


//---------------------------------------------------------
//   ctrlType2Int
//   int2ctrlType
//---------------------------------------------------------

static struct
{
	MidiController::ControllerType type;
	QString name;
} ctrlTypes[] = {
	{ MidiController::Controller7, QString("Control7")},
	{ MidiController::Controller14, QString("Control14")},
	{ MidiController::RPN, QString("RPN")},
	{ MidiController::NRPN, QString("NRPN")},
	{ MidiController::RPN14, QString("RPN14")},
	{ MidiController::NRPN14, QString("NRPN14")},
	{ MidiController::Pitch, QString("Pitch")},
	{ MidiController::Program, QString("Program")},
	{ MidiController::Controller7, QString("Control")}, // alias
};

//---------------------------------------------------------
//   ctrlType2Int
//---------------------------------------------------------

MidiController::ControllerType ctrlType2Int(const QString& s)
{
	int n = sizeof (ctrlTypes) / sizeof (*ctrlTypes);
	for (int i = 0; i < n; ++i)
	{
		if (ctrlTypes[i].name == s)
			return ctrlTypes[i].type;
	}
	return MidiController::ControllerType(0);
}

//---------------------------------------------------------
//   int2ctrlType
//---------------------------------------------------------

const QString& int2ctrlType(int n)
{
	static QString dontKnow("?T?");
	int size = sizeof (ctrlTypes) / sizeof (*ctrlTypes);
	for (int i = 0; i < size; ++i)
	{
		if (ctrlTypes[i].type == n)
			return ctrlTypes[i].name;
	}
	return dontKnow;
}

//---------------------------------------------------------
//   initMidiController
//---------------------------------------------------------

void initMidiController()
{
	defaultMidiController.add(&veloCtrl);
	defaultMidiController.add(&pitchCtrl);
	defaultMidiController.add(&programCtrl);
	// Removed p3.3.37
	//defaultMidiController.add(&mastervolCtrl);
	defaultMidiController.add(&volumeCtrl);
	defaultMidiController.add(&panCtrl);
	defaultMidiController.add(&modCtrl);
}

//---------------------------------------------------------
//   midiCtrlName
//---------------------------------------------------------

QString midiCtrlName(int ctrl)
{
	if (ctrl < 0x10000)
		return QString(ctrlName[ctrl]);
	return QString("?N?");
}

//---------------------------------------------------------
//   MidiController
//---------------------------------------------------------

MidiController::MidiController()
: _name(QString(QT_TRANSLATE_NOOP("@default", "Velocity")))
{
	_num = CTRL_VELOCITY;
	_minVal = 0;
	_maxVal = 127;
	_initVal = 0;
	updateBias();
}

MidiController::MidiController(const QString& s, int n, int min, int max, int init)
: _name(s), _num(n), _minVal(min), _maxVal(max), _initVal(init)
{
	updateBias();
}

MidiController::MidiController(const MidiController& mc)
{
	copy(mc);
}

//---------------------------------------------------------
//   copy
//---------------------------------------------------------

void MidiController::copy(const MidiController &mc)
{
	_name = mc._name;
	_num = mc._num;
	_minVal = mc._minVal;
	_maxVal = mc._maxVal;
	_initVal = mc._initVal;
	//updateBias();
	_bias = mc._bias;
}

//---------------------------------------------------------
//   operator =
//---------------------------------------------------------

MidiController& MidiController::operator=(const MidiController &mc)
{
	copy(mc);
	return *this;
}

//---------------------------------------------------------
//   type
//---------------------------------------------------------

MidiController::ControllerType midiControllerType(int num)
{
	// p3.3.37
	//if (num < 0x10000)
	if (num < CTRL_14_OFFSET)
		return MidiController::Controller7;
	//if (num < 0x20000)
	if (num < CTRL_RPN_OFFSET)
		return MidiController::Controller14;
	//if (num < 0x30000)
	if (num < CTRL_NRPN_OFFSET)
		return MidiController::RPN;
	//if (num < 0x40000)
	if (num < CTRL_INTERNAL_OFFSET)
		return MidiController::NRPN;
	if (num == CTRL_PITCH)
		return MidiController::Pitch;
	if (num == CTRL_PROGRAM)
		return MidiController::Program;
	if (num == CTRL_VELOCITY)
		return MidiController::Velo;
	//if (num < 0x60000)
	if (num < CTRL_NRPN14_OFFSET)
		return MidiController::RPN14;
	//if (num < 0x70000)
	if (num < CTRL_NONE_OFFSET)
		return MidiController::NRPN14;
	return MidiController::Controller7;
}

//---------------------------------------------------------
//   updateBias
//---------------------------------------------------------

void MidiController::updateBias()/*{{{*/
{
	// If the specified minimum value is negative, we will
	//  translate to a positive-biased range.
	// Cue: A controller like 'coarse tuning', is meant to be displayed
	//  as -24/+24, but really is centered on 64 and goes from 40 to 88.
	int b;
	int mn, mx;
	ControllerType t = midiControllerType(_num);
	switch (t)
	{
		case RPN:
		case NRPN:
		case Controller7:
			b = 64;
			mn = 0;
			mx = 127;
			break;
		case Controller14:
		case RPN14:
		case NRPN14:
			b = 8192;
			mn = 0;
			mx = 16383;
			break;
		case Program:
			b = 0x800000;
			mn = 0;
			mx = 0xffffff;
			break;
		case Pitch:
			b = 0;
			mn = -8192;
			mx = 8191;
			break;
			//case Velo:        // cannot happen
		default:
			b = 64;
			mn = 0;
			mx = 127;
			break;
	}

	// Special handling of pan: Only thing to do is force the range!
	//if(_num == CTRL_PANPOT)
	//{
	//  _minVal = -64;
	//  _maxVal = 63;
	//  _initVal = 0;
	//}

	// TODO: Limit _minVal and _maxVal to range.

	if (_minVal >= 0)
		_bias = 0;
	else
	{
		_bias = b;

		if (t != Program && t != Pitch)
		{
			// Adjust bias to fit desired range.
			if (_minVal + _bias < mn)
				//_minVal = mn - _bias;
				_bias += mn - _minVal + _bias;
			else
				if (_maxVal + _bias > mx)
				//_maxVal = mx - _bias;
				_bias -= _maxVal + _bias - mx;
		}
	}
}/*}}}*/


//---------------------------------------------------------
//   MidiController::write
//---------------------------------------------------------

void MidiController::write(int level, Xml& xml) const
{
	ControllerType t = midiControllerType(_num);
	if (t == Velo)
		return;

	QString type(int2ctrlType(t));

	int h = (_num >> 8) & 0x7f;
	int l = _num & 0x7f;

	QString sl;
	if ((_num & 0xff) == 0xff)
		sl = "pitch";
	else
		sl.setNum(l);

	xml.nput(level, "<Controller name=\"%s\"", Xml::xmlString(_name).toLatin1().constData());
	if (t != Controller7)
		xml.nput(" type=\"%s\"", type.toLatin1().constData());

	int mn = 0;
	int mx = 0;
	switch (t)
	{
		case RPN:
		case NRPN:
			xml.nput(" h=\"%d\"", h);
			xml.nput(" l=\"%s\"", sl.toLatin1().constData());
			mx = 127;
			break;
		case Controller7:
			xml.nput(" l=\"%s\"", sl.toLatin1().constData());
			mx = 127;
			break;
		case Controller14:
		case RPN14:
		case NRPN14:
			xml.nput(" h=\"%d\"", h);
			xml.nput(" l=\"%s\"", sl.toLatin1().constData());
			mx = 16383;
			break;
		case Pitch:
			mn = -8192;
			mx = 8191;
			break;
		case Program:
		case Velo: // Cannot happen
			break;
	}

	if (t == Program)
	{
		if (_initVal != CTRL_VAL_UNKNOWN && _initVal != 0xffffff)
			xml.nput(" init=\"0x%x\"", _initVal);
	}
	else
	{
		if (_minVal != mn)
			xml.nput(" min=\"%d\"", _minVal);
		if (_maxVal != mx)
			xml.nput(" max=\"%d\"", _maxVal);

		if (_initVal != CTRL_VAL_UNKNOWN)
			xml.nput(" init=\"%d\"", _initVal);
	}
	//xml.put(level, " />");
	xml.put(" />");
}

//---------------------------------------------------------
//   MidiController::read
//---------------------------------------------------------

void MidiController::read(Xml& xml)
{
	ControllerType t = Controller7;
	_initVal = CTRL_VAL_UNKNOWN;
	static const int NOT_SET = 0x100000;
	_minVal = NOT_SET;
	_maxVal = NOT_SET;
	int h = 0;
	int l = 0;
	bool ok;
	int base = 10;

	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::Attribut:
			{
				QString s = xml.s2();
				if (s.left(2) == "0x")
					base = 16;
				if (tag == "name")
					_name = xml.s2();
				else if (tag == "type")
					t = ctrlType2Int(xml.s2());
				else if (tag == "h")
					h = xml.s2().toInt(&ok, base);
				else if (tag == "l")
				{
					// By T356 08/16/08. Changed wildcard to '*'.
					// Changed back to 'pitch' again.
					// Support instrument files with '*' as wildcard.
					if ((xml.s2() == "*") || (xml.s2() == "pitch"))
						l = 0xff;
					else
						l = xml.s2().toInt(&ok, base);
				}
				else if (tag == "min")
					_minVal = xml.s2().toInt(&ok, base);
				else if (tag == "max")
					_maxVal = xml.s2().toInt(&ok, base);
				else if (tag == "init")
					_initVal = xml.s2().toInt(&ok, base);
			}
				break;
			case Xml::TagStart:
				xml.unknown("MidiController");
				break;
			case Xml::TagEnd:
				if (tag == "Controller")
				{
					_num = (h << 8) + l;
					switch (t)
					{
						case RPN:
							if (_maxVal == NOT_SET)
								_maxVal = 127;
							// p3.3.37
							//_num |= 0x20000;
							_num |= CTRL_RPN_OFFSET;
							break;
						case NRPN:
							if (_maxVal == NOT_SET)
								_maxVal = 127;
							//_num |= 0x30000;
							_num |= CTRL_NRPN_OFFSET;
							break;
						case Controller7:
							if (_maxVal == NOT_SET)
								_maxVal = 127;
							break;
						case Controller14:
							//_num |= 0x10000;
							_num |= CTRL_14_OFFSET;
							if (_maxVal == NOT_SET)
								_maxVal = 16383;
							break;
						case RPN14:
							if (_maxVal == NOT_SET)
								_maxVal = 16383;
							//_num |= 0x50000;
							_num |= CTRL_RPN14_OFFSET;
							break;
						case NRPN14:
							if (_maxVal == NOT_SET)
								_maxVal = 16383;
							//_num |= 0x60000;
							_num |= CTRL_NRPN14_OFFSET;
							break;
						case Pitch:
							if (_maxVal == NOT_SET)
								_maxVal = 8191;
							if (_minVal == NOT_SET)
								_minVal = -8192;
							_num = CTRL_PITCH;
							break;
						case Program:
							if (_maxVal == NOT_SET)
								_maxVal = 0xffffff;
							_num = CTRL_PROGRAM;
							break;
						case Velo: // cannot happen
							break;
					}
					if (_minVal == NOT_SET)
						_minVal = 0;

					updateBias();
					return;
				}
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   genNum
//---------------------------------------------------------

int MidiController::genNum(MidiController::ControllerType t, int h, int l)
{
	int val = (h << 8) + l;
	switch (t)
	{
		case Controller7:
			return l;
		case Controller14:
			return val + CTRL_14_OFFSET;
		case RPN:
			//return l + CTRL_RPN_OFFSET;
			return val + CTRL_RPN_OFFSET;
		case NRPN:
			//return l + CTRL_NRPN_OFFSET;
			return val + CTRL_NRPN_OFFSET;
		case RPN14:
			return val + CTRL_RPN14_OFFSET;
		case NRPN14:
			return val + CTRL_NRPN14_OFFSET;
		case Pitch:
			return CTRL_PITCH;
		case Program:
			return CTRL_PROGRAM;
		default:
			return -1;
	}
}

//---------------------------------------------------------
//   MidiCtrlValList
//---------------------------------------------------------

MidiCtrlValList::MidiCtrlValList(int c)
{
	ctrlNum = c;
	_hwVal = CTRL_VAL_UNKNOWN;
	_lastValidHWVal = CTRL_VAL_UNKNOWN;
}

//---------------------------------------------------------
//   clearDelete
//---------------------------------------------------------

void MidiCtrlValListList::clearDelete(bool deleteLists)
{
	for (iMidiCtrlValList imcvl = begin(); imcvl != end(); ++imcvl)
	{
		if (imcvl->second)
		{
			imcvl->second->clear();
			if (deleteLists)
				delete imcvl->second;
		}
	}
	if (deleteLists)
		clear();
}

//---------------------------------------------------------
//   setHwVal
//   Returns false if value is already equal, true if value is changed.
//---------------------------------------------------------

bool MidiCtrlValList::setHwVal(const int v)
{
	if (_hwVal == v)
		return false;

	_hwVal = v;
	if (_hwVal != CTRL_VAL_UNKNOWN)
		_lastValidHWVal = _hwVal;

	return true;
}

//---------------------------------------------------------
//   setHwVals
//   Sets current and last HW values.
//   Handy for forcing labels to show 'off' and knobs to show specific values 
//    without having to send two messages.
//   Returns false if both values are already set, true if either value is changed.
//---------------------------------------------------------

bool MidiCtrlValList::setHwVals(const int v, int const lastv)
{
	if (_hwVal == v && _lastValidHWVal == lastv)
		return false;

	_hwVal = v;
	// Don't want to break our own rules - _lastValidHWVal can't be unknown while _hwVal is valid...
	// But _hwVal can be unknown while _lastValidHWVal is valid...
	if (lastv == CTRL_VAL_UNKNOWN)
		_lastValidHWVal = _hwVal;
	else
		_lastValidHWVal = lastv;

	return true;
}

//---------------------------------------------------------
//   partAtTick
//---------------------------------------------------------

Part* MidiCtrlValList::partAtTick(int tick) const
{
	// Determine (first) part at tick. Return 0 if none found.

	ciMidiCtrlVal i = lower_bound(tick);
	if (i == end() || i->first != tick)
	{
		if (i == begin())
			return 0;
		--i;
	}
	return i->second.part;
}

//---------------------------------------------------------
//   iValue
//---------------------------------------------------------

iMidiCtrlVal MidiCtrlValList::iValue(int tick)
{
	// Determine value at tick, using values stored by ANY part...

	iMidiCtrlVal i = lower_bound(tick);
	if (i == end() || i->first != tick)
	{
		if (i == begin())
			return end();
		--i;
	}
	return i;
}

//---------------------------------------------------------
//   value
//---------------------------------------------------------

int MidiCtrlValList::value(int tick) const
{
	// Determine value at tick, using values stored by ANY part...

	ciMidiCtrlVal i = lower_bound(tick);
	if (i == end() || i->first != tick)
	{
		if (i == begin())
			return CTRL_VAL_UNKNOWN;
		--i;
	}
	return i->second.val;
}

int MidiCtrlValList::value(int tick, Part* part) const
{
	// Determine value at tick, using values stored by the SPECIFIC part...

	// Get the first value found with with a tick equal or greater than specified tick.
	ciMidiCtrlVal i = lower_bound(tick);
	// Since values from different parts can have the same tick, scan for part in all values at that tick.
	for (ciMidiCtrlVal j = i; j != end() && j->first == tick; ++j)
	{
		if (j->second.part == part)
			return j->second.val;
	}
	// Scan for part in all previous values, regardless of tick.
	while (i != begin())
	{
		--i;
		if (i->second.part == part)
			return i->second.val;
	}
	// No previous values were found belonging to the specified part.
	return CTRL_VAL_UNKNOWN;
}

//---------------------------------------------------------
//   add
//    return true if new controller value added or replaced
//---------------------------------------------------------

// Changed by T356.
//bool MidiCtrlValList::add(int tick, int val)

bool MidiCtrlValList::addMCtlVal(int tick, int val, Part* part)
{
	iMidiCtrlVal e = findMCtlVal(tick, part);

	if (e != end())
	{
		if (e->second.val != val)
		{
			e->second.val = val;
			return true;
		}
		return false;
	}

	MidiCtrlVal v;
	v.val = val;
	v.part = part;
	insert(std::pair<const int, MidiCtrlVal > (tick, v));
	return true;
}

//---------------------------------------------------------
//   del
//---------------------------------------------------------

// Changed by T356.
//void MidiCtrlValList::del(int tick)

void MidiCtrlValList::delMCtlVal(int tick, Part* part)
{
	iMidiCtrlVal e = findMCtlVal(tick, part);
	if (e == end())
	{
		if (debugMsg)
			printf("MidiCtrlValList::delMCtlVal(%d): not found (size %zd)\n", tick, size());
		return;
	}
	erase(e);
}

//---------------------------------------------------------
//   find
//---------------------------------------------------------

// Changed by T356.
//iMidiCtrlVal MidiCtrlValList::find(int tick, Part* part)

iMidiCtrlVal MidiCtrlValList::findMCtlVal(int tick, Part* part)
{
	MidiCtrlValRange range = equal_range(tick);
	for (iMidiCtrlVal i = range.first; i != range.second; ++i)
	{
		if (i->second.part == part)
			return i;
	}
	return end();
}

//---------------------------------------------------------
//   MidiControllerList
//---------------------------------------------------------

MidiControllerList::MidiControllerList(const MidiControllerList& mcl) : std::map<int, MidiController*>()
{
	//copy(mcl);

	for (ciMidiController i = mcl.begin(); i != mcl.end(); ++i)
	{
		MidiController* mc = i->second;
		add(new MidiController(*mc));
	}
}

//---------------------------------------------------------
//   copy
//---------------------------------------------------------
//void MidiControllerList::copy(const MidiControllerList &mcl)
//{
//  clear();
//  for(ciMidiController i = mcl.begin(); i != mcl.end(); ++i)
//  {
//    MidiController* mc = *i;
//    push_back(new MidiController(*mc));
//  }  
//}

//---------------------------------------------------------
//   operator =
//---------------------------------------------------------
//MidiControllerList& MidiControllerList::operator= (const MidiControllerList &mcl)
//{
//  copy(mcl);
//  return *this;
//}
