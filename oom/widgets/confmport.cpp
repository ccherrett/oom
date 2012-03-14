//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: confmport.cpp,v 1.9.2.10 2009/12/15 03:39:58 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <list>
#include <termios.h>
#include <iostream>
#include <stdio.h>

#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QTableWidget>
#include <QTableWidgetItem>

#include "confmport.h"
#include "app.h"
#include "icons.h"
#include "globals.h"
#include "Composer.h"
#include "midiport.h"
#include "mididev.h"
#include "xml.h"
//#include "midisyncimpl.h"
//#include "midifilterimpl.h"
#include "ctrlcombo.h"
#include "minstrument.h"
#include "audio.h"
#include "plugin.h"
#include "midiseq.h"
#include "driver/alsamidi.h"
#include "driver/jackmidi.h"
#include "audiodev.h"
#include "menutitleitem.h"
#include "utils.h"

enum
{
	DEVCOL_NO = 0, DEVCOL_GUI, DEVCOL_CACHE_NRPN, DEVCOL_REC, DEVCOL_PLAY, DEVCOL_INSTR, DEVCOL_NAME,
	DEVCOL_INROUTES, DEVCOL_OUTROUTES, DEVCOL_DEF_IN_CHANS, DEVCOL_DEF_OUT_CHANS, DEVCOL_STATE
	
};

//---------------------------------------------------------
//   MPConfig
//    Midi Port Config
//---------------------------------------------------------

