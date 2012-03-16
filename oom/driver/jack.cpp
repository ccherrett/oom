//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: jack.cpp,v 1.30.2.17 2009/12/20 05:00:35 terminator356 Exp $
//  (C) Copyright 2002 Werner Schweer (ws@seh.de)
//=========================================================

#include "config.h"
#include <string>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
//#include <time.h> 
#include <unistd.h>
#include <jack/midiport.h>
#include <string.h>

#include "audio.h"
#include "globals.h"
#include "song.h"
#include "jackaudio.h"
#include "track.h"
#include "pos.h"
#include "tempo.h"
#include "sync.h"
#include "utils.h"

#include "midi.h"
#include "mididev.h"
#include "mpevent.h"

#include "jackmidi.h"


#define JACK_DEBUG 0

//#include "errorhandler.h"

#ifndef RTCAP
extern void doSetuid();
extern void undoSetuid();
#endif

#ifdef VST_SUPPORT
#include <fst.h>
#endif

JackAudioDevice* jackAudio;

//---------------------------------------------------------
//   checkJackClient - make sure client is valid
//---------------------------------------------------------

inline bool checkJackClient(jack_client_t* _client)
{
	if (_client == NULL)
	{
		printf("Panic! no _client!\n");
		return false;
	}
	return true;
}
//---------------------------------------------------------
//   checkAudioDevice - make sure audioDevice exists
//---------------------------------------------------------

bool checkAudioDevice()
{
	if (audioDevice == NULL)
	{
		if(debugMsg)
			printf("OOMidi:checkAudioDevice: no audioDevice\n");
		return false;
	}
	return true;
}


//---------------------------------------------------------
//   jack_thread_init
//---------------------------------------------------------

static void jack_thread_init(void*) // data
{
	doSetuid();
#ifdef VST_SUPPORT
	if (loadVST)
		fst_adopt_thread();
#endif
	undoSetuid();
}

int JackAudioDevice::processAudio(jack_nframes_t frames, void*)
{
	jackAudio->_frameCounter += frames;

	//      if (JACK_DEBUG)
	//            printf("processAudio - >>>>\n");
	segmentSize = frames;
	if (audio->isRunning())
		audio->process((unsigned long) frames);
	else
	{
		if (debugMsg)
			puts("jack calling when audio is disconnected!\n");
	}
	//      if (JACK_DEBUG)
	//            printf("processAudio - <<<<\n");
	return 0;
}

//---------------------------------------------------------
//   processSync
//    return TRUE (non-zero) when ready to roll.
//---------------------------------------------------------

static int processSync(jack_transport_state_t state, jack_position_t* pos, void*)
{
	if (JACK_DEBUG)
		printf("processSync()\n");

	if (!useJackTransport.value())
		return 1;

	int audioState = Audio::STOP;
	switch (state)
	{
		case JackTransportStopped:
			audioState = Audio::STOP;
			break;
		case JackTransportLooping:
		case JackTransportRolling:
			audioState = Audio::PLAY;
			break;
		case JackTransportStarting:
			//printf("processSync JackTransportStarting\n");

			audioState = Audio::START_PLAY;
			break;
#ifdef JACK2_SUPPORT
		case JackTransportNetStarting:
			//printf("processSync JackTransportNetStarting\n");
			audioState = Audio::START_PLAY;
			break;
#endif
	}

	unsigned frame = pos->frame;
	//printf("processSync valid:%d frame:%d\n", pos->valid, frame);

	// p3.3.23
	//printf("Jack processSync() before audio->sync frame:%d\n", frame);
	int rv = audio->sync(audioState, frame);
	//printf("Jack processSync() after audio->sync frame:%d\n", frame);
	return rv;
}

//---------------------------------------------------------
//   timebase_callback
//---------------------------------------------------------

static void timebase_callback(jack_transport_state_t /* state */,
		jack_nframes_t /* nframes */,
		jack_position_t* pos,
		int /* new_pos */,
		void*)
{
    //printf("Jack timebase_callback pos->frame:%u audio->tickPos:%d song->cpos:%d\n", pos->frame, audio->tickPos(), song->cpos());

	Pos p(extSyncFlag.value() ? audio->tickPos() : pos->frame, extSyncFlag.value() ? true : false);

	pos->valid = JackPositionBBT;
	p.mbt(&pos->bar, &pos->beat, &pos->tick);
	pos->bar++;
	pos->beat++;
	pos->bar_start_tick = Pos(pos->bar, 0, 0).tick();

	int z, n;
	AL::sigmap.timesig(p.tick(), z, n);
	pos->beats_per_bar = z;
	pos->beat_type = n;
	pos->ticks_per_beat = 24;

    int tempo = tempomap.tempo(p.tick());
    pos->beats_per_minute = (60000000.0 / tempo) * tempomap.globalTempo() / 100.0;
}

//---------------------------------------------------------
//   processShutdown
//---------------------------------------------------------

