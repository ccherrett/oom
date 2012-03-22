//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: minstrument.cpp,v 1.10.2.5 2009/03/28 01:46:10 terminator356 Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <errno.h>

#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QList>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QMessageBox>
#include <QObject>

#include "minstrument.h"
#include "midiport.h"
#include "mididev.h"
#include "audio.h"
#include "midi.h"
#include "globals.h"
#include "xml.h"
#include "event.h"
#include "mpevent.h"
#include "midictrl.h"
#include "gconfig.h"
#include "song.h"

MidiInstrumentList midiInstruments;
MidiInstrument* genericMidiInstrument;

static const char* gmdrumname = "GM-drums";

//---------------------------------------------------------
//   string2sysex
//---------------------------------------------------------

int string2sysex(const QString& s, unsigned char** data)
{
	QByteArray ba = s.toLatin1();
	const char* src = ba.constData();
	char buffer[2048];
	char* dst = buffer;

	if (src)
	{
		while (*src)
		{
			while (*src == ' ' || *src == '\n')
			{
				++src;
			}
			char* ep;
			long val = strtol(src, &ep, 16);
			if (ep == src)
			{
				QMessageBox::information(0,
						QString("OOMidi"),
						QWidget::tr("Cannot convert sysex string"));
				return 0;
			}
			src = ep;
			*dst++ = val;
			if (dst - buffer >= 2048)
			{
				QMessageBox::information(0,
						QString("OOMidi"),
						QWidget::tr("Hex String too long (2048 bytes limit)"));
				return 0;
			}
		}
	}
	int len = dst - buffer;
	unsigned char* b = new unsigned char[len + 1];
	memcpy(b, buffer, len);
	b[len] = 0;
	*data = b;
	return len;
}

//---------------------------------------------------------
//   sysex2string
//---------------------------------------------------------

QString sysex2string(int len, unsigned char* data)
{
	QString d;
	QString s;
	for (int i = 0; i < len; ++i)
	{
		if ((i > 0) && ((i % 8) == 0))
		{
			d += "\n";
		}
		else if (i)
			d += " ";
		d += s.sprintf("%02x", data[i]);
	}
	return d;
}

//---------------------------------------------------------
//   readEventList
//---------------------------------------------------------

static void readEventList(Xml& xml, EventList* el, const char* name)
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
				if (tag == "event")
				{
					Event e(Note);
					e.read(xml);
					el->add(e);
				}
				else
					xml.unknown("readEventList");
				break;
			case Xml::TagEnd:
				if (tag == name)
					return;
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

//---------------------------------------------------------
//   loadIDF
//---------------------------------------------------------

static void loadIDF(QFileInfo* fi)
{
	FILE* f = fopen(fi->filePath().toAscii().constData(), "r");
	if (f == 0)
		return;
	if (debugMsg)
		printf("READ IDF %s\n", fi->filePath().toLatin1().constData());
	Xml xml(f);

	bool skipmode = true;
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
				if (skipmode && (tag == "oom" || tag == "muse"))
					skipmode = false;
				else if (skipmode)
					break;
				else if (tag == "MidiInstrument")
				{
					MidiInstrument* i = new MidiInstrument();
					i->setFilePath(fi->filePath());
					i->read(xml);
					// Ignore duplicate named instruments.
					iMidiInstrument ii = midiInstruments.begin();
					for (; ii != midiInstruments.end(); ++ii)
					{
						if ((*ii)->iname() == i->iname())
							break;
					}
					if (ii == midiInstruments.end())
						midiInstruments.push_back(i);
					else
						delete i;
				}
				else
					xml.unknown("oom");
				break;
			case Xml::Attribut:
				break;
			case Xml::TagEnd:
				if (!skipmode && (tag == "oom" || tag == "muse"))
				{
					return;
				}
			default:
				break;
		}
	}
	fclose(f);
}

//---------------------------------------------------------
//   initMidiInstruments
//---------------------------------------------------------