MPConfig::MPConfig(QWidget* parent)
: QFrame(parent)
{
	setupUi(this);
	mdevView->setRowCount(MIDI_PORTS);
	mdevView->verticalHeader()->hide();
	mdevView->setSelectionMode(QAbstractItemView::SingleSelection);
	mdevView->setShowGrid(false);

	//popup      = 0;
	instrPopup = 0;
	_showAliases = -1; // 0: Show first aliases, if available. Nah, stick with -1: none at first.

	QStringList columnnames;
	columnnames << tr("Port")
			<< tr("GUI")
			<< tr("N")
			<< tr("I")
			<< tr("O")
			<< tr("Instr")
			<< tr("D-Name")
			<< tr("Ins")
			<< tr("Outs")
			<< tr("In Ch")
			<< tr("Out Ch")
			<< tr("State");

	mdevView->setColumnCount(columnnames.size());
	mdevView->setHorizontalHeaderLabels(columnnames);
	for (int i = 0; i < columnnames.size(); ++i)
	{
		setWhatsThis(mdevView->horizontalHeaderItem(i), i);
		setToolTip(mdevView->horizontalHeaderItem(i), i);
	}
	mdevView->setFocusPolicy(Qt::NoFocus);
	//mdevView->horizontalHeader()->setMinimumSectionSize(60);
	mdevView->horizontalHeader()->resizeSection(DEVCOL_NO, 50);
	mdevView->horizontalHeader()->resizeSection(DEVCOL_CACHE_NRPN, 20);
	mdevView->horizontalHeader()->resizeSection(DEVCOL_REC, 20);
	mdevView->horizontalHeader()->resizeSection(DEVCOL_PLAY, 20);
	mdevView->horizontalHeader()->resizeSection(DEVCOL_GUI, 40);
	mdevView->horizontalHeader()->resizeSection(DEVCOL_INROUTES, 40);
	mdevView->horizontalHeader()->resizeSection(DEVCOL_OUTROUTES, 140);
	mdevView->horizontalHeader()->resizeSection(DEVCOL_DEF_IN_CHANS, 70);
	mdevView->horizontalHeader()->resizeSection(DEVCOL_DEF_OUT_CHANS, 70);
	mdevView->horizontalHeader()->resizeSection(DEVCOL_INSTR, 140);
	mdevView->horizontalHeader()->resizeSection(DEVCOL_NAME, 140);
	mdevView->horizontalHeader()->setStretchLastSection(true);
	mdevView->horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignLeft);

	connect(mdevView, SIGNAL(itemPressed(QTableWidgetItem*)), this, SLOT(rbClicked(QTableWidgetItem*)));
	connect(mdevView, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(mdevViewItemRenamed(QTableWidgetItem*)));
	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));

	for (int i = MIDI_PORTS - 1; i >= 0; --i)
	{
		mdevView->blockSignals(true); // otherwise itemChanged() is triggered and bad things happen.
		QString s;
		s.setNum(i + 1);
		QTableWidgetItem* itemno = new QTableWidgetItem(s);
		addItem(i, DEVCOL_NO, itemno, mdevView);
		itemno->setTextAlignment(Qt::AlignHCenter);
		itemno->setFlags(Qt::ItemIsEnabled);
		QTableWidgetItem* itemstate = new QTableWidgetItem();
		addItem(i, DEVCOL_STATE, itemstate, mdevView);
		itemstate->setFlags(Qt::ItemIsEnabled);
		QTableWidgetItem* iteminstr = new QTableWidgetItem();
		addItem(i, DEVCOL_INSTR, iteminstr, mdevView);
		iteminstr->setFlags(Qt::ItemIsEnabled);
		QTableWidgetItem* itemname = new QTableWidgetItem;
		addItem(i, DEVCOL_NAME, itemname, mdevView);
		itemname->setFlags(Qt::ItemIsEnabled);
		QTableWidgetItem* itemgui = new QTableWidgetItem;
		addItem(i, DEVCOL_GUI, itemgui, mdevView);
		itemgui->setTextAlignment(Qt::AlignHCenter);
		itemgui->setFlags(Qt::ItemIsEnabled);
		QTableWidgetItem* itemfb = new QTableWidgetItem;
		addItem(i, DEVCOL_CACHE_NRPN, itemfb, mdevView);
		itemfb->setTextAlignment(Qt::AlignHCenter);
		itemfb->setFlags(Qt::ItemIsEnabled);
		QTableWidgetItem* itemrec = new QTableWidgetItem;
		addItem(i, DEVCOL_REC, itemrec, mdevView);
		itemrec->setTextAlignment(Qt::AlignHCenter);
		itemrec->setFlags(Qt::ItemIsEnabled);
		QTableWidgetItem* itemplay = new QTableWidgetItem;
		addItem(i, DEVCOL_PLAY, itemplay, mdevView);
		itemplay->setTextAlignment(Qt::AlignHCenter);
		itemplay->setFlags(Qt::ItemIsEnabled);
		QTableWidgetItem* itemout = new QTableWidgetItem;
		addItem(i, DEVCOL_OUTROUTES, itemout, mdevView);
		itemout->setFlags(Qt::ItemIsEnabled);
		QTableWidgetItem* itemin = new QTableWidgetItem;
		addItem(i, DEVCOL_INROUTES, itemin, mdevView);
		itemin->setFlags(Qt::ItemIsEnabled);
		QTableWidgetItem* itemdefin = new QTableWidgetItem();
		addItem(i, DEVCOL_DEF_IN_CHANS, itemdefin, mdevView);
		itemdefin->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
		QTableWidgetItem* itemdefout = new QTableWidgetItem();
		addItem(i, DEVCOL_DEF_OUT_CHANS, itemdefout, mdevView);
		itemdefout->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);
		mdevView->blockSignals(false);
	}

	songChanged(0);
}

MPConfig::~MPConfig()
{
}

//---------------------------------------------------------
//   mdevViewItemRenamed
//---------------------------------------------------------