static void processShutdown(void*)
{
	if (JACK_DEBUG)
		printf("processShutdown()\n");
	jackAudio->nullify_client();
	audio->shutdown();

	int c = 0;
	while (midiSeqRunning == true)
	{
		if (c++ > 10)
		{
			fprintf(stderr, "sequencer still running, something is very wrong.\n");
			break;
		}
		sleep(1);
	}
	delete jackAudio;
	jackAudio = 0;
	audioDevice = 0;
}

//---------------------------------------------------------
//   jackError
//---------------------------------------------------------

static void jackError(const char *s)
{
	fprintf(stderr, "JACK ERROR: %s\n", s);
}

//---------------------------------------------------------
//   noJackError
//---------------------------------------------------------

static void noJackError(const char* /* s */)
{
}

//---------------------------------------------------------
//   JackAudioDevice
//---------------------------------------------------------

JackAudioDevice::JackAudioDevice(jack_client_t* cl, char* name)
: AudioDevice()
{
	_frameCounter = 0;
	strcpy(jackRegisteredName, name);
	_client = cl;
	dummyState = Audio::STOP;
	dummyPos = 0;
}

//---------------------------------------------------------
//   ~JackAudioDevice
//---------------------------------------------------------

JackAudioDevice::~JackAudioDevice()
{
	if (JACK_DEBUG)
		printf("~JackAudioDevice()\n");
	if (_client)
	{
		if (jack_client_close(_client))
		{
			fprintf(stderr, "jack_client_close() failed: %s\n", strerror(errno));
		}
	}
	if (JACK_DEBUG)
		printf("~JackAudioDevice() after jack_client_close()\n");
}

//---------------------------------------------------------
//   realtimePriority
//      return zero if not running realtime
//      can only be called if JACK client thread is already
//      running
//---------------------------------------------------------

int JackAudioDevice::realtimePriority() const
{
	pthread_t t = jack_client_thread_id(_client);
	int policy;
	struct sched_param param;
	memset(&param, 0, sizeof (param));
	int rv = pthread_getschedparam(t, &policy, &param);
	if (rv)
	{
		perror("OOMidi: JackAudioDevice::realtimePriority: Error: Get jack schedule parameter");
		return 0;
	}
	if (policy != SCHED_FIFO)
	{
		printf("OOMidi: JackAudioDevice::realtimePriority: JACK is not running realtime\n");
		return 0;
	}
	return param.sched_priority;
}

//---------------------------------------------------------
//   initJackAudio
//    return true if JACK not found
//---------------------------------------------------------

bool initJackAudio()
{
	if (JACK_DEBUG)
		printf("initJackAudio()\n");
	if (debugMsg)
	{
		fprintf(stderr, "initJackAudio()\n");
		jack_set_error_function(jackError);
	}
	else
		jack_set_error_function(noJackError);
	doSetuid();

	jack_status_t status;
	jack_client_t* client = jack_client_open("OOMidi", JackNoStartServer, &status);
	if (!client)
	{
		if (status & JackServerStarted)
			printf("jack server started...\n");
		if (status & JackServerFailed)
			printf("cannot connect to jack server\n");
		if (status & JackServerError)
			printf("communication with jack server failed\n");
		if (status & JackShmFailure)
			printf("jack cannot access shared memory\n");
		if (status & JackVersionError)
			printf("jack server has wrong version\n");
		printf("cannot create jack client\n");
		undoSetuid();
		return true;
	}

	if (debugMsg)
		fprintf(stderr, "initJackAudio(): client %s opened.\n", jack_get_client_name(client));
	if (client)
	{
		jack_set_error_function(jackError);
		jackAudio = new JackAudioDevice(client, jack_get_client_name(client));
		if (debugMsg)
			fprintf(stderr, "initJackAudio(): registering client...\n");
		jackAudio->registerClient();
		sampleRate = jack_get_sample_rate(client);
		segmentSize = jack_get_buffer_size(client);
		jack_set_thread_init_callback(client, (JackThreadInitCallback) jack_thread_init, 0);
		//jack_set_timebase_callback(client, 0, (JackTimebaseCallback) timebase_callback, 0);
	}
	undoSetuid();

	if (client)
	{
		audioDevice = jackAudio;
		jackAudio->scanMidiPorts();
		return false;
	}
	return true;
}

static int bufsize_callback(jack_nframes_t bufsize, void*)
{
    printf("JACK: buffersize changed %d\n", bufsize);
    audio->msgSetSegSize(bufsize, sampleRate);
	return 0;
}

//---------------------------------------------------------
//   freewheel_callback
//---------------------------------------------------------

static void freewheel_callback(int starting, void*)
{
	if (debugMsg || JACK_DEBUG)
		printf("JACK: freewheel_callback: starting%d\n", starting);
	audio->setFreewheel(starting);
}

static int srate_callback(jack_nframes_t n, void*)
{
	if (debugMsg || JACK_DEBUG)
		printf("JACK: sample rate changed: %d\n", n);
	return 0;
}

