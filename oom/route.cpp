//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: route.cpp,v 1.18.2.3 2008/05/21 00:28:52 terminator356 Exp $
//
//  (C) Copyright 2003-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <QWidget>

#include "song.h"
#include "route.h"
#include "node.h"
#include "audio.h"
#include "track.h"
#include "audiodev.h"
#include "xml.h"
#include "globals.h"
#include "midiport.h"
#include "driver/jackmidi.h"
#include "driver/alsamidi.h"

//#define ROUTE_DEBUG 

//#define ROUTE_MIDIPORT_NAME_PREFIX       "OOMidi MidiPort "
const QString ROUTE_MIDIPORT_NAME_PREFIX = "OOMidi MidiPort ";

//---------------------------------------------------------
//   Route
//---------------------------------------------------------

//Jack Type Route
Route::Route(void* t, int ch)
{
	jackPort = t;
	midiPort = -1;
	channel = ch;
	channels = -1;
	trackId = -1;
	remoteChannel = -1;
	type = JACK_ROUTE;
}

//Track to Track Route
Route::Route(Track* t, int ch, int chans)
{
	track = t;
	if(t)
		trackId = t->id();
	midiPort = -1;
	channel = ch;
	channels = chans;
	remoteChannel = -1;
	type = TRACK_ROUTE;
}

Route::Route(qint64 t, int ch, int chans)
{
	track = song->findTrackById(t);
	trackId = t;
	midiPort = -1;
	channel = ch;
	channels = chans;
	remoteChannel = -1;
	type = TRACK_ROUTE;
}

//Midi Device Route
Route::Route(MidiDevice* d, int ch)
{
	device = d;
	midiPort = -1;
	trackId = -1;
	channel = ch;
	channels = -1;
	remoteChannel = -1;
	type = MIDI_DEVICE_ROUTE;
}

//Midi Port Route
Route::Route(int port, int ch)
{
	track = 0;
	midiPort = port;
	channel = ch;
	channels = -1;
	remoteChannel = -1;
	type = MIDI_PORT_ROUTE;
	trackId = -1;
	if(midiPort >= 0 && midiPort < MIDI_PORTS)
	{
		MidiPort* mp = &midiPorts[midiPort];
		midiPortId = mp->id();
	}
}

Route::Route(qint64 port, int ch)
{
	track = 0;
	trackId = -1;
	midiPortId = port;
	if(oomMidiPorts.contains(port))
		midiPort = oomMidiPorts.value(port)->portno();
	channel = ch;
	channels = -1;
	remoteChannel = -1;
	type = MIDI_PORT_ROUTE;
}

//Route From settings
//Used mostly for restoring routes from the xml file
Route::Route(const QString& s, bool dst, int ch, int rtype)
{
	trackId = -1;
	midiPort = -1;
	midiPortId = -1;
	Route node(name2route(s, dst, rtype));
	channel = node.channel;
	if (channel == -1)
		channel = ch;
	channels = node.channels;
	remoteChannel = node.remoteChannel;
	type = node.type;
	if (type == TRACK_ROUTE)
	{
		track = node.track;
		if(track)
			trackId = track->id();
	}
	else if (type == JACK_ROUTE)
	{
		jackPort = node.jackPort;
	}
	else if (type == MIDI_DEVICE_ROUTE)
	{
		device = node.device;
	}
	else if (type == MIDI_PORT_ROUTE)
	{
		track = 0;
		midiPort = node.midiPort;
		if(midiPort >= 0 && midiPort < MIDI_PORTS)
		{
			MidiPort* mp = &midiPorts[midiPort];
			midiPortId = mp->id();
		}
	}
}

//Default Track to Track Route
//This creates a Null route that must be configured to be used
Route::Route()
{
	track = 0;
	trackId = -1;
	midiPort = -1;
	channel = -1;
	channels = -1;
	remoteChannel = -1;
	type = TRACK_ROUTE;
}

//---------------------------------------------------------
//   addRoute
//---------------------------------------------------------