void MPConfig::mdevViewItemRenamed(QTableWidgetItem* item)
{
	int col = item->column();
	QString s = item->text();
	//printf("MPConfig::mdevViewItemRenamed col:%d txt:%s\n", col, s.toLatin1().constData());
	if (item == 0)
		return;
	switch (col)
	{
		case DEVCOL_DEF_IN_CHANS:
		{
			QString id = item->tableWidget()->item(item->row(), DEVCOL_NO)->text();
			int no = atoi(id.toLatin1().constData()) - 1;
			if (no < 0 || no >= MIDI_PORTS)
				return;
			midiPorts[no].setDefaultInChannels(((1 << MIDI_CHANNELS) - 1) & string2bitmap(s));
			song->update();
		}
			break;
		case DEVCOL_DEF_OUT_CHANS:
		{
			QString id = item->tableWidget()->item(item->row(), DEVCOL_NO)->text();
			int no = atoi(id.toLatin1().constData()) - 1;
			if (no < 0 || no >= MIDI_PORTS)
				return;
			midiPorts[no].setDefaultOutChannels(((1 << MIDI_CHANNELS) - 1) & string2bitmap(s));
			song->update();
		}
			break;
		case DEVCOL_NAME:
		{
			QString id = item->tableWidget()->item(item->row(), DEVCOL_NO)->text();
			int no = atoi(id.toLatin1().constData()) - 1;
			if (no < 0 || no >= MIDI_PORTS)
				return;

			MidiPort* port = &midiPorts[no];
			MidiDevice* dev = port->device();
			// Only Jack midi devices.
			if (!dev || dev->deviceType() != MidiDevice::JACK_MIDI)
				return;
			if (dev->name() == s)
				return;

			if (midiDevices.find(s))
			{
				QMessageBox::critical(this,
						tr("OOMidi: bad device name"),
						tr("please choose a unique device name"),
						QMessageBox::Ok,
						Qt::NoButton,
						Qt::NoButton);
				songChanged(-1);
				return;
			}
			dev->setName(s);
			song->update();
		}
			break;
		default:
			//printf("MPConfig::mdevViewItemRenamed unknown column clicked col:%d txt:%s\n", col, s.toLatin1().constData());
			break;
	}
}

//---------------------------------------------------------
//   rbClicked
//---------------------------------------------------------