//---------------------------------------------------------
//   registration_callback
//---------------------------------------------------------

static void registration_callback(jack_port_id_t, int, void*)
{
	if (debugMsg || JACK_DEBUG)
		printf("JACK: registration changed\n");

	audio->sendMsgToGui('R');
}

//---------------------------------------------------------
//   JackAudioDevice::registrationChanged
//    this is called from song in gui context triggered
//    by registration_callback()
//---------------------------------------------------------

void JackAudioDevice::registrationChanged()
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::registrationChanged()\n");

	// Rescan.
	scanMidiPorts();
}

//---------------------------------------------------------
//   JackAudioDevice::connectJackMidiPorts
//---------------------------------------------------------

void JackAudioDevice::connectJackMidiPorts()
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::connectJackMidiPorts()\n");

	for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
	{
		MidiDevice* md = *i;
		if (md->deviceType() != MidiDevice::JACK_MIDI)
			continue;

		if (md->rwFlags() & 1)
		{
			void* port = md->outClientPort(); 
			if (port) 
			{
				RouteList* rl = md->outRoutes();
				for (iRoute r = rl->begin(); r != rl->end(); ++r)
					connect(port, r->jackPort);
			}
		}

		if (md->rwFlags() & 2)
		{
			void* port = md->inClientPort(); 
			if (port) 
			{
				RouteList* rl = md->inRoutes();
				for (iRoute r = rl->begin(); r != rl->end(); ++r)
					connect(r->jackPort, port);
			}
		}
	}
}
//---------------------------------------------------------
//   client_registration_callback
//---------------------------------------------------------

static void client_registration_callback(const char *name, int isRegister, void*)
{
	if (debugMsg || JACK_DEBUG)
		printf("JACK: client registration changed:%s register:%d\n", name, isRegister);
}

//---------------------------------------------------------
//   port_connect_callback
//---------------------------------------------------------

static void port_connect_callback(jack_port_id_t a, jack_port_id_t b, int isConnect, void*)
{
	if (debugMsg || JACK_DEBUG)
	{
		printf("JACK: port connections changed: A:%d B:%d isConnect:%d\n", a, b, isConnect);
	}
}

//---------------------------------------------------------
//   graph_callback
//    this is called from jack when the connections
//    changed
//---------------------------------------------------------

static int graph_callback(void*)
{
	if (JACK_DEBUG)
		printf("graph_callback()\n");
	// we cannot call JackAudioDevice::graphChanged() from this
	// context, so we send a message to the gui thread which in turn
	// calls graphChanged()
	audio->sendMsgToGui('C');
	if (debugMsg)
		printf("JACK: graph changed\n");
	return 0;
}

//---------------------------------------------------------
//   JackAudioDevice::graphChanged
//    this is called from song in gui context triggered
//    by graph_callback()
//---------------------------------------------------------