void addRoute(Route src, Route dst)/*{{{*/
{
//TODO: Add hooks to update the patchcanvas on success
//This should be conditionally checked so we dont update if the canvas is what made the connection

#ifdef ROUTE_DEBUG
	fprintf(stderr, "addRoute:\n");
#endif

	if (!src.isValid() || !dst.isValid())
	{
		if (!src.isValid())
			fprintf(stderr, "addRoute: invalid src\n");
		if (!dst.isValid())
			fprintf(stderr, "addRoute: invalid dst\n");
		return;
	}

	if (src.type == Route::JACK_ROUTE)
	{
		if (dst.type == Route::TRACK_ROUTE)
		{
			if (dst.track->type() != Track::AUDIO_INPUT)
			{
				fprintf(stderr, "addRoute: source is jack, dest:%s is track but not audio input\n", dst.track->name().toLatin1().constData());
				return;
			}
			if (dst.channel < 0)
			{
				fprintf(stderr, "addRoute: source is jack, dest:%s is track but invalid channel:%d\n", dst.track->name().toLatin1().constData(), dst.channel);
				return;
			}

			src.channel = dst.channel;
			RouteList* inRoutes = dst.track->inRoutes();
			for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i)
			{
				if (*i == src) // route already there
				{
					fprintf(stderr, "addRoute: src track route already exists.\n");
					return;
				}
			}
#ifdef ROUTE_DEBUG
			fprintf(stderr, "addRoute: src Jack dst track name: %s pushing source route\n", dst.track->name().toLatin1().constData());
#endif
			inRoutes->push_back(src);
		}
		else if (dst.type == Route::MIDI_DEVICE_ROUTE)
		{
			if (dst.device->deviceType() == MidiDevice::JACK_MIDI)
			{
				src.channel = dst.channel;

				RouteList* routes = dst.device->inRoutes();
				for (iRoute i = routes->begin(); i != routes->end(); ++i)
				{
					if (*i == src) // route already there
					{
						fprintf(stderr, "addRoute: src Jack midi route already exists.\n");
						return;
					}
				}
#ifdef ROUTE_DEBUG
				fprintf(stderr, "addRoute: src Jack dst Jack midi name: %s pushing source route\n", dst.device->name().toLatin1().constData());
#endif
				routes->push_back(src);
			}
			else
			{
				fprintf(stderr, "addRoute: source is Jack, but destination is not jack midi - type:%d\n", dst.device->deviceType());
				return;
			}
		}
		else
		{
			fprintf(stderr, "addRoute: source is Jack, but destination is not track or midi - type:%d \n", dst.type);
			return;
		}
	}
	else if (dst.type == Route::JACK_ROUTE)
	{
		if (src.type == Route::TRACK_ROUTE)
		{
			if (src.track->type() != Track::AUDIO_OUTPUT)
			{
				fprintf(stderr, "addRoute: destination is jack, source is track but not audio output\n");
				return;
			}
			if (src.channel < 0)
			{
				fprintf(stderr, "addRoute: destination is jack, source:%s is track but invalid channel:%d\n", src.track->name().toLatin1().constData(), src.channel);
				return;
			}

			RouteList* outRoutes = src.track->outRoutes();
			dst.channel = src.channel;

			for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i)
			{
				if (*i == dst) // route already there
				{
#ifdef ROUTE_DEBUG
					fprintf(stderr, "addRoute: dst track route already exists.\n");
#endif
					return;
				}
			}
#ifdef ROUTE_DEBUG
			fprintf(stderr, "addRoute: dst Jack src track name: %s pushing destination route\n", src.track->name().toLatin1().constData());
#endif
			outRoutes->push_back(dst);
		}
		else if (src.type == Route::MIDI_DEVICE_ROUTE)
		{
			if (src.device->deviceType() == MidiDevice::JACK_MIDI)
			{
				dst.channel = src.channel;

				RouteList* routes = src.device->outRoutes();
				for (iRoute i = routes->begin(); i != routes->end(); ++i)
				{
					if (*i == dst) // route already there
					{
						fprintf(stderr, "addRoute: dst Jack midi route already exists.\n");
						return;
					}
				}
#ifdef ROUTE_DEBUG
				fprintf(stderr, "addRoute: dst Jack src Jack midi name: %s pushing destination route\n", src.device->name().toLatin1().constData());
#endif
				routes->push_back(dst);
			}
			else
			{
				fprintf(stderr, "addRoute: destination is Jack, but source is not jack midi - type:%d\n", src.device->deviceType());
				return;
			}
		}
		else
		{
			fprintf(stderr, "addRoute: destination is Jack, but source is not track or midi - type:%d \n", src.type);
			return;
		}
	}
	else if (src.type == Route::MIDI_PORT_ROUTE) // p3.3.49
	{
		if (dst.type != Route::TRACK_ROUTE)
		{
			fprintf(stderr, "addRoute: source is midi port:%d, but destination is not track\n", src.midiPort);
			return;
		}
		if (dst.channel < 1 || dst.channel >= (1 << MIDI_CHANNELS))
		{
			fprintf(stderr, "addRoute: source is midi port:%d, but destination channel mask:%d out of range\n", src.midiPort, dst.channel);
			return;
		}

		MidiPort *mp = &midiPorts[src.midiPort];

		src.channel = dst.channel;
		RouteList* outRoutes = mp->outRoutes();
		iRoute ir = outRoutes->begin(); // p3.3.50
		for (; ir != outRoutes->end(); ++ir)
		{
			if (ir->type == Route::TRACK_ROUTE && ir->track == dst.track) // p3.3.50 Does a route to the track exist?
			{
				ir->channel |= dst.channel; // p3.3.50 Bitwise OR the desired channel bit with the existing bit mask.
				break;
			}
		}
#ifdef ROUTE_DEBUG
		fprintf(stderr, "addRoute: src midi port:%d dst track name:%s pushing dst and src routes\n", src.midiPort, dst.track->name().toLatin1().constData());
#endif

		if (ir == outRoutes->end()) // p3.3.50 Only if route not found, add the route, with the requested channel bits as mask to start with.
			outRoutes->push_back(dst);

		RouteList* inRoutes = dst.track->inRoutes();

		// p3.3.50 Make sure only one single route, with a channel mask, can ever exist.
		ir = inRoutes->begin();
		for (; ir != inRoutes->end(); ++ir)
		{
			if (ir->type == Route::MIDI_PORT_ROUTE && ir->midiPort == src.midiPort) // p3.3.50 Does a route to the midi port exist?
			{
				ir->channel |= src.channel; // p3.3.50 Bitwise OR the desired channel bit with the existing bit mask.
				break;
			}
		}

		if (ir == inRoutes->end()) // p3.3.50 Only if route not found, add the route, with the requested channel bits as mask to start with.
			inRoutes->push_back(src);
	}
	else if (dst.type == Route::MIDI_PORT_ROUTE) // p3.3.49
	{
		if (src.type != Route::TRACK_ROUTE)
		{
			fprintf(stderr, "addRoute: destination is midi port:%d, but source is not track\n", dst.midiPort);
			return;
		}
		if (src.channel < 1 || src.channel >= (1 << MIDI_CHANNELS))
		{
			fprintf(stderr, "addRoute: destination is midi port:%d, but source channel mask:%d out of range\n", dst.midiPort, src.channel);
			return;
		}

		dst.channel = src.channel;
		RouteList* outRoutes = src.track->outRoutes();

		iRoute ir = outRoutes->begin(); // p3.3.50
		for (; ir != outRoutes->end(); ++ir)
		{
			if (ir->type == Route::MIDI_PORT_ROUTE && ir->midiPort == dst.midiPort) // p3.3.50 Does a route to the midi port exist?
			{
				ir->channel |= dst.channel; // p3.3.50 Bitwise OR the desired channel bit with the existing bit mask.
				break;
			}
		}

		if (ir == outRoutes->end()) // p3.3.50 Only if route not found, add the route, with the requested channel bits as mask to start with.
			outRoutes->push_back(dst);

		MidiPort *mp = &midiPorts[dst.midiPort];

#ifdef ROUTE_DEBUG
		fprintf(stderr, "addRoute: src track:%s dst midi port:%d pushing dst and src routes\n", src.track->name().toLatin1().constData(), dst.midiPort);
#endif
		RouteList* inRoutes = mp->inRoutes();

		// p3.3.50 Make sure only one single route, with a channel mask, can ever exist.
		ir = inRoutes->begin();
		for (; ir != inRoutes->end(); ++ir)
		{
			if (ir->type == Route::TRACK_ROUTE && ir->track == src.track) // p3.3.50 Does a route to the track exist?
			{
				ir->channel |= src.channel; // p3.3.50 Bitwise OR the desired channel bit with the existing bit mask.
				break;
			}
		}

		if (ir == inRoutes->end()) // p3.3.50 Only if route not found, add the route, with the requested channel bits as mask to start with.
			inRoutes->push_back(src);
	}
	else
	{
		if (src.type != Route::TRACK_ROUTE || dst.type != Route::TRACK_ROUTE) // p3.3.49
		{
			fprintf(stderr, "addRoute: source or destination are not track routes\n");
			return;
		}

		{
			RouteList* outRoutes = src.track->outRoutes();

			//
			// Must enforce to ensure channel and channels are valid if defaults of -1 passed.
			//
			if (src.track->type() == Track::AUDIO_SOFTSYNTH)
			{
				if (src.channel == -1)
					src.channel = 0;
				if (src.channels == -1)
					src.channels = src.track->channels();
				dst.channel = src.channel;
				dst.channels = src.channels;
				dst.remoteChannel = src.remoteChannel;
			}

			for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i)
			{
				if (*i == dst) // route already there
				{
					//#ifdef ROUTE_DEBUG
					fprintf(stderr, "addRoute: src track route already exists.\n");
					//#endif
					return;
				}
			}
			outRoutes->push_back(dst);
			RouteList* inRoutes;

			{
#ifdef ROUTE_DEBUG
				fprintf(stderr, "addRoute: src track ch:%d chs:%d remch:%d  dst track ch:%d chs:%d remch:%d name: %s pushing dest and source routes\n",
						src.channel, src.channels, src.remoteChannel, dst.channel, dst.channels, dst.remoteChannel, dst.track->name().toLatin1().constData());
#endif
				inRoutes = dst.track->inRoutes();
			}


			//
			// make sure AUDIO_AUX is processed last
			//
			if (src.track->type() == Track::AUDIO_AUX)
				inRoutes->push_back(src);
			else
				inRoutes->insert(inRoutes->begin(), src);
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void removeRoute(Route src, Route dst)/*{{{*/
{
//TODO: Add hooks to update the patchcanvas on success
//This should be conditionally checked so we dont update if the canvas is what made the connection
	if (src.type == Route::JACK_ROUTE)
	{
		if (!dst.isValid())
		{
			printf("removeRoute: source is jack, invalid destination\n");
			return;
		}

		if (dst.type == Route::TRACK_ROUTE)
		{
			if (dst.track->type() != Track::AUDIO_INPUT)
			{
				fprintf(stderr, "removeRoute: source is jack, destination is track but not audio input\n");
				// exit(-1);
				return;
			}
			RouteList* inRoutes = dst.track->inRoutes();
			iRoute i;
			for (i = inRoutes->begin(); i != inRoutes->end(); ++i)
			{
				if (*i == src)
				{
					inRoutes->erase(i);
					break;
				}
			}
		}
		else if (dst.type == Route::MIDI_DEVICE_ROUTE)
		{
			RouteList* routes = dst.device->inRoutes();
			iRoute i;
			for (i = routes->begin(); i != routes->end(); ++i)
			{
				if (*i == src)
				{
					routes->erase(i);
					break;
				}
			}
		}
		else
		{
			fprintf(stderr, "removeRoute: source is jack, destination unknown\n");
			return;
		}
	}
	else if (dst.type == Route::JACK_ROUTE)
	{
		if (!src.isValid())
		{
			printf("removeRoute: destination is jack, invalid source\n");
			return;
		}

		if (src.type == Route::TRACK_ROUTE)
		{
			if (src.track->type() != Track::AUDIO_OUTPUT)
			{
				fprintf(stderr, "removeRoute: destination is jack, source is track but not audio output\n");
				return;
			}
			RouteList* outRoutes = src.track->outRoutes();
			iRoute i;
			for (i = outRoutes->begin(); i != outRoutes->end(); ++i)
			{
				if (*i == dst)
				{
					outRoutes->erase(i);
					break;
				}
			}
		}
		else if (src.type == Route::MIDI_DEVICE_ROUTE)
		{
			RouteList* routes = src.device->outRoutes();
			iRoute i;
			for (i = routes->begin(); i != routes->end(); ++i)
			{
				if (*i == dst)
				{
					routes->erase(i);
					break;
				}
			}
		}
		else
		{
			fprintf(stderr, "removeRoute: destination is jack, source unknown\n");
			// exit(-1);
			return;
		}
	}
	else if (src.type == Route::MIDI_PORT_ROUTE) // p3.3.49
	{
		if (dst.type != Route::TRACK_ROUTE)
		{
			fprintf(stderr, "removeRoute: source is midi port:%d, but destination is not track\n", src.midiPort);
			return;
		}

		if (src.isValid())
		{
			MidiPort *mp = &midiPorts[src.midiPort];
			RouteList* outRoutes = mp->outRoutes();
			for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i)
			{
				//if (*i == dst)
				if (i->type == Route::TRACK_ROUTE && i->track == dst.track) // p3.3.50 Is there a route to the track?
				{
					i->channel &= ~dst.channel; // p3.3.50 Unset the desired channel bits.
					if (i->channel == 0) // Only if there are no channel bits set, erase the route.
						outRoutes->erase(i);

					break; // For safety, keep looking and remove any more found.
					// No, must break, else crash. There should only be one route anyway...
				}
			}
		}
		else
			printf("removeRoute: source is midi port:%d but invalid\n", src.midiPort);

		if (dst.isValid())
		{
			RouteList* inRoutes = dst.track->inRoutes();
			for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i)
			{
				if (i->type == Route::MIDI_PORT_ROUTE && i->midiPort == src.midiPort) // p3.3.50 Is there a route to the midi port?
				{
					i->channel &= ~src.channel; // p3.3.50 Unset the desired channel bits.
					if (i->channel == 0) // Only if there are no channel bits set, erase the route.
						inRoutes->erase(i);

					break; // For safety, keep looking and remove any more found.
					// No, must break, else crash. There should only be one route anyway...
				}
			}
		}
		else
			printf("removeRoute: source is midi port:%d but destination track invalid\n", src.midiPort);
	}
	else if (dst.type == Route::MIDI_PORT_ROUTE) // p3.3.49
	{
		if (src.type != Route::TRACK_ROUTE)
		{
			fprintf(stderr, "removeRoute: destination is midi port:%d, but source is not track\n", dst.midiPort);
			return;
		}

		if (src.isValid())
		{
			RouteList* outRoutes = src.track->outRoutes();
			for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i)
			{
				if (i->type == Route::MIDI_PORT_ROUTE && i->midiPort == dst.midiPort) // p3.3.50 Is there a route to the midi port?
				{
					i->channel &= ~dst.channel; // p3.3.50 Unset the desired channel bits.
					if (i->channel == 0) // Only if there are no channel bits set, erase the route.
						outRoutes->erase(i);

					break; // For safety, keep looking and remove any more found.
					// No, must break, else crash. There should only be one route anyway...
				}
			}
		}
		else
			printf("removeRoute: destination is midi port:%d but source track is invalid\n", dst.midiPort);

		if (dst.isValid())
		{
			MidiPort *mp = &midiPorts[src.midiPort];
			RouteList* inRoutes = mp->inRoutes();
			for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i)
			{
				if (i->type == Route::TRACK_ROUTE && i->track == src.track) // p3.3.50 Is there a route to the track?
				{
					i->channel &= ~src.channel; // p3.3.50 Unset the desired channel bits.
					if (i->channel == 0) // Only if there are no channel bits set, erase the route.
						inRoutes->erase(i);

					break; // For safety, keep looking and remove any more found.
					// No, must break, else crash. There should only be one route anyway...
				}
			}
		}
		else
			printf("removeRoute: destination is midi port:%d but invalid\n", dst.midiPort);
	}
	else
	{
		if (src.type != Route::TRACK_ROUTE || dst.type != Route::TRACK_ROUTE) // p3.3.49
		{
			fprintf(stderr, "removeRoute: source and destination are not tracks\n");
			return;
		}

		if (src.isValid())
		{
			RouteList* outRoutes = src.track->outRoutes();
			for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i)
			{
				if ((*i).name() == dst.name())
				{
					outRoutes->erase(i);
					break;
				}
			}
		}
		else
			printf("removeRoute: source is track but invalid\n");

		if (dst.isValid())
		{
			RouteList* inRoutes = dst.track->inRoutes();
			for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i)
			{
				if ((*i).name() == src.name())
				{
					inRoutes->erase(i);
					break;
				}
			}
		}
		else
			printf("removeRoute: source is track but destination invalid\n");
	}
}/*}}}*/