void MPConfig::rbClicked(QTableWidgetItem* item)
{
	if (item == 0)
		return;
	QString id = item->tableWidget()->item(item->row(), DEVCOL_NO)->text();
	int no = atoi(id.toLatin1().constData()) - 1;
	if (no < 0 || no >= MIDI_PORTS)
		return;

	int n;
	MidiPort* port = &midiPorts[no];
	MidiDevice* dev = port->device();
	int rwFlags = dev ? dev->rwFlags() : 0;
	int openFlags = dev ? dev->openFlags() : 0;
	QTableWidget* listView = item->tableWidget();
	QPoint ppt = listView->visualItemRect(item).bottomLeft();
	QPoint mousepos = QCursor::pos();

	int col = item->column();
	ppt += QPoint(0, listView->horizontalHeader()->height());

	ppt = listView->mapToGlobal(ppt);

	switch (col)
	{
		case DEVCOL_GUI:
			if (dev == 0)
				//break;
				return;
            // falkTX, we don't want this in the connections manager
            //if (port->hasGui())
            //{
            //	port->instrument()->showGui(!port->guiVisible());
            //	item->setIcon(port->guiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
            //}
			//break;
			return;

		case DEVCOL_CACHE_NRPN:
			if (!dev)
				return;
			dev->setCacheNRPN(!dev->cacheNRPN());
			item->setIcon(dev->cacheNRPN() ? QIcon(*dotIcon) : QIcon(*dothIcon));

			return;

		case DEVCOL_REC:
			if (dev == 0 || !(rwFlags & 2))
				return;
			openFlags ^= 0x2;
			dev->setOpenFlags(openFlags);
			midiSeq->msgSetMidiDevice(port, dev); // reopen device
			item->setIcon(openFlags & 2 ? QIcon(*dotIcon) : QIcon(*dothIcon));

			if (dev->deviceType() == MidiDevice::JACK_MIDI)
			{
				if (dev->openFlags() & 2)
				{
					item->tableWidget()->item(item->row(), DEVCOL_INROUTES)->setText(tr("in"));
				}
				else
				{
					item->tableWidget()->item(item->row(), DEVCOL_INROUTES)->setText("");
				}
			}

			return;

		case DEVCOL_PLAY:
			if (dev == 0 || !(rwFlags & 1))
				return;
			openFlags ^= 0x1;
			dev->setOpenFlags(openFlags);
			midiSeq->msgSetMidiDevice(port, dev); // reopen device
			item->setIcon(openFlags & 1 ? QIcon(*dotIcon) : QIcon(*dothIcon));

			if (dev->deviceType() == MidiDevice::JACK_MIDI)
			{
				if (dev->openFlags() & 1)
				{
					item->tableWidget()->item(item->row(), DEVCOL_OUTROUTES)->setText(tr("out"));
				}
				else
				{
					item->tableWidget()->item(item->row(), DEVCOL_OUTROUTES)->setText("");
				}
			}

			return;

		case DEVCOL_INROUTES:
		case DEVCOL_OUTROUTES:
		{
			if (!checkAudioDevice())
				return;

			if (audioDevice->deviceType() != AudioDevice::JACK_AUDIO) //Only if Jack is running.
				return;

			if (!dev)
				return;

			// Only Jack midi devices.
			if (dev->deviceType() != MidiDevice::JACK_MIDI)
				return;

			if (!(dev->openFlags() & ((col == DEVCOL_OUTROUTES) ? 1 : 2)))
				return;

			RouteList* rl = (col == DEVCOL_OUTROUTES) ? dev->outRoutes() : dev->inRoutes(); // p3.3.55
			QMenu* pup = 0;
			int gid = 0;
			std::list<QString> sl;
			pup = new QMenu(this);
			//A temporary Route to us for matching later
			QString currentRoute("");
			bool routeSelected = false;

_redisplay:
			pup->clear();
			gid = 0;

			// Jack input ports if device is writable, and jack output ports if device is readable.
			sl = (col == DEVCOL_OUTROUTES) ? audioDevice->inputPorts(true, _showAliases) : audioDevice->outputPorts(true, _showAliases);

			QAction* act;

			act = pup->addAction(tr("Show first aliases"));
			act->setData(gid);
			act->setCheckable(true);
			act->setChecked(_showAliases == 0);
			++gid;

			act = pup->addAction(tr("Show second aliases"));
			act->setData(gid);
			act->setCheckable(true);
			act->setChecked(_showAliases == 1);
			++gid;

			pup->addSeparator();
			for (std::list<QString>::iterator ip = sl.begin(); ip != sl.end(); ++ip)
			{
				act = pup->addAction(*ip);
				act->setData(gid);
				act->setCheckable(true);

				Route rt(*ip, (col == DEVCOL_OUTROUTES), -1, Route::JACK_ROUTE);
				for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
				{
					if (*ir == rt)
					{
						currentRoute = (*ir).name();
						act->setChecked(true);
						routeSelected = true;
						break;
					}
				}
				++gid;
			}

			act = pup->exec(ppt);
			if (act)
			{
				n = act->data().toInt();
				if (n == 0) // Show first aliases
				{
					if (_showAliases == 0)
						_showAliases = -1;
					else
						_showAliases = 0;
					goto _redisplay; // Go back
				}
				else if (n == 1) // Show second aliases
				{
					if (_showAliases == 1)
						_showAliases = -1;
					else
						_showAliases = 1;
					goto _redisplay; // Go back
				}

				QString s(act->text());

				if (col == DEVCOL_OUTROUTES) // Writable 
				{
					Route srcRoute(dev, -1);
					Route dstRoute(s, true, -1, Route::JACK_ROUTE);
					if(routeSelected && currentRoute == s)
					{
						//it will alread be handled by an unchecked operation
						routeSelected = false;
					}

					iRoute iir = rl->begin();
					for (; iir != rl->end(); ++iir)
					{
						if (*iir == dstRoute)
							break;
					}
					
					if (iir != rl->end())
					{
						// disconnect
						audio->msgRemoveRoute(srcRoute, dstRoute);
					}
					else
					{
						// connect
						audio->msgAddRoute(srcRoute, dstRoute);
					}
					if(routeSelected)
					{
						iRoute selr = rl->begin();
						for (; selr != rl->end(); ++selr)
						{
							//clean up the routing list as something was selected that was not the current so delete the old route
							if((*selr).name() == currentRoute)
							{
								audio->msgRemoveRoute(srcRoute, (*selr));
								break;
							}
						}
					}

				}
				else
				{
					Route srcRoute(s, false, -1, Route::JACK_ROUTE);
					Route dstRoute(dev, -1);

					iRoute iir = rl->begin();
					for (; iir != rl->end(); ++iir)
					{
						if (*iir == srcRoute)
							break;
					}
					if (iir != rl->end())
						// disconnect
						audio->msgRemoveRoute(srcRoute, dstRoute);
					else
						// connect
						audio->msgAddRoute(srcRoute, dstRoute);
				}

				audio->msgUpdateSoloStates();
				song->update(SC_ROUTE);
			}

			delete pup;
		}
			return;

		case DEVCOL_DEF_IN_CHANS:
		case DEVCOL_DEF_OUT_CHANS:
		{
		}
			//break;
			return;

		case DEVCOL_NAME:
		{
			//printf("MPConfig::rbClicked DEVCOL_NAME\n");

			// Did we click in the text area?
			// NOTE: this needs the +15 pixels to make up for padding in the stylesheet.
			if ((mousepos.x() - (ppt.x() + 15)) > buttondownIcon->width())
			{
				// Start the renaming of the cell...
				QModelIndex current = item->tableWidget()->currentIndex();
				if (item->flags() & Qt::ItemIsEditable)
					item->tableWidget()->edit(current.sibling(current.row(), DEVCOL_NAME));

				return;
			}
			else
			{// We clicked the 'down' button.
				QMenu* pup = new QMenu(this);

				QAction* act;

				act = pup->addAction(tr("Create") + QT_TRANSLATE_NOOP("@default", " Jack") + tr(" device"));
				act->setData(0);

				typedef std::map<std::string, int > asmap;
				typedef std::map<std::string, int >::iterator imap;

				asmap mapALSA;
				asmap mapJACK;
				asmap mapSYNTH;

				int aix = 0x10000000;
				int jix = 0x20000000;
				int six = 0x30000000;
				for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
				{
					if ((*i)->deviceType() == MidiDevice::ALSA_MIDI)
					{
						mapALSA.insert(std::pair<std::string, int> (std::string((*i)->name().toLatin1().constData()), aix));
						++aix;
					}
					else if ((*i)->deviceType() == MidiDevice::JACK_MIDI)
					{
						mapJACK.insert(std::pair<std::string, int> (std::string((*i)->name().toLatin1().constData()), jix));
						++jix;
					}
					else if ((*i)->deviceType() == MidiDevice::SYNTH_MIDI)
					{
						mapSYNTH.insert(std::pair<std::string, int> (std::string((*i)->name().toLatin1().constData()), six));
						++six;
					}
					else
						printf("MPConfig::rbClicked unknown midi device: %s\n", (*i)->name().toLatin1().constData());
				}

				pup->addSeparator();
				pup->addAction(new MenuTitleItem(QT_TRANSLATE_NOOP("@default", "ALSA:"), pup));

				for (imap i = mapALSA.begin(); i != mapALSA.end(); ++i)
				{
					int idx = i->second;
					QString s(i->first.c_str());
					MidiDevice* md = midiDevices.find(s, MidiDevice::ALSA_MIDI);
					if (md)
					{
						if (md->deviceType() != MidiDevice::ALSA_MIDI)
							continue;

						act = pup->addAction(QT_TRANSLATE_NOOP("@default", md->name()));
						act->setData(idx);
						act->setCheckable(true);
						act->setChecked(md == dev);
					}
				}

				if (!mapSYNTH.empty())
				{
					pup->addSeparator();
					pup->addAction(new MenuTitleItem(QT_TRANSLATE_NOOP("@default", "SYNTH:"), pup));

					for (imap i = mapSYNTH.begin(); i != mapSYNTH.end(); ++i)
					{
						int idx = i->second;
						QString s(i->first.c_str());
						MidiDevice* md = midiDevices.find(s, MidiDevice::SYNTH_MIDI);
						if (md)
						{
							if (md->deviceType() != MidiDevice::SYNTH_MIDI)
								continue;

							act = pup->addAction(QT_TRANSLATE_NOOP("@default", md->name()));
							act->setData(idx);
							act->setCheckable(true);
							act->setChecked(md == dev);
						}
					}
				}

				pup->addSeparator();
				pup->addAction(new MenuTitleItem(QT_TRANSLATE_NOOP("@default", "JACK:"), pup));

				for (imap i = mapJACK.begin(); i != mapJACK.end(); ++i)
				{
					int idx = i->second;
					QString s(i->first.c_str());
					MidiDevice* md = midiDevices.find(s, MidiDevice::JACK_MIDI);
					if (md)
					{
						if (md->deviceType() != MidiDevice::JACK_MIDI)
							continue;

						act = pup->addAction(QT_TRANSLATE_NOOP("@default", md->name()));
						act->setData(idx);
						act->setCheckable(true);
						act->setChecked(md == dev);
					}
				}

				act = pup->exec(ppt);
				if (!act)
				{
					delete pup;
					return;
				}

				n = act->data().toInt();
				//printf("MPConfig::rbClicked n:%d\n", n);

				MidiDevice* sdev = 0;
				if (n < 0x10000000)
				{
					delete pup;
					if (n <= 2) 
					{
						sdev = MidiJackDevice::createJackMidiDevice();

						if (sdev)
						{
							int of = 3;
							switch (n)
							{
								case 0: of = 0; //3; Set the open flags of the midiDevice this should be off by default
									break;
								case 1: of = 2;
									break;
								case 2: of = 1;
									break;
							}
							sdev->setOpenFlags(of);
						}
					}
				}
				else
				{
					int typ;
					if (n < 0x20000000)
						typ = MidiDevice::ALSA_MIDI;
					else
						if (n < 0x30000000)
						typ = MidiDevice::JACK_MIDI;
					else
						typ = MidiDevice::SYNTH_MIDI;

					sdev = midiDevices.find(act->text(), typ);
					delete pup;
                    
					// Is it the current device? Reset it to <none>.
                    // falkTX, handle synths properly here
					if (sdev == dev)
                    {
                        if (typ == MidiDevice::SYNTH_MIDI)
                        {
                            SynthPluginDevice* synth = (SynthPluginDevice*)sdev;
                            if (synth->duplicated())
                            {
                                synth->close();
                                midiDevices.remove(sdev);
                                //delete synth;
                            }
                        }
						sdev = 0;
                    }
                    else
                    {
                        // create a new synth device if needed
                        if (typ == MidiDevice::SYNTH_MIDI)
                        {
                            SynthPluginDevice* oldSynth = (SynthPluginDevice*)sdev;
                            if (oldSynth->duplicated() == false)
                                sdev = oldSynth->clone();
                        }
                    }
				}

				midiSeq->msgSetMidiDevice(port, sdev);
				oom->changeConfig(true); // save configuration file
				song->update();
			}
		}
			//break;
			return;

		case DEVCOL_INSTR:
		{
            if (dev && dev->isSynthPlugin())
				//break;
				return;
			if (instrPopup == 0)
				instrPopup = new QMenu(this);
			instrPopup->clear();
			for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
			{
                instrPopup->addAction((*i)->iname());
			}

			QAction* act = instrPopup->exec(ppt, 0);
			if (!act)
				return;
			QString s = act->text();
			item->tableWidget()->item(item->row(), DEVCOL_INSTR)->setText(s);
			for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
			{
				if ((*i)->iname() == s)
				{
					port->setInstrument(*i);
					break;
				}
			}
			song->update();
		}
			return;
	}
}