void JackAudioDevice::graphChanged()
{
	if (JACK_DEBUG)
		printf("graphChanged()\n");
	if (!checkJackClient(_client)) return;
	InputList* il = song->inputs();
	for (iAudioInput ii = il->begin(); ii != il->end(); ++ii)
	{
		AudioInput* it = *ii;
		int channels = it->channels();
		for (int channel = 0; channel < channels; ++channel)
		{
			jack_port_t* port = (jack_port_t*) (it->jackPort(channel));
			if (port == 0)
				continue;
			const char** ports = jack_port_get_all_connections(_client, port);
			RouteList* rl = it->inRoutes();

			//---------------------------------------
			// check for disconnects
			//---------------------------------------

			bool erased = true;
			// limit set to 20 iterations for disconnects, don't know how to make it go
			// the "right" amount
			while(erased)//for (int i = 0; i < 20; i++)
			{
				erased = false;
				//for (iRoute irl = rl->begin(); irl != rl->end(); ++irl)
				for(int r = 0; r < rl->size(); r++)
				{
					Route src = rl->at(r);
					if (src.channel != channel)
						continue;
					QString name = src.name();
					QByteArray ba = name.toLatin1();
					const char* portName = ba.constData();
					//printf("portname=%s\n", portName);
					bool found = false;
					const char** pn = ports;
					while (pn && *pn)
					{
						if (strcmp(*pn, portName) == 0)
						{
							found = true;
							break;
						}
						++pn;
					}
					//FIXME: This is the code that removes the route from the input track if jack dies
					if (!found)
					{
						//Just remove the route from the list now
						//src = rl->takeAt(r);
						if(debugMsg)
							qDebug("JackAudioDevice::graphChanged: remove port: %s, from %s", portName, it->name().toUtf8().constData());
						audio->msgRemoveRoute1(
								src,
								//Route(portName, false, channel, Route::JACK_ROUTE),
								Route(it, channel)
								);
						erased = true;
						break;
					}
				}
				//if (!erased)
				//	break;
			}

			//---------------------------------------
			// check for connects
			//---------------------------------------

			if (ports)
			{
				const char** pn = ports;
				while (*pn)
				{
					bool found = false;
					for (iRoute irl = rl->begin(); irl != rl->end(); ++irl)
					{
						if (irl->channel != channel)
							continue;
						QString name = irl->name();
						QByteArray ba = name.toLatin1();
						const char* portName = ba.constData();
						if (strcmp(*pn, portName) == 0)
						{
							found = true;
							break;
						}
					}
					if (!found)
					{
						audio->msgAddRoute1(
								Route(*pn, false, channel, Route::JACK_ROUTE),
								Route(it, channel)
								);
					}
					++pn;
				}

				// p3.3.37
				//delete ports;
				free(ports);

				ports = NULL;
			}
		}
	}
	OutputList* ol = song->outputs();
	for (iAudioOutput ii = ol->begin(); ii != ol->end(); ++ii)
	{
		AudioOutput* it = *ii;
		int channels = it->channels();
		for (int channel = 0; channel < channels; ++channel)
		{
			jack_port_t* port = (jack_port_t*) (it->jackPort(channel));
			if (port == 0)
				continue;
			const char** ports = jack_port_get_all_connections(_client, port);
			RouteList* rl = it->outRoutes();

			//---------------------------------------
			// check for disconnects
			//---------------------------------------

			bool erased = true;
			// limit set to 20 iterations for disconnects, don't know how to make it go
			// the "right" amount
			while (erased)//int i = 0; i < 20; i++)
			{
				erased = false;
				//for (iRoute irl = rl->begin(); irl != rl->end(); ++irl)
				for(int r = 0; r < rl->size(); r++)
				{
					Route dst = rl->at(r);
					if (dst.channel != channel)
						continue;
					QString name = dst.name();
					QByteArray ba = name.toLatin1();
					const char* portName = ba.constData();
					bool found = false;
					const char** pn = ports;
					while (pn && *pn)
					{
						if (strcmp(*pn, portName) == 0)
						{
							found = true;
							break;
						}
						++pn;
					}
					if (!found)
					{
						//Just remove the route from the list now
						//src = rl->takeAt(r);
						audio->msgRemoveRoute1(
								Route(it, channel),
								dst
								//Route(portName, false, channel, Route::JACK_ROUTE)
								);
						erased = true;
						break;
					}
				}
				//if (!erased)
				//	break;
			}

			//---------------------------------------
			// check for connects
			//---------------------------------------

			if (ports)
			{
				const char** pn = ports;
				while (*pn)
				{
					bool found = false;
					for (iRoute irl = rl->begin(); irl != rl->end(); ++irl)
					{
						if (irl->channel != channel)
							continue;
						QString name = irl->name();
						QByteArray ba = name.toLatin1();
						const char* portName = ba.constData();
						if (strcmp(*pn, portName) == 0)
						{
							found = true;
							break;
						}
					}
					if (!found)
					{
						audio->msgAddRoute1(Route(it, channel), Route(*pn, false, channel, Route::JACK_ROUTE));
					}
					++pn;
				}

				// p3.3.37
				//delete ports;
				free(ports);

				ports = NULL;
			}
		}
	}

	for (iMidiDevice ii = midiDevices.begin(); ii != midiDevices.end(); ++ii)
	{
		MidiDevice* md = *ii;
		if (md->deviceType() != MidiDevice::JACK_MIDI)
			continue;

		//---------------------------------------
		// outputs
		//---------------------------------------

		if (md->rwFlags() & 1) // Writable
		{
			// p3.3.55
			jack_port_t* port = (jack_port_t*) md->outClientPort();
			if (port != 0)
			{
				//printf("graphChanged() valid out client port\n"); // p3.3.55

				const char** ports = jack_port_get_all_connections(_client, port);

				RouteList* rl = md->outRoutes();

				//---------------------------------------
				// check for disconnects
				//---------------------------------------

				bool erased = true;
				// limit set to 20 iterations for disconnects, don't know how to make it go
				// the "right" amount
				//for (int i = 0; i < 20; i++)
				while(erased)
				{
					erased = false;
					//for (iRoute irl = rl->begin(); irl != rl->end(); ++irl)
					for(int r = 0; r < rl->size(); r++)
					{
						Route dst = rl->at(r);
						QString name = dst.name();
						QByteArray ba = name.toLatin1();
						const char* portName = ba.constData();
						bool found = false;
						const char** pn = ports;
						while (pn && *pn)
						{
							if (strcmp(*pn, portName) == 0)
							{
								found = true;
								break;
							}
							++pn;
						}
						if (!found)
						{
							//Just remove the route
							//src = rl->takeAt(r);
							audio->msgRemoveRoute1(	
								Route(md, -1),
								dst
								//Route(portName, false, -1, Route::JACK_ROUTE)
							);
							erased = true;
							break;
						}
					}
					//if (!erased)
					//	break;
				}

				//---------------------------------------
				// check for connects
				//---------------------------------------

				if (ports)
				{
					const char** pn = ports;
					while (*pn)
					{
						bool found = false;
						for (iRoute irl = rl->begin(); irl != rl->end(); ++irl)
						{
							QString name = irl->name();
							QByteArray ba = name.toLatin1();
							const char* portName = ba.constData();
							if (strcmp(*pn, portName) == 0)
							{
								found = true;
								break;
							}
						}
						if (!found)
						{
							audio->msgAddRoute1(Route(md, -1), Route(*pn, false, -1, Route::JACK_ROUTE));
						}
						++pn;
					}

					// Done with ports. Free them.
					free(ports);
				}
			}
		}


		//------------------------
		// Inputs
		//------------------------

		if (md->rwFlags() & 2) // Readable
		{
			// p3.3.55
			jack_port_t* port = (jack_port_t*) md->inClientPort();
			if (port != 0)
			{
				//printf("graphChanged() valid in client port\n"); // p3.3.55
				const char** ports = jack_port_get_all_connections(_client, port);

				RouteList* rl = md->inRoutes();

				//---------------------------------------
				// check for disconnects
				//---------------------------------------

				bool erased = true;
				// limit set to 20 iterations for disconnects, don't know how to make it go
				// the "right" amount
				//for (int i = 0; i < 20; i++)
				while(erased)
				{
					erased = false;
					//for (iRoute irl = rl->begin(); irl != rl->end(); ++irl)
					for(int r = 0; r < rl->size(); r++)
					{
						Route src = rl->at(r);
						QString name = src.name();
						QByteArray ba = name.toLatin1();
						const char* portName = ba.constData();
						bool found = false;
						const char** pn = ports;
						while (pn && *pn)
						{
							if (strcmp(*pn, portName) == 0)
							{
								found = true;
								break;
							}
							++pn;
						}
						if (!found)
						{
							//src = rl->takeAt(r);
							audio->msgRemoveRoute1(
								//Route(portName, false, -1, Route::JACK_ROUTE),
								src,
								Route(md, -1));
							erased = true;
							break;
						}
					}
					//if (!erased)
					//	break;
				}

				//---------------------------------------
				// check for connects
				//---------------------------------------

				if (ports)
				{
					const char** pn = ports;
					while (*pn)
					{
						bool found = false;
						for (iRoute irl = rl->begin(); irl != rl->end(); ++irl)
						{
							QString name = irl->name();
							QByteArray ba = name.toLatin1();
							const char* portName = ba.constData();
							if (strcmp(*pn, portName) == 0)
							{
								found = true;
								break;
							}
						}
						if (!found)
						{
							audio->msgAddRoute1(Route(*pn, false, -1, Route::JACK_ROUTE), Route(md, -1));
						}
						++pn;
					}
					// Done with ports. Free them.
					free(ports);
				}
			}
		}
	}
}