//---------------------------------------------------------
//   removeAllRoutes
//   If src is valid, disconnects all output routes from it.
//   If dst is valid, disconnects all input routes to it.
//   src and dst Route are used SIMPLY because Route provides convenient way to 
//    specify the different pointer types (track, port, jack)
//   This routine will ONLY look at the pointer, not the channel or port etc...
//   So far it only works with MidiDevice <-> Jack.
//---------------------------------------------------------

void removeAllRoutes(Route src, Route dst)/*{{{*/
{
	if (src.isValid())
	{
		if (src.type == Route::MIDI_DEVICE_ROUTE)
			src.device->outRoutes()->clear();
		else
			printf("removeAllRoutes: source is not midi device\n");
	}

	if (dst.isValid())
	{
		if (dst.type == Route::MIDI_DEVICE_ROUTE)
			dst.device->inRoutes()->clear();
		else
			printf("removeAllRoutes: dest is not midi device\n");
	}
}/*}}}*/

//---------------------------------------------------------
//   track2name
//    create string name representation for audio node
//---------------------------------------------------------

static QString track2name(const Track* n)
{
	if (n == 0)
		return QWidget::tr("None");
	return n->name();
}

//---------------------------------------------------------
//   name
//    create string name representation for audio node
//---------------------------------------------------------