//---------------------------------------------------------
//   MPConfig::setToolTip
//---------------------------------------------------------

void MPConfig::setToolTip(QTableWidgetItem *item, int col)
{
	switch (col)
	{
		case DEVCOL_NO: item->setToolTip(tr("Port Number"));
			break;
		case DEVCOL_GUI: item->setToolTip(tr("Enable gui"));
			break;
		case DEVCOL_CACHE_NRPN: item->setToolTip(tr("Enable caching of NRPN events before processing"));
			break;
		case DEVCOL_REC: item->setToolTip(tr("Enable reading"));
			break;
		case DEVCOL_PLAY: item->setToolTip(tr("Enable writing"));
			break;
		case DEVCOL_INSTR: item->setToolTip(tr("Port instrument"));
			break;
		case DEVCOL_NAME: item->setToolTip(tr("Midi device name. Click to edit (Jack)"));
			break;
			//case DEVCOL_ROUTES: item->setToolTip(tr("Jack midi ports")); break;
		case DEVCOL_INROUTES: item->setToolTip(tr("Connections from Jack Midi outputs"));
			break;
		case DEVCOL_OUTROUTES: item->setToolTip(tr("Connections to Jack Midi inputs"));
			break;
		case DEVCOL_DEF_IN_CHANS: item->setToolTip(tr("Connect these to new midi tracks"));
			break;
		case DEVCOL_DEF_OUT_CHANS: item->setToolTip(tr("Connect new midi tracks to this (first listed only)"));
			break;
		case DEVCOL_STATE: item->setToolTip(tr("Device state"));
			break;
		default: return;
	}
}