void initMidiInstruments()
{
	genericMidiInstrument = new MidiInstrument(QWidget::tr("generic midi"));
	midiInstruments.push_back(genericMidiInstrument);
	if (debugMsg)
		printf("load user instrument definitions from <%s>\n", oomUserInstruments.toLatin1().constData());
	QDir usrInstrumentsDir(oomUserInstruments, QString("*.idf"));
	if (usrInstrumentsDir.exists())
	{
		QFileInfoList list = usrInstrumentsDir.entryInfoList();
		QFileInfoList::iterator it = list.begin(); // ddskrjo
		while (it != list.end())
		{ // ddskrjo
			loadIDF(&*it);
			++it;
		}
	}
	//else
	//{
	//  if(usrInstrumentsDir.mkdir(oomUserInstruments))
	//    printf("Created user instrument directory: %s\n", oomUserInstruments.toLatin1());
	//  else
	//    printf("Unable to create user instrument directory: %s\n", oomUserInstruments.toLatin1());
	//}

	if (debugMsg)
		printf("load instrument definitions from <%s>\n", oomInstruments.toLatin1().constData());
	QDir instrumentsDir(oomInstruments, QString("*.idf"));
	if (instrumentsDir.exists())
	{
		QFileInfoList list = instrumentsDir.entryInfoList();
		QFileInfoList::iterator it = list.begin(); // ddskrjo
		while (it != list.end())
		{
			loadIDF(&*it);
			++it;
		}
	}
	else
		printf("Instrument directory not found: %s\n", oomInstruments.toLatin1().constData());

}

//---------------------------------------------------------
//   registerMidiInstrument
//---------------------------------------------------------

MidiInstrument* registerMidiInstrument(const QString& name)
{
	for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
	{
		if ((*i)->iname() == name)
			return *i;
	}
	return genericMidiInstrument;
}

//---------------------------------------------------------
//   removeMidiInstrument
//---------------------------------------------------------

void removeMidiInstrument(const QString& name)
{
	for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
	{
		if ((*i)->iname() == name)
		{
			midiInstruments.erase(i);
			return;
		}
	}
}

void removeMidiInstrument(const MidiInstrument* instr)
{
	for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
	{
		if (*i == instr)
		{
			midiInstruments.erase(i);
			return;
		}
	}
}

//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

void MidiInstrument::init()
{
	_nullvalue = -1;
	_initScript = 0;
	_midiInit = new EventList();
	_midiReset = new EventList();
	_midiState = new EventList();
	_controller = new MidiControllerList;

	// add some default controller to controller list
	// this controllers are always available for all instruments
	//
	MidiController* prog = new MidiController("Program", CTRL_PROGRAM, 0, 0xffffff, 0);
	_controller->add(prog);
	_dirty = false;
}

MidiInstrument::MidiInstrument()
{
	m_oomInstrument = false;
	m_panValue = 0.0;
	m_verbValue = config.minSlider;
	init();
}

//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

MidiInstrument::MidiInstrument(const QString& txt)
{
	_name = txt;
	init();
}

//---------------------------------------------------------
//   MidiInstrument
//---------------------------------------------------------

MidiInstrument::~MidiInstrument()
{
	for (ciPatchGroup g = pg.begin(); g != pg.end(); ++g)
	{
		PatchGroup* pgp = *g;
		const PatchList& pl = pgp->patches;
		for (ciPatch p = pl.begin(); p != pl.end(); ++p)
		{
			delete *p;
		}
		delete pgp;
	}


	delete _midiInit;
	delete _midiReset;
	delete _midiState;
	for (iMidiController i = _controller->begin(); i != _controller->end(); ++i)
		delete i->second;
	delete _controller;

	if (_initScript)
		delete _initScript;
}

//---------------------------------------------------------
//   assign
//---------------------------------------------------------