//static int xrun_callback(void*)
//      {
//      printf("JACK: xrun\n");
//      return 0;
//      }

//---------------------------------------------------------
//   register
//---------------------------------------------------------

void JackAudioDevice::registerClient()
{
	if (JACK_DEBUG)
		printf("registerClient()\n");
	if (!checkJackClient(_client)) return;
	jack_set_process_callback(_client, processAudio, 0);
	jack_set_sync_callback(_client, processSync, 0);

	jack_on_shutdown(_client, processShutdown, 0);
	jack_set_buffer_size_callback(_client, bufsize_callback, 0);
	jack_set_sample_rate_callback(_client, srate_callback, 0);
	jack_set_port_registration_callback(_client, registration_callback, 0);
	jack_set_client_registration_callback(_client, client_registration_callback, 0);
	jack_set_port_connect_callback(_client, port_connect_callback, 0);

	jack_set_graph_order_callback(_client, graph_callback, 0);
	//      jack_set_xrun_callback(client, xrun_callback, 0);
	jack_set_freewheel_callback(_client, freewheel_callback, 0);
}

//---------------------------------------------------------
//   registerInPort
//---------------------------------------------------------

void* JackAudioDevice::registerInPort(const char* name, bool midi)
{
	if (JACK_DEBUG)
		printf("registerInPort()\n");
	if (!checkJackClient(_client)) return NULL;
	const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
	void* p = jack_port_register(_client, name, type, JackPortIsInput, 0);
	return p;
}

//---------------------------------------------------------
//   registerOutPort
//---------------------------------------------------------

void* JackAudioDevice::registerOutPort(const char* name, bool midi)
{
	if (JACK_DEBUG)
		printf("registerOutPort()\n");
	if (!checkJackClient(_client)) return NULL;
	const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
	void* p = jack_port_register(_client, name, type, JackPortIsOutput, 0);
	return p;
}

//---------------------------------------------------------
//   exitJackAudio
//---------------------------------------------------------

void exitJackAudio()
{
	if (JACK_DEBUG)
		printf("exitJackAudio()\n");
	if (jackAudio)
		delete jackAudio;

	if (JACK_DEBUG)
		printf("exitJackAudio() after delete jackAudio\n");

	audioDevice = NULL;

}