//---------------------------------------------------------
//   MPConfig::setWhatsThis
//---------------------------------------------------------

void MPConfig::setWhatsThis(QTableWidgetItem *item, int col)
{
	switch (col)
	{
		case DEVCOL_NO:
			item->setWhatsThis(tr("Port Number"));
			break;
		case DEVCOL_GUI:
			item->setWhatsThis(tr("Enable gui for device"));
			break;
		case DEVCOL_REC:
			item->setWhatsThis(tr("Enable reading from device"));
			break;
		case DEVCOL_PLAY:
			item->setWhatsThis(tr("Enable writing to device"));
			break;
		case DEVCOL_NAME:
			item->setWhatsThis(tr("Name of the midi device associated with"
					" this port number. Click to edit Jack midi name."));
			break;
		case DEVCOL_INSTR:
			item->setWhatsThis(tr("Instrument connected to port"));
			break;
			//case DEVCOL_ROUTES:
			//      item->setWhatsThis(tr("Jack midi ports")); break;
		case DEVCOL_INROUTES:
			item->setWhatsThis(tr("Connections from Jack Midi output ports"));
			break;
		case DEVCOL_OUTROUTES:
			item->setWhatsThis(tr("Connections to Jack Midi input ports"));
			break;
		case DEVCOL_DEF_IN_CHANS:
			item->setWhatsThis(tr("Connect these channels, on this port, to new midi tracks.\n"
					"Example:\n"
					" 1 2 3    channel 1 2 and 3\n"
					" 1-3      same\n"
					" 1-3 5    channel 1 2 3 and 5\n"
					" all      all channels\n"
					" none     no channels"));
			break;
		case DEVCOL_DEF_OUT_CHANS:
			item->setWhatsThis(tr("Connect new midi tracks to these channels, on this port.\n"
					"See default in channels.\n"
					"NOTE: Currently only one output port and channel supported (first found)"));
			break;
		case DEVCOL_STATE:
			item->setWhatsThis(tr("State: result of opening the device"));
			break;
		default:
			break;
	}
}