MidiInstrument& MidiInstrument::assign(const MidiInstrument& ins)
{
	//---------------------------------------------------------
	// TODO: Copy the _initScript, and _midiInit, _midiReset, and _midiState lists.
	//---------------------------------------------------------

	for (iMidiController i = _controller->begin(); i != _controller->end(); ++i)
		delete i->second;
	_controller->clear();

	_nullvalue = ins._nullvalue;

	// Assignment
	for (ciMidiController i = ins._controller->begin(); i != ins._controller->end(); ++i)
	{
		MidiController* mc = i->second;
		_controller->add(new MidiController(*mc));
	}

	for (ciPatchGroup g = pg.begin(); g != pg.end(); ++g)
	{
		PatchGroup* pgp = *g;
		const PatchList& pl = pgp->patches;
		for (ciPatch p = pl.begin(); p != pl.end(); ++p)
		{
			delete *p;
		}

		delete pgp;
	}
	pg.clear();

	for (ciPatchGroup g = ins.pg.begin(); g != ins.pg.end(); ++g)
	{
		PatchGroup* pgp = *g;
		const PatchList& pl = pgp->patches;
		PatchGroup* npg = new PatchGroup;
		npg->name = pgp->name;
		pg.push_back(npg);
		for (ciPatch p = pl.begin(); p != pl.end(); ++p)
		{
			Patch* pp = *p;
			Patch* np = new Patch;
			np->typ = pp->typ;
			np->hbank = pp->hbank;
			np->lbank = pp->lbank;
			np->prog = pp->prog;
			np->name = pp->name;
			np->drum = pp->drum;
			np->keys = pp->keys;
			np->keyswitches = pp->keyswitches;
			np->comments = pp->comments;
			np->loadmode = pp->loadmode;
			np->filename = pp->filename;
			np->engine = pp->engine;
			np->volume = pp->volume;
			np->index = pp->index;
			npg->patches.push_back(np);
		}
	}

	_name = ins._name;
	_filePath = ins._filePath;
	m_keymaps = ins.m_keymaps;
	m_oomInstrument = ins.m_oomInstrument;
	m_panValue = ins.m_panValue;
	m_verbValue = ins.m_verbValue;

	return *this;
}

Patch* MidiInstrument::getDefaultPatch()
{
	Patch* rv = 0;
	if(m_oomInstrument)
	{
		for (ciPatchGroup g = pg.begin(); g != pg.end(); ++g)
		{
			PatchGroup* pgp = *g;
			const PatchList& pl = pgp->patches;
			for (ciPatch p = pl.begin(); p != pl.end(); ++p)
			{
				if(!(*p)->filename.isEmpty() && (*p)->loadmode >= 0 && !(*p)->engine.isEmpty())
				{
					rv = (*p);
					break;
				}
			}
			if(rv)
				break;
		}
	}
	return rv;
}

//---------------------------------------------------------
//   reset
//    send note off to all channels
//---------------------------------------------------------

void MidiInstrument::reset(int portNo, MType)
{
	MidiPort* port = &midiPorts[portNo];
	MidiPlayEvent ev;
	ev.setType(0x90);
	ev.setPort(portNo);
	ev.setTime(0);
	for (int chan = 0; chan < MIDI_CHANNELS; ++chan)
	{
		ev.setChannel(chan);
		for (int pitch = 0; pitch < 128; ++pitch)
		{
			ev.setA(pitch);
			ev.setB(0);
			port->sendEvent(ev);
		}
	}
}

//---------------------------------------------------------
//   readPatchGroup
//---------------------------------------------------------