//---------------------------------------------------------
//   connect
//---------------------------------------------------------

void JackAudioDevice::connect(void* src, void* dst)
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::connect()\n");
	if (!checkJackClient(_client)) return;
	const char* sn = jack_port_name((jack_port_t*) src);
	const char* dn = jack_port_name((jack_port_t*) dst);
	if (sn == 0 || dn == 0)
	{
		fprintf(stderr, "JackAudio::connect: unknown jack ports\n");
		return;
	}
	int err = jack_connect(_client, sn, dn);
	if (err)
	{
		fprintf(stderr, "jack connect <%s>%p - <%s>%p failed with err:%d\n", sn, src, dn, dst, err);
	}
	else if (JACK_DEBUG)
	{
		fprintf(stderr, "jack connect <%s>%p - <%s>%p succeeded\n", sn, src, dn, dst);
	}
}

//---------------------------------------------------------
//   disconnect
//---------------------------------------------------------

void JackAudioDevice::disconnect(void* src, void* dst)
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::disconnect()\n");
	if (!checkJackClient(_client)) return;
	if (!src || !dst) // p3.3.55
		return;
	const char* sn = jack_port_name((jack_port_t*) src);
	const char* dn = jack_port_name((jack_port_t*) dst);
	if (sn == 0 || dn == 0)
	{
		fprintf(stderr, "JackAudio::disconnect: unknown jack ports\n");
		return;
	}
	int err = jack_disconnect(_client, sn, dn);
	if (err)
	{
		fprintf(stderr, "jack disconnect <%s> - <%s> failed with err:%d\n", sn, dn, err);
	}
	else if (JACK_DEBUG)
	{
		fprintf(stderr, "jack disconnect <%s> - <%s> succeeded\n", sn, dn);
	}
}

//---------------------------------------------------------
//   start
//---------------------------------------------------------

void JackAudioDevice::start(int /*priority*/)
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::start()\n");
	if (!checkJackClient(_client)) return;

	doSetuid();

	if (jack_activate(_client))
	{
		undoSetuid();
		fprintf(stderr, "JACK: cannot activate client\n");
		exit(-1);
	}
	/* connect the ports. Note: you can't do this before
	   the client is activated, because we can't allow
	   connections to be made to clients that aren't
	   running.
	 */

	InputList* il = song->inputs();
	for (iAudioInput i = il->begin(); i != il->end(); ++i)
	{
		AudioInput* ai = *i;
		int channel = ai->channels();
		for (int ch = 0; ch < channel; ++ch)
		{
			RouteList* rl = ai->inRoutes();
			void* port = ai->jackPort(ch);
			for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
			{
				if (ir->channel == ch)
					connect(ir->jackPort, port);
			}
		}
	}
	OutputList* ol = song->outputs();
	for (iAudioOutput i = ol->begin(); i != ol->end(); ++i)
	{
		AudioOutput* ai = *i;
		int channel = ai->channels();
		for (int ch = 0; ch < channel; ++ch)
		{
			RouteList* rl = ai->outRoutes();
			void* port = ai->jackPort(ch);
			for (iRoute r = rl->begin(); r != rl->end(); ++r)
			{
				if (r->channel == ch)
				{
					connect(port, r->jackPort);
				}
			}
		}
	}

	// Connect the Jack midi client ports to device ports.
	connectJackMidiPorts();

	undoSetuid();

	fflush(stdin);
}

//---------------------------------------------------------
//   stop
//---------------------------------------------------------

void JackAudioDevice::stop()
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::stop()\n");
	if (!checkJackClient(_client)) return;
	if (jack_deactivate(_client))
	{
		fprintf(stderr, "cannot deactivate client\n");
	}
}

//---------------------------------------------------------
//   transportQuery
//---------------------------------------------------------

jack_transport_state_t JackAudioDevice::transportQuery(jack_position_t* pos)
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::transportQuery pos:%d\n", (unsigned int) pos->frame);

	// TODO: Compose and return a state if OOMidi is disengaged from Jack transport.

	return jack_transport_query(_client, pos);
}

//---------------------------------------------------------
//   getCurFrame
//---------------------------------------------------------

unsigned int JackAudioDevice::getCurFrame()
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::getCurFrame pos.frame:%d\n", pos.frame);

	if (!useJackTransport.value())
		return (unsigned int) dummyPos;

	return pos.frame;
}

//---------------------------------------------------------
//   framePos
//---------------------------------------------------------

int JackAudioDevice::framePos() const
{
	if (!checkJackClient(_client)) return 0;
	jack_nframes_t n = jack_frame_time(_client);

	return (int) n;
}

#if 0
//---------------------------------------------------------
//   framesSinceCycleStart
//---------------------------------------------------------

int JackAudioDevice::framesSinceCycleStart() const
{
	jack_nframes_t n = jack_frames_since_cycle_start(client);
	return (int) n;
}