QString Route::name() const/*{{{*/
{
	if (type == MIDI_DEVICE_ROUTE)
	{
		if (device)
		{
			return device->name();
		}
		return QWidget::tr("None");
	}
	else if (type == JACK_ROUTE)
	{
		if (!checkAudioDevice()) return "";
		return audioDevice->portName(jackPort);
	}
	else if (type == MIDI_PORT_ROUTE)
	{
		return ROUTE_MIDIPORT_NAME_PREFIX + QString().setNum(midiPort);
	}
	else
		return track2name(track);
}/*}}}*/

//---------------------------------------------------------
//   name2route
//---------------------------------------------------------

Route name2route(const QString& rn, bool /*dst*/, int rtype)/*{{{*/
{
	// printf("name2route %s\n", rn.toLatin1().constData());
	int channel = -1;
	QString s(rn);
	// Support old route style in oom files. Obsolete.
	if (rn.size() >= 2 && rn[0].isNumber() && rn[1] == ':')
	{
		channel = rn[0].toAscii() - int('1');
		s = rn.mid(2);
	}

	if (rtype == -1)
	{
		if (checkAudioDevice())
		{
			void* p = audioDevice->findPort(s.toLatin1().constData());
			if (p)
				return Route(p, channel);
		}

		TrackList* tl = song->tracks();
		for (iTrack i = tl->begin(); i != tl->end(); ++i)
		{
			if ((*i)->isMidiTrack())
			{
				MidiTrack* track = (MidiTrack*) * i;
				if (track->name() == s)
					return Route(track, channel);
			}
			else
			{
				AudioTrack* track = (AudioTrack*) * i;
				if (track->name() == s)
					return Route(track, channel);
			}
		}

		for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
		{
			if ((*i)->name() == s)
				return Route(*i, channel);
		}

		// p3.3.49
		if (s.left(ROUTE_MIDIPORT_NAME_PREFIX.length()) == ROUTE_MIDIPORT_NAME_PREFIX)
		{
			bool ok = false;
			int port = s.mid(ROUTE_MIDIPORT_NAME_PREFIX.length()).toInt(&ok);
			if (ok)
				return Route(port, channel);
		}
	}
	else
	{
		if (rtype == Route::TRACK_ROUTE)
		{
			TrackList* tl = song->tracks();
			for (iTrack i = tl->begin(); i != tl->end(); ++i)
			{
				if ((*i)->isMidiTrack())
				{
					MidiTrack* track = (MidiTrack*) * i;
					if (track->name() == s)
						return Route(track, channel);
				}
				else
				{
					AudioTrack* track = (AudioTrack*) * i;
					if (track->name() == s)
						return Route(track, channel);
				}
			}
		}// TODO Distinguish the device types
		else if (rtype == Route::MIDI_DEVICE_ROUTE)
		{
			for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
			{
				if ((*i)->name() == s)
					return Route(*i, channel);
			}
		}
		else if (rtype == Route::JACK_ROUTE)
		{
			if (checkAudioDevice())
			{
				void* p = audioDevice->findPort(s.toLatin1().constData());
				if (p)
					return Route(p, channel);
			}
		}
		else if (rtype == Route::MIDI_PORT_ROUTE) // p3.3.49
		{
			if (s.left(ROUTE_MIDIPORT_NAME_PREFIX.length()) == ROUTE_MIDIPORT_NAME_PREFIX)
			{
				bool ok = false;
				int port = s.mid(ROUTE_MIDIPORT_NAME_PREFIX.length()).toInt(&ok);
				if (ok)
					return Route(port, channel);
			}
		}
	}

	printf("  name2route: <%s> not found\n", rn.toLatin1().constData());
	return Route((Track*) 0, channel);
}/*}}}*/