void PatchGroup::read(Xml& xml)
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
				if (tag == "Patch")
				{
					Patch* patch = new Patch;
					patch->read(xml);
					patches.push_back(patch);
				}
				else
					xml.unknown("PatchGroup");
				break;
			case Xml::Attribut:
				if (tag == "name")
					name = xml.s2();
				break;
			case Xml::TagEnd:
				if (tag == "PatchGroup")
					return;
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Patch::read(Xml& xml)/*{{{*/
{
	typ = -1;
	hbank = -1;
	lbank = -1;
	prog = 0;
	drum = false;
	keys.clear();
	keyswitches.clear();
	loadmode = -1;
	index = -1;
	volume = 1.0;
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
				xml.unknown("Patch");
				break;
			case Xml::Attribut:
				if (tag == "name")
					name = xml.s2();
				else if (tag == "mode")
					typ = xml.s2().toInt();
				else if (tag == "hbank")
					hbank = xml.s2().toInt();
				else if (tag == "lbank")
					lbank = xml.s2().toInt();
				else if (tag == "prog")
					prog = xml.s2().toInt();
				else if (tag == "drum")
					drum = xml.s2().toInt();
				else if(tag == "keys")
				{
					QStringList klist = ((QString)xml.s2()).split(QString(" "), QString::SkipEmptyParts);
					for (QStringList::Iterator it = klist.begin(); it != klist.end(); ++it)
					{
						int val = (*it).toInt();
						keys.append(val);
					}
				}
				else if(tag == "keyswitches")
				{
					QStringList klist = ((QString)xml.s2()).split(QString(" "), QString::SkipEmptyParts);
					for (QStringList::Iterator it = klist.begin(); it != klist.end(); ++it)
					{
						int val = (*it).toInt();
						keyswitches.append(val);
					}
				}
				else if(tag == "comments")
				{
					QStringList clist = ((QString)xml.s2()).split(QString(" "), QString::SkipEmptyParts);
					for (QStringList::Iterator it = clist.begin(); it != clist.end(); ++it)
					{
						QStringList hashlist = ((*it)).split(QString("@@:@@"), QString::SkipEmptyParts);
						if(hashlist.size() == 2)
						{
							int k = hashlist.at(0).toInt();
							comments[k] = hashlist.at(1);
						}
					}
				}
				else if(tag == "engine")
				{
					engine = xml.s2();
				}
				else if(tag == "filename")
				{
					filename = xml.s2();
				}
				else if(tag == "loadmode")
				{
					loadmode = xml.s2().toInt();
				}
				else if(tag == "volume")
				{
					volume = xml.s2().toFloat();
				}
				else if(tag == "index")
				{
					index = xml.s2().toInt();
				}
				break;
			case Xml::TagEnd:
				if (tag == "Patch")
					return;
			default:
				break;
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Patch::write(int level, Xml& xml)/*{{{*/
{
	xml.nput(level, "<Patch name=\"%s\"", Xml::xmlString(name).toLatin1().constData());
	if (typ != -1)
		xml.nput(" mode=\"%d\"", typ);

	if (hbank != -1)
		xml.nput(" hbank=\"%d\"", hbank);

	if (lbank != -1)
		xml.nput(" lbank=\"%d\"", lbank);

	xml.nput(" prog=\"%d\"", prog);

	if (drum)
		xml.nput(" drum=\"%d\"", int(drum));
	
	if(!QString(filename).isEmpty())
		xml.nput(" filename=\"%s\"", Xml::xmlString(filename).toLatin1().constData());
	if(!QString(engine).isEmpty())
		xml.nput(" engine=\"%s\"", Xml::xmlString(engine).toLatin1().constData());
	if(loadmode != -1)
		xml.nput(" loadmode=\"%d\"", loadmode);
	if(index != -1)
		xml.nput(" index=\"%d\"", index);
	xml.nput(" volume=\"%f\"", volume);

	if(!keys.isEmpty())
	{
		QString keyString;
		for(int i = 0; i < keys.size(); ++i)
		{
			keyString.append(QString::number(keys.at(i)));
			if(i < (keys.size() - 1))
				keyString.append(" ");
		}
		xml.nput(" keys=\"%s\"", keyString.toUtf8().constData());
	}
	if(!keyswitches.isEmpty())
	{
		QString keyString;
		for(int i = 0; i < keyswitches.size(); ++i)
		{
			keyString.append(QString::number(keyswitches.at(i)));
			if(i < (keyswitches.size() - 1))
				keyString.append(" ");
		}
		xml.nput(" keyswitches=\"%s\"", keyString.toUtf8().constData());
	}
	if(!comments.empty())
	{
		QString c;
		QHashIterator<int, QString> it(comments);
		while(it.hasNext())
		{
			it.next();
			QString val = QString::number(it.key()).append("@@:@@").append(it.value());
			c.append(val).append(" ");
		}
		xml.nput(" comments=\"%s\"", c.toUtf8().constData());
	}
	xml.put(" />");
}/*}}}*/

void KeyMap::read(Xml& xml)/*{{{*/
{
	program = -1;
	pname = "";
	comment = "";
	key = -1;
	hasProgram = false;
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
				xml.unknown("KeyMap");
				break;
			case Xml::Attribut:
				if (tag == "comment")
					comment = xml.s2();
				else if (tag == "program")
					program = xml.s2().toInt();
				else if (tag == "key")
					key = xml.s2().toInt();
				else if (tag == "pname")
					pname = xml.s2();
				else if(tag == "hasProgram")
					hasProgram = (bool)xml.s2().toInt();
				break;
			case Xml::TagEnd:
				if (tag == "KeyMap")
					return;
			default:
				break;
		}
	}
}/*}}}*/