//---------------------------------------------------------
//   framesDelay
//    TODO
//---------------------------------------------------------

int JackAudioDevice::frameDelay() const
{
	jack_nframes_t n = (segmentSize * (segmentCount - 1)) - jack_frames_since_cycle_start(client);
	return (int) n;
}
#endif

//---------------------------------------------------------
//   outputPorts
//---------------------------------------------------------

std::list<QString> JackAudioDevice::outputPorts(bool midi, int aliases)
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::outputPorts()\n");
	std::list<QString> clientList;
	if (!checkJackClient(_client)) return clientList;
	QString qname;
	const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
	const char** ports = jack_get_ports(_client, 0, type, JackPortIsOutput);
	for (const char** p = ports; p && *p; ++p)
	{
		jack_port_t* port = jack_port_by_name(_client, *p);

		int nsz = jack_port_name_size();
		char buffer[nsz];

		strncpy(buffer, *p, nsz);

		// Ignore our own client ports.
		if (jack_port_is_mine(_client, port))
		{
			if (debugMsg)
				printf("JackAudioDevice::outputPorts ignoring own port: %s\n", *p);
			continue;
		}

		if ((aliases == 0) || (aliases == 1))
		{
			char a2[nsz];
			char* al[2];
			al[0] = buffer;
			al[1] = a2;
			int na = jack_port_get_aliases(port, al);
			int a = aliases;
			if (a >= na)
			{
				a = na;
				if (a > 0)
					a--;
			}
			qname = QString(al[a]);
		}
		else
			qname = QString(buffer);

		clientList.push_back(qname);
	}

	if (ports)
		free(ports);

	return clientList;
}

//---------------------------------------------------------
//   inputPorts
//---------------------------------------------------------

std::list<QString> JackAudioDevice::inputPorts(bool midi, int aliases)
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::inputPorts()\n");
	std::list<QString> clientList;
	if (!checkJackClient(_client)) return clientList;
	QString qname;
	const char* type = midi ? JACK_DEFAULT_MIDI_TYPE : JACK_DEFAULT_AUDIO_TYPE;
	const char** ports = jack_get_ports(_client, 0, type, JackPortIsInput);
	for (const char** p = ports; p && *p; ++p)
	{
		jack_port_t* port = jack_port_by_name(_client, *p);

		int nsz = jack_port_name_size();
		char buffer[nsz];

		strncpy(buffer, *p, nsz);

		// Ignore our own client ports.
		if (jack_port_is_mine(_client, port))
		{
			if (debugMsg)
				printf("JackAudioDevice::inputPorts ignoring own port: %s\n", *p);
			continue;
		}

		if ((aliases == 0) || (aliases == 1))
		{
			char a2[nsz];
			char* al[2];
			al[0] = buffer;
			al[1] = a2;
			int na = jack_port_get_aliases(port, al);
			int a = aliases;
			if (a >= na)
			{
				a = na;
				if (a > 0)
					a--;
			}
			qname = QString(al[a]);
		}
		else
			qname = QString(buffer);

		clientList.push_back(qname);
	}

	if (ports)
		free(ports);

	return clientList;
}

//---------------------------------------------------------
//   portName
//---------------------------------------------------------

QString JackAudioDevice::portName(void* port)
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::portName(\n");
	if (!checkJackClient(_client)) return "";
	if (!port)
		return "";

	QString s(jack_port_name((jack_port_t*) port));
	return s;
}

//---------------------------------------------------------
//   unregisterPort
//---------------------------------------------------------

void JackAudioDevice::unregisterPort(void* p)
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::unregisterPort(\n");
	if (!checkJackClient(_client)) return;
	jack_port_t* port = (jack_port_t*)p;
	if(port)
		jack_port_unregister(_client, port);
}

//---------------------------------------------------------
//   getState
//---------------------------------------------------------

int JackAudioDevice::getState()
{
	// If we're not using Jack's transport, just return current state.
	if (!useJackTransport.value())
	{
		return dummyState;
	}

	if (!checkJackClient(_client)) return 0;
	transportState = jack_transport_query(_client, &pos);

	switch (transportState)
	{
		case JackTransportStopped:
			return Audio::STOP;
		case JackTransportLooping:
		case JackTransportRolling:
			return Audio::PLAY;
		case JackTransportStarting:
			return Audio::START_PLAY;
#ifdef JACK2_SUPPORT
		case JackTransportNetStarting:
			return Audio::START_PLAY;
			break;
#endif
		default:
			return Audio::STOP;
	}
}

//---------------------------------------------------------
//   setFreewheel
//---------------------------------------------------------

void JackAudioDevice::setFreewheel(bool f)
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::setFreewheel(\n");
	if (!checkJackClient(_client)) return;
	jack_set_freewheel(_client, f);
}

//---------------------------------------------------------
//   dummySync
//---------------------------------------------------------