//---------------------------------------------------------
//   MPConfig::addItem()
//---------------------------------------------------------

void MPConfig::addItem(int row, int col, QTableWidgetItem *item, QTableWidget *table)
{
	setWhatsThis(item, col);
	table->setItem(row, col, item);
}


//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void MPConfig::selectionChanged()
{
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MPConfig::songChanged(int flags)
{
	// Is it simply a midi controller value adjustment? Forget it.
	if (flags == SC_MIDI_CONTROLLER)
		return;

	// Get currently selected index...
	int no = -1;
	QTableWidgetItem* sitem = mdevView->currentItem();
	if (sitem)
	{
		QString id = sitem->tableWidget()->item(sitem->row(), DEVCOL_NO)->text();
		no = atoi(id.toLatin1().constData()) - 1;
		if (no < 0 || no >= MIDI_PORTS)
			no = -1;
	}

	sitem = 0;
	for (int i = MIDI_PORTS - 1; i >= 0; --i)
	{
		mdevView->blockSignals(true); // otherwise itemChanged() is triggered and bad things happen.
		MidiPort* port = &midiPorts[i];
		MidiDevice* dev = port->device();
		QString s;
		s.setNum(i + 1);
		QTableWidgetItem* itemno = mdevView->item(i, DEVCOL_NO);
		QTableWidgetItem* itemstate = mdevView->item(i, DEVCOL_STATE);
		itemstate->setText(port->state());
		QTableWidgetItem* iteminstr = mdevView->item(i, DEVCOL_INSTR);
		QString instrumentName = port->instrument() ? port->instrument()->iname() : tr("<unknown>");
		iteminstr->setText(instrumentName);
		iteminstr->setToolTip(instrumentName);
		QTableWidgetItem* itemname = mdevView->item(i, DEVCOL_NAME);
		QTableWidgetItem* itemgui = mdevView->item(i, DEVCOL_GUI);
		QTableWidgetItem* itemfb = mdevView->item(i, DEVCOL_CACHE_NRPN);
		QTableWidgetItem* itemrec = mdevView->item(i, DEVCOL_REC);
		QTableWidgetItem* itemplay = mdevView->item(i, DEVCOL_PLAY);
		QTableWidgetItem* itemout = mdevView->item(i, DEVCOL_OUTROUTES);
		QTableWidgetItem* itemin = mdevView->item(i, DEVCOL_INROUTES);
		QTableWidgetItem* itemdefin = mdevView->item(i, DEVCOL_DEF_IN_CHANS);
		itemdefin->setText(bitmap2String(port->defaultInChannels()));
		QTableWidgetItem* itemdefout = mdevView->item(i, DEVCOL_DEF_OUT_CHANS);
		itemdefout->setText(bitmap2String(port->defaultOutChannels()));
		mdevView->blockSignals(false);


		if (dev)
		{
			itemname->setText(dev->name());
			itemname->setToolTip(dev->name());

			// Is it a Jack midi device? Allow renaming.
			//if(dynamic_cast<MidiJackDevice*>(dev))
			if (dev->deviceType() == MidiDevice::JACK_MIDI)
				itemname->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled);

			if (dev->rwFlags() & 0x2)
			{
				itemrec->setIcon(dev->openFlags() & 2 ? QIcon(*dotIcon) : QIcon(*dothIcon));
			}
			else
			{
				itemrec->setIcon(QIcon(QPixmap()));
			}

			if (dev->rwFlags() & 0x1)
			{
				itemplay->setIcon(dev->openFlags() & 1 ? QIcon(*dotIcon) : QIcon(*dothIcon));
			}
			else
				itemplay->setIcon(QIcon(QPixmap()));
			itemfb->setIcon(dev->cacheNRPN() ? QIcon(*dotIcon) : QIcon(*dothIcon));
		}
		else
		{
			itemname->setText(tr("<none>"));
			itemname->setToolTip("");
			itemgui->setIcon(QIcon(*dothIcon));
			itemrec->setIcon(QIcon(QPixmap()));
			itemplay->setIcon(QIcon(QPixmap()));
            itemfb->setIcon(QIcon(QPixmap()));
		}
        // falkTX, we don't want this in the connections manager
        //if (port->hasGui())
        //{
        //	itemgui->setIcon(port->guiVisible() ? QIcon(*dotIcon) : QIcon(*dothIcon));
        //}
        //else
        //{
			itemgui->setIcon(QIcon(QPixmap()));
        //}
        if (!(dev && dev->isSynthPlugin()))
			iteminstr->setIcon(QIcon(*buttondownIcon));
		itemname->setIcon(QIcon(*buttondownIcon));


		//if(dev && dynamic_cast<MidiJackDevice*>(dev))
		if (dev && dev->deviceType() == MidiDevice::JACK_MIDI)
		{
			//item->setPixmap(DEVCOL_ROUTES, *buttondownIcon);
			//item->setText(DEVCOL_ROUTES, tr("routes"));

			// p3.3.55
			if (dev->rwFlags() & 1)
				//if(dev->openFlags() & 1)
			{
				itemout->setIcon(QIcon(*buttondownIcon));
				if (port->device() && !port->device()->outRoutes()->empty())
				{
					RouteList* list = port->device()->outRoutes();
					if (!list->empty())
					{
						iRoute r = list->begin();
						itemout->setText(r->name());
					}
				}
				else
				{
					itemout->setText(tr("out"));
				}

				//if (dev->openFlags() & 1)
				//	itemout->setText(tr("out"));
			}
			if (dev->rwFlags() & 2)
				//if(dev->openFlags() & 2)
			{
				itemin->setIcon(QIcon(*buttondownIcon));
				if (dev->openFlags() & 2)
					itemin->setText(tr("in"));
			}
		}

		if (i == no) sitem = itemno;
	}

	if (sitem)
	{
		mdevView->setCurrentItem(sitem);
	}
}

//---------------------------------------------------------
//   addInstanceClicked
//---------------------------------------------------------

void MPConfig::addInstanceClicked()
{
}

//---------------------------------------------------------
//   removeInstanceClicked
//---------------------------------------------------------

void MPConfig::removeInstanceClicked()
{
}