void KeyMap::write(int level, Xml& xml)/*{{{*/
{
	xml.nput(level, "<KeyMap key=\"%d\"", key);

	xml.nput(" program=\"%d\"", program);
	xml.nput(" hasProgram=\"%d\"", hasProgram);

	xml.nput(" comment=\"%s\"", Xml::xmlString(comment).toLatin1().constData());

	xml.nput(" pname=\"%s\"", Xml::xmlString(pname).toLatin1().constData());

	xml.put(" />");
}/*}}}*/
//---------------------------------------------------------
//   readMidiState
//---------------------------------------------------------

void MidiInstrument::readMidiState(Xml& xml)
{
	_midiState->read(xml, "midistate", true);
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MidiInstrument::read(Xml& xml)
{
	bool ok;
	int base = 10;
	_nullvalue = -1;
	m_keymaps.clear();
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
				if (tag == "Patch")
				{
					Patch* patch = new Patch;
					patch->read(xml);
					if (pg.empty())
					{
						PatchGroup* p = new PatchGroup;
						p->patches.push_back(patch);
						pg.push_back(p);
					}
					else
						pg[0]->patches.push_back(patch);
				}
				else if (tag == "PatchGroup")
				{
					PatchGroup* p = new PatchGroup;
					p->read(xml);
					pg.push_back(p);
				}
				else if (tag == "Controller")
				{
					MidiController* mc = new MidiController();
					mc->read(xml);
					// Added by Tim. Copied from oom 2.
					//
					// HACK: make predefined "Program" controller overloadable
					//
					if (mc->name() == "Program")
					{
						for (iMidiController i = _controller->begin(); i != _controller->end(); ++i)
						{
							if (i->second->name() == mc->name())
							{
								delete i->second;
								_controller->erase(i);
								break;
							}
						}
					}

					_controller->add(mc);
				}
				else if (tag == "Init")
					readEventList(xml, _midiInit, "Init");
				else if (tag == "Reset")
					readEventList(xml, _midiReset, "Reset");
				else if (tag == "State")
					readEventList(xml, _midiState, "State");
				else if (tag == "InitScript")
				{
					if (_initScript)
						delete _initScript;
					QByteArray ba = xml.parse1().toLatin1();
					const char* istr = ba.constData();
					int len = strlen(istr) + 1;
					if (len > 1)
					{
						_initScript = new char[len];
						memcpy(_initScript, istr, len);
					}
				}
				else if(tag == "KeyMap")
				{
					KeyMap *km = new KeyMap;
					km->read(xml);
					m_keymaps.insert(km->key, km);
				}
				else
					xml.unknown("MidiInstrument");
				break;
			case Xml::Attribut:
				if (tag == "name")
					setIName(xml.s2());
				else if (tag == "nullparam")
				{
					_nullvalue = xml.s2().toInt(&ok, base);
				}
				else if(tag == "oomInstrument")
					m_oomInstrument = xml.s2().toInt();
				else if(tag == "panValue")
					m_panValue = xml.s2().toDouble();
				else if(tag == "verbValue")
					m_verbValue = xml.s2().toDouble();
				break;
			case Xml::TagEnd:
				if (tag == "MidiInstrument")
					return;
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MidiInstrument::write(int level, Xml& xml)
{
	xml.header();
	xml.tag(level, "oom version=\"1.0\"");
	level++;
	xml.nput(level, "<MidiInstrument name=\"%s\" oomInstrument=\"%d\" panValue=\"%f\" verbValue=\"%f\"", 
			Xml::xmlString(iname()).toLatin1().constData(), m_oomInstrument, m_panValue, m_verbValue);

	if (_nullvalue != -1)
	{
		QString nv;
		nv.setNum(_nullvalue);
		xml.nput(" nullparam=\"%s\"", nv.toLatin1().constData());
	}
	xml.put(">");

	// -------------
	// What about Init, Reset, State, and InitScript ?
	// -------------

	level++;
	for (ciPatchGroup g = pg.begin(); g != pg.end(); ++g)
	{
		PatchGroup* pgp = *g;
		const PatchList& pl = pgp->patches;
		xml.tag(level, "PatchGroup name=\"%s\"", Xml::xmlString(pgp->name).toLatin1().constData());
		level++;
		for (ciPatch p = pl.begin(); p != pl.end(); ++p)
			(*p)->write(level, xml);
		level--;
		xml.etag(level, "PatchGroup");
	}
	for (iMidiController ic = _controller->begin(); ic != _controller->end(); ++ic)
		ic->second->write(level, xml);
	for(QHash<int, KeyMap*>::const_iterator km = m_keymaps.begin(); km != m_keymaps.end(); ++km)
	{
		KeyMap *m = km.value();
		m->write(level, xml);
	}
	level--;
	xml.etag(level, "MidiInstrument");
	level--;
	xml.etag(level, "oom");
}

//---------------------------------------------------------
//   getPatchName
//---------------------------------------------------------

QString MidiInstrument::getPatchName(int channel, int prog, MType mode, bool drum)/*{{{*/
{
	int pr = prog & 0xff;
	if (prog == CTRL_VAL_UNKNOWN || pr == 0xff)
		return "<unknown>";

	int hbank = (prog >> 16) & 0xff;
	int lbank = (prog >> 8) & 0xff;
	int tmask = 1;
	bool drumchan = channel == 9;
	bool hb = false;
	bool lb = false;
	switch (mode)
	{
		case MT_GS:
			tmask = 2;
			hb = true;
			break;
		case MT_XG:
			hb = true;
			lb = true;
			tmask = 4;
			break;
		case MT_GM:
			if (drumchan)
				return gmdrumname;
			tmask = 1;
			break;
		default:
			hb = true; // MSB bank matters
			lb = true; // LSB bank matters
			break;
	}
	for (ciPatchGroup i = pg.begin(); i != pg.end(); ++i)
	{
		const PatchList& pl = (*i)->patches;
		for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl)
		{
			const Patch* mp = *ipl;
			if ((mp->typ & tmask)
					&& (pr == mp->prog)
					&& ((drum && mode != MT_GM) ||
					(mp->drum == drumchan))

					&& (hbank == mp->hbank || !hb || mp->hbank == -1)
					&& (lbank == mp->lbank || !lb || mp->lbank == -1))
				return mp->name;
		}
	}
	return "<unknown>";
}/*}}}*/

Patch* MidiInstrument::getPatch(int channel, int prog, MType mode, bool drum)/*{{{*/
{
	int pr = prog & 0xff;
	if (prog == CTRL_VAL_UNKNOWN || pr == 0xff)
		return 0;

	int hbank = (prog >> 16) & 0xff;
	int lbank = (prog >> 8) & 0xff;
	int tmask = 1;
	bool drumchan = channel == 9;
	bool hb = false;
	bool lb = false;
	switch (mode)
	{
		case MT_GS:
			tmask = 2;
			hb = true;
			break;
		case MT_XG:
			hb = true;
			lb = true;
			tmask = 4;
			break;
		case MT_GM:
			if (drumchan)
				return 0;
			tmask = 1;
			break;
		default:
			hb = true; // MSB bank matters
			lb = true; // LSB bank matters
			break;
	}
	for (ciPatchGroup i = pg.begin(); i != pg.end(); ++i)
	{
		const PatchList& pl = (*i)->patches;
		for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl)
		{
			Patch* mp = *ipl;
			if ((mp->typ & tmask)
					&& (pr == mp->prog)
					&& ((drum && mode != MT_GM) ||
					(mp->drum == drumchan))

					&& (hbank == mp->hbank || !hb || mp->hbank == -1)
					&& (lbank == mp->lbank || !lb || mp->lbank == -1))
				return mp;
		}
	}
	return 0;
}/*}}}*/