//---------------------------------------------------------
//   checkRoute
//    return true if route is valid
//---------------------------------------------------------

bool checkRoute(const QString& s, const QString& d)/*{{{*/
{
	Route src(s, false, -1);
	Route dst(d, true, -1);

	if (!(src.isValid() && dst.isValid()) || (src == dst))
		return false;
	if (src.type == Route::JACK_ROUTE)
	{
		if (dst.type == Route::TRACK_ROUTE)
		{
			if (dst.track->type() != Track::AUDIO_INPUT)
			{
				return false;
			}
			src.channel = dst.channel;
			RouteList* inRoutes = dst.track->inRoutes();
			for (iRoute i = inRoutes->begin(); i != inRoutes->end(); ++i)
			{
				if (*i == src)
				{ // route already there
					return false;
				}
			}
		}
		else if (dst.type == Route::MIDI_DEVICE_ROUTE)
		{
			src.channel = -1;
			RouteList* routes = dst.device->inRoutes();
			for (iRoute i = routes->begin(); i != routes->end(); ++i)
			{
				if (*i == src)
				{ // route already there
					return false;
				}
			}
		}
		else
			return false;
	}
	else if (dst.type == Route::JACK_ROUTE)
	{
		if (src.type == Route::TRACK_ROUTE)
		{
			if (src.track->type() != Track::AUDIO_OUTPUT)
			{
				return false;
			}
			RouteList* outRoutes = src.track->outRoutes();
			dst.channel = src.channel;
			for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i)
			{
				if (*i == dst)
				{ // route already there
					return false;
				}
			}
		}
		else if (src.type == Route::MIDI_DEVICE_ROUTE)
		{
			RouteList* routes = src.device->outRoutes();
			dst.channel = -1;
			for (iRoute i = routes->begin(); i != routes->end(); ++i)
			{
				if (*i == dst)
				{ // route already there
					return false;
				}
			}
		}
		else
			return false;
	}
	else if (src.type == Route::MIDI_PORT_ROUTE) // p3.3.49
	{
		RouteList* outRoutes = midiPorts[src.midiPort].outRoutes();
		for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i)
		{
			if (*i == dst)
			{ // route already there
				return false;
			}
		}
	}
	else
	{
		RouteList* outRoutes = (src.type == Route::MIDI_DEVICE_ROUTE) ? src.device->outRoutes() : src.track->outRoutes();

		for (iRoute i = outRoutes->begin(); i != outRoutes->end(); ++i)
		{
			if (*i == dst)
			{ // route already there
				return false;
			}
		}
	}
	return true;
}/*}}}*/

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Route::read(Xml& xml)/*{{{*/
{
	QString s;
	int dtype = MidiDevice::ALSA_MIDI;
	int port = -1;
	unsigned char rtype = Route::TRACK_ROUTE;

	for (;;)
	{
		const QString& tag = xml.s1();
		Xml::Token token = xml.parse();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::Attribut:
#ifdef ROUTE_DEBUG
				printf("Route::read(): attribute:%s\n", tag.toLatin1().constData());
#endif
				if (tag == "type")
					rtype = xml.s2().toInt();
				else if (tag == "devtype")
				{
					dtype = xml.s2().toInt();
					rtype = Route::MIDI_DEVICE_ROUTE;
				}
				else if (tag == "name")
					s = xml.s2();
				else if(tag == "trackId")
				{
					trackId = xml.s2().toLongLong();
					rtype = Route::TRACK_ROUTE;
				}
				else if (tag == "mport") // p3.3.49
				{
					port = xml.s2().toInt();
					rtype = Route::MIDI_PORT_ROUTE;
				}
				else if(tag == "mportId")
				{
					midiPortId  = xml.s2().toLongLong();
					rtype = Route::MIDI_PORT_ROUTE;
				}
				else
					printf("Route::read(): unknown attribute:%s\n", tag.toLatin1().constData());
				break;
			case Xml::TagEnd:
#ifdef ROUTE_DEBUG
				printf("Route::read(): tag end type:%d channel:%d name:%s\n", rtype, channel, s.toLatin1().constData());
#endif
				if (rtype == MIDI_PORT_ROUTE)
				{
					if(midiPortId > 0)
					{
						//qDebug("Route::read(): MIDI_PORT_ROUTE Finding midiport from id");
						type = rtype;
						MidiPort *mp = oomMidiPorts.value(midiPortId);
						if(mp)
						{
							midiPort = mp->portno();
							//qDebug("Route::read(): Found midiport from id: %d", midiPort);
						}
					}
					else if (port >= 0 && port < MIDI_PORTS)
					{
						type = rtype;
						midiPort = port;
						MidiPort *mp = &midiPorts[midiPort];
						midiPortId = mp->id();
					}
					else
						printf("Route::read(): midi port <%d> out of range\n", port);
				}
				else if (!s.isEmpty())
				{
					if (rtype == TRACK_ROUTE)
					{
						if(trackId > 0)
						{
							track = song->findTrackById(trackId);
							type = rtype;
						}
						else
						{
							TrackList* tl = song->tracks();
							iTrack i = tl->begin();
							for (; i != tl->end(); ++i)
							{
								Track* t = *i;
								if (t->name() == s)
								{
									track = t;
									type = rtype;
									trackId = t->id();
									break;
								}
							}
							if (i == tl->end())
								printf("Route::read(): track <%s> not found\n", s.toLatin1().constData());
						}
					}
					else if (rtype == JACK_ROUTE)
					{
						void* jport = 0;
						if (audioDevice) //Fix from Robert at muse
							jport = audioDevice->findPort(s.toLatin1().constData());

						if (jport == 0)
							printf("Route::read(): jack port <%s> not found audioDevice(%p)\n", s.toLatin1().constData(), audioDevice);
						else
						{
							jackPort = jport;
							type = rtype;
						}
					}
					else if (rtype == MIDI_DEVICE_ROUTE)
					{
						iMidiDevice imd = midiDevices.begin();
						for (; imd != midiDevices.end(); ++imd)
						{
							MidiDevice* md = *imd;
							if (md->name() == s && md->deviceType() == dtype)
							{
								// We found a device, but if it is not in use by the song (port is -1), ignore it.
								// This prevents loading and propagation of bogus routes in the oom file.
								if (md->midiPort() == -1)
									break;

								device = md;
								type = rtype;
								break;
							}
						}
						if (imd == midiDevices.end())
							printf("Route::read(): midi device <%s> not found\n", s.toLatin1().constData());
					}
				}
				return;
			default:
				break;
		}
	}
}/*}}}*/