bool JackAudioDevice::dummySync(int state)
{
	// Roughly segment time length.
	//timespec ts = { 0, (1000000000 * segmentSize) / sampleRate };     // In nanoseconds.
	unsigned int sl = (1000000 * segmentSize) / sampleRate; // In microseconds.

	double ct = curTime();
	// Wait for a default maximum of 5 seconds.
	// Similar to how Jack is supposed to wait a default of 2 seconds for slow clients.
	// TODO: Make this timeout a 'settings' option so it can be applied both to Jack and here.
	while ((curTime() - ct) < 5.0)
	{
		if (audio->sync(state, dummyPos))
			return true;

		// Not ready. Wait a 'segment', try again...
		//nanosleep(&ts, NULL);
		usleep(sl); // usleep is supposed to be obsolete!
	}

	//if(JACK_DEBUG)
	printf("JackAudioDevice::dummySync Sync timeout - audio not ready!\n");

	return false;
}

//---------------------------------------------------------
//   startTransport
//---------------------------------------------------------

void JackAudioDevice::startTransport()
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::startTransport()\n");

	// If we're not using Jack's transport, just pass PLAY and current frame along
	//  as if processSync was called.
	if (!useJackTransport.value())
	{
		if (dummySync(Audio::START_PLAY))
		{
			dummyState = Audio::PLAY;
			return;
		}

		dummyState = Audio::PLAY;
		return;
	}

	if (!checkJackClient(_client)) return;
	jack_transport_start(_client);
}

//---------------------------------------------------------
//   stopTransport
//---------------------------------------------------------

void JackAudioDevice::stopTransport()
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::stopTransport()\n");

	dummyState = Audio::STOP;

	if (!useJackTransport.value())
	{
		return;
	}

	if (!checkJackClient(_client)) return;
	if (transportState != JackTransportStopped)
	{
		jack_transport_stop(_client);
		transportState = JackTransportStopped;
	}
}

//---------------------------------------------------------
//   seekTransport
//---------------------------------------------------------

void JackAudioDevice::seekTransport(unsigned frame)
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::seekTransport() frame:%d\n", frame);

	dummyPos = frame;
	if (!useJackTransport.value())
	{
		// If we're not using Jack's transport, just pass the current state and new frame along
		//  as if processSync was called.
		int tempState = dummyState;

		// Is OOMidi audio ready yet?
		if (dummySync(Audio::START_PLAY))
		{
			dummyState = tempState;
			return;
		}

		// Not ready, resume previous state anyway.
		dummyState = Audio::STOP;
		return;
	}

	if (!checkJackClient(_client)) return;
	jack_transport_locate(_client, frame);
}

//---------------------------------------------------------
//   seekTransport
//---------------------------------------------------------

void JackAudioDevice::seekTransport(const Pos &p)
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::seekTransport() frame:%d\n", p.frame());

	dummyPos = p.frame();
	if (!useJackTransport.value())
	{
		// If we're not using Jack's transport, just pass the current state and new frame along
		//  as if processSync was called.
		int tempState = dummyState;

		// Is OOMidi audio ready yet?
		if (dummySync(Audio::START_PLAY))
		{
			dummyState = tempState;
			return;
		}

		dummyState = Audio::STOP;
		return;
	}

	if (!checkJackClient(_client)) return;

	jack_transport_locate(_client, p.frame());
}

//---------------------------------------------------------
//   findPort
//---------------------------------------------------------

void* JackAudioDevice::findPort(const char* name)
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::findPort(\n");
	if (!checkJackClient(_client)) return NULL;
	void* p = jack_port_by_name(_client, name);
	return p;
}

//---------------------------------------------------------
//   setMaster
//---------------------------------------------------------

int JackAudioDevice::setMaster(bool f)
{
	if (JACK_DEBUG)
		printf("JackAudioDevice::setMaster val:%d\n", f);
	if (!checkJackClient(_client))
		return 0;

	int r = 0;
	if (f)
	{
		if (useJackTransport.value())
		{
			// Make OOMidi the Jack timebase master. Do it unconditionally (second param = 0).
			r = jack_set_timebase_callback(_client, 0, (JackTimebaseCallback) timebase_callback, 0);
			if (debugMsg || JACK_DEBUG)
			{
				if (r)
					printf("JackAudioDevice::setMaster jack_set_timebase_callback failed: result:%d\n", r);
			}
		}
		else
		{
			r = 1;
			printf("JackAudioDevice::setMaster cannot set master because useJackTransport is false\n");
		}
	}
	else
	{
		r = jack_release_timebase(_client);
		if (debugMsg || JACK_DEBUG)
		{
			if (r)
				printf("JackAudioDevice::setMaster jack_release_timebase failed: result:%d\n", r);
		}
	}
	return r;
}

//---------------------------------------------------------
//   scanMidiPorts
//---------------------------------------------------------

void JackAudioDevice::scanMidiPorts()
{
	if (debugMsg)
		printf("JackAudioDevice::scanMidiPorts:\n");
}