//---------------------------------------------------------
//   populatePatchPopup
//---------------------------------------------------------

void MidiInstrument::populatePatchPopup(QMenu* menu, int chan, MType songType, bool drum)
{
	menu->clear();
	int mask = 0;
	bool drumchan = chan == 9;
	switch (songType)
	{
		case MT_XG: mask = 4;
			break;
		case MT_GS: mask = 2;
			break;
		case MT_GM:
			if (drumchan)
				return;
			mask = 1;
			break;
		case MT_UNKNOWN: mask = 7;
			break;
	}
	if (pg.size() > 1)
	{
		for (ciPatchGroup i = pg.begin(); i != pg.end(); ++i)
		{
			PatchGroup* pgp = *i;
			QMenu* pm = menu->addMenu(pgp->name);
			pm->setFont(config.fonts[0]);
			const PatchList& pl = pgp->patches;
			QString& gname = pgp->name;
			for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl)
			{
				const Patch* mp = *ipl;
				if ((mp->typ & mask) &&
						((drum && songType != MT_GM) ||
						(mp->drum == drumchan)))
				{
					int id = ((mp->hbank & 0xff) << 16)
							+ ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
					QAction* act = pm->addAction(mp->name);
					//act->setCheckable(true);
					QString strId = QString::number(id);
					QStringList _data = (QStringList() << strId << gname);
					//_data->append(strId);
					//_data->append(gname);
					//act->setData(id);
					act->setData(_data);
				}

			}
		}
	}
	else if (pg.size() == 1)
	{
		// no groups
		const PatchList& pl = pg.front()->patches;
		for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl)
		{
			const Patch* mp = *ipl;
			if (mp->typ & mask)
			{
				int id = ((mp->hbank & 0xff) << 16)
						+ ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
				QAction* act = menu->addAction(mp->name);
				//act->setCheckable(true);
				QString strId = QString::number(id);
				QStringList _data = (QStringList() << strId);
				//_data->append(strId);
				//act->setData(id);
				act->setData(_data);
			}
		}
	}
}