//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Song::readRoute(Xml& xml)/*{{{*/
{
	QString src;
	QString dst;
	int ch = -1;
	int chs = -1;
	int remch = -1;

	Route sroute, droute;

	for (;;)
	{
		const QString& tag = xml.s1();
		Xml::Token token = xml.parse();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (tag == "source")
				{
					sroute.read(xml);
					sroute.channel = ch;
					sroute.channels = chs;
					sroute.remoteChannel = remch;
				}
				else if (tag == "dest")
				{
					droute.read(xml);
					droute.channel = ch;
					droute.channels = chs;
					droute.remoteChannel = remch;
				}
				else
					xml.unknown("readRoute");
				break;
			case Xml::Attribut:
#ifdef ROUTE_DEBUG
				printf("Song::readRoute(): attribute:%s\n", tag.toLatin1().constData());
#endif
				if (tag == "channel")
					ch = xml.s2().toInt();
				else
					if (tag == "channels")
					chs = xml.s2().toInt();
				else
					if (tag == "remch")
					remch = xml.s2().toInt();
				else
					if (tag == "channelMask") // p3.3.50 New channel mask for midi port-track routes.
					ch = xml.s2().toInt();
				else
					printf("Song::readRoute(): unknown attribute:%s\n", tag.toLatin1().constData());
				break;
			case Xml::TagEnd:
				if (xml.s1() == "Route")
				{
					if (sroute.isValid() && droute.isValid())// Support new routes.
					{
						// p3.3.49 Support pre- 1.1-RC2 midi-device-to-track routes. Obsolete. Replaced with midi port routes.
						if (sroute.type == Route::MIDI_DEVICE_ROUTE && droute.type == Route::TRACK_ROUTE)
						{
							if (sroute.device->midiPort() >= 0 && sroute.device->midiPort() < MIDI_PORTS
									&& ch >= 0 && ch < MIDI_CHANNELS)
							{
								sroute.midiPort = sroute.device->midiPort();
								sroute.device = 0;
								sroute.type = Route::MIDI_PORT_ROUTE;

								sroute.channel = 1 << ch; // p3.3.50  Convert to new bit-wise channel mask.
								droute.channel = sroute.channel;

								addRoute(sroute, droute);
							}
							else
								printf("  Warning - device:%s to track route, no device midi port or chan:%d out of range. Ignoring route!\n",
									sroute.device->name().toLatin1().constData(), ch);
						}
						else if (sroute.type == Route::TRACK_ROUTE && droute.type == Route::MIDI_DEVICE_ROUTE)
						{
							if (droute.device->midiPort() >= 0 && droute.device->midiPort() < MIDI_PORTS
									&& ch >= 0 && ch < MIDI_CHANNELS) // p3.3.50
							{
								droute.midiPort = droute.device->midiPort();
								droute.device = 0;
								droute.type = Route::MIDI_PORT_ROUTE;

								droute.channel = 1 << ch; // p3.3.50  Convert to new bit-wise channel mask.
								sroute.channel = droute.channel;

								addRoute(sroute, droute);
							}
							else
								printf("  Warning - track to device:%s route, no device midi port or chan:%d out of range. Ignoring route!\n",
									droute.device->name().toLatin1().constData(), ch);
						}
						else
						{
							//printf("adding new route...\n");
							addRoute(sroute, droute);
						}
					}
					else
						printf("  Warning - route invalid. Ignoring route! srcValid: %d, destValid: %d\n", 
								sroute.isValid(), droute.isValid());

					return;
				}
			default:
				break;
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   removeRoute
//---------------------------------------------------------

void RouteList::removeRoute(const Route& r)
{
	int rem = removeAll(r);
	if(!rem)
		printf("RouteList::removeRoute: Removed  %d routes\n", rem);
	//printf("RouteList::removeRoute: internal error: cannot remove Route\n");
}

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Route::dump() const/*{{{*/
{
	if (type == TRACK_ROUTE)
	{
		if (track)
			printf("Route dump: track <%s> channel %d channels %d\n", track->name().toLatin1().constData(), channel, channels);
	}
	else
		if (type == JACK_ROUTE)
	{
		if (checkAudioDevice())
			printf("Route dump: jack audio port <%s> channel %d\n", audioDevice->portName(jackPort).toLatin1().constData(), channel);
	}
	else
		if (type == MIDI_PORT_ROUTE) // p3.3.49
	{
		printf("Route dump: midi port <%d> channel mask %d\n", midiPort, channel);
	}
	else
		if (type == MIDI_DEVICE_ROUTE)
	{
		printf("Route dump: ");
		if (device)
		{
			if (device->deviceType() == MidiDevice::JACK_MIDI)
			{
				if (checkAudioDevice())
				{
					printf("jack midi device <%s> ", device->name().toLatin1().constData());
					if (device->inClientPort())
						printf("input port <%s> ",
							audioDevice->portName(device->inClientPort()).toLatin1().constData());
					if (device->outClientPort())
						printf("output port <%s> ",
							audioDevice->portName(device->outClientPort()).toLatin1().constData());
				}
			}
			else
				if (device->deviceType() == MidiDevice::ALSA_MIDI)
				printf("alsa midi device <%s> ", device->name().toLatin1().constData());
			else
				if (device->isSynthPlugin())
				printf("synth midi device <%s> ", device->name().toLatin1().constData());
			else
				printf("is midi but unknown device type:%d, ", device->deviceType());
		}
		else
			printf("is midi but invalid device, ");

		printf("channel:%d\n", channel);
	}
	else
		printf("Route dump: unknown route type:%d\n", type);
}/*}}}*/

//---------------------------------------------------------
//   operator==
//---------------------------------------------------------

bool Route::operator==(const Route& a) const
{
	// Track routes channel dont matter.
	if ((type == a.type)/* && (channel == a.channel)*/)
	{
		if (type == TRACK_ROUTE)
		{
			//qDebug("Route::operator== ~~~~~~~TRACK_ROUTE srcId:%lld name:%s, dstId:%lld name:%s", a.trackId, a.track->name().toUtf8().constData(), trackId, track->name().toUtf8().constData());
			return trackId == a.trackId && /*channels == a.channels &&*/ remoteChannel == a.remoteChannel;
		}
		else if (channel == a.channel)
		{
			if (type == JACK_ROUTE)
			{
				return jackPort == a.jackPort;
			}
			else if (type == MIDI_PORT_ROUTE)
			{
				return midiPortId == a.midiPortId;
			}
			else if (type == MIDI_DEVICE_ROUTE)
			{
				return device == a.device;
			}
		}
	}
	return false;
}