//---------------------------------------------------------
//   populatePatchModel
//---------------------------------------------------------

void MidiInstrument::populatePatchModel(QStandardItemModel* model, int chan, MType songType, bool drum)
{
	model->clear();
	int mask = 0;
	bool drumchan = chan == 9;
	switch (songType)
	{
		case MT_XG: mask = 4;
			break;
		case MT_GS: mask = 2;
			break;
		case MT_GM:
			if (drumchan)
				return;
			mask = 1;
			break;
		case MT_UNKNOWN: mask = 7;
			break;
	}
	if (pg.size() > 1)
	{
		for (ciPatchGroup i = pg.begin(); i != pg.end(); ++i)
		{
			PatchGroup* pgp = *i;
			QList<QStandardItem*> folder;
			QStandardItem* noop = new QStandardItem("");
			QStandardItem *dir = new QStandardItem(pgp->name);
			QFont f = dir->font();
			f.setBold(true);
			dir->setFont(f);
			const PatchList& pl = pgp->patches;
			for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl)
			{
				const Patch* mp = *ipl;
				if ((mp->typ & mask) && ((drum && songType != MT_GM) || (mp->drum == drumchan)))
				{
					int id = ((mp->hbank & 0xff) << 16) + ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
					QList<QStandardItem*> row;
					QString strId = QString::number(id);
					QStandardItem* idItem = new QStandardItem(strId);
					QStandardItem* nItem = new QStandardItem(mp->name);
					nItem->setToolTip(QString(pgp->name+":\n    "+mp->name));
					row.append(nItem);
					row.append(idItem);
					dir->appendRow(row);
				}

			}
			folder.append(dir);
			folder.append(noop);
			model->appendRow(folder);
		}
	}
	else if (pg.size() == 1)
	{
		// no groups
		const PatchList& pl = pg.front()->patches;
		QStandardItem* root = model->invisibleRootItem();
		for (ciPatch ipl = pl.begin(); ipl != pl.end(); ++ipl)
		{
			const Patch* mp = *ipl;
			if (mp->typ & mask)
			{
				int id = ((mp->hbank & 0xff) << 16) + ((mp->lbank & 0xff) << 8) + (mp->prog & 0xff);
				QList<QStandardItem*> row;
				QString strId = QString::number(id);
				QStandardItem* idItem = new QStandardItem(strId);
				QStandardItem* nItem = new QStandardItem(mp->name);
				nItem->setToolTip(QString(mp->name));
				row.append(nItem);
				row.append(idItem);
				root->appendRow(row);
			}
		}
	}
}

bool MidiInstrument::fileSave()/*{{{*/
{
	if(_filePath.isEmpty())
		return false;
	// Do not allow a direct overwrite of a 'built-in' oom instrument.
	QFileInfo qfi(_filePath);
	if (qfi.absolutePath() == oomInstruments)
	{
		return false;
	}
	FILE* f = fopen(_filePath.toAscii().constData(), "w");
	if (f == 0)
	{
		return false;
	}

	Xml xml(f);

	write(0, xml);

	// Now signal the rest of the app so stuff can change...
	song->update(SC_CONFIG | SC_MIDI_CONTROLLER);

	if (fclose(f) != 0)
	{
		return false;
	}
	return true;
}/*}}}*/
