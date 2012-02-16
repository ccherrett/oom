//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//
//  Based sndfile-jackplay.c
//  Copyright (c) 2007-2009 Erik de Castro Lopo <erikd@mega-nerd.com>
//  Copyright (C) 2007 Jonatan Liljedahl <lijon@kymatica.com>
//=========================================================

#include "AudioPlayer.h"
#include "config.h"
#include "gconfig.h"
#include "globals.h"

#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <QStringList>
#include <QTimer>

#include <jack/ringbuffer.h>

//Define pthread lock types
pthread_mutex_t disk_thread_lock = PTHREAD_MUTEX_INITIALIZER ;
pthread_cond_t data_ready = PTHREAD_COND_INITIALIZER ;

static jack_ringbuffer_t *ringbuf ;
static jack_port_t **output_port ;
static jack_default_audio_sample_t ** outs ;
const size_t sample_size = sizeof (jack_default_audio_sample_t);

AudioPlayer::AudioPlayer()
{
	m_client = 0;
	m_isPlaying = false;
	m_seeking = false;
	memset (&info, 0, sizeof (info)) ;
	info.volume = 0.60;
	info.can_process = 0 ;
	info.can_read = 0 ;
	m_oldVolume = 0.60;
}

AudioPlayer::~AudioPlayer()
{//Clean up jack client
	stop();
}

int AudioPlayer::process(jack_nframes_t nframes, void * arg)
{
	thread_info_t *info = (thread_info_t *) arg ;
	jack_default_audio_sample_t buf [info->channels] ;
	unsigned i, n ;

	if (!info->can_process)
		return 0 ;

	for (n = 0 ; n < info->channels ; n++)
	{
		outs [n] = (jack_default_audio_sample_t*)jack_port_get_buffer (output_port [n], nframes) ;
	}

	for (i = 0 ; i < nframes ; i++)
	{	
		size_t read_cnt ;

		// Read one frame of audio. 
		read_cnt = jack_ringbuffer_read (ringbuf, (char*)buf, sample_size*info->channels) ;
		if (read_cnt == 0 && info->read_done)
		{	// File is done, so stop the main loop. 
			info->play_done = 1 ;
			return 0 ;
		}

		// Update play-position counter. 
		info->pos += read_cnt / (sample_size*info->channels) ;

		// Output each channel of the frame. 
		for (n = 0 ; n < info->channels ; n++)
			outs [n][i] = buf[n] * info->volume;
	}

	// Wake up the disk thread to read more data. 
	if (pthread_mutex_trylock (&disk_thread_lock) == 0)
	{	pthread_cond_signal (&data_ready) ;
		pthread_mutex_unlock (&disk_thread_lock) ;
	}

	return 0 ;
}

void* AudioPlayer::read_file(void* arg)
{
	thread_info_t *info = (thread_info_t *) arg ;
	sf_count_t buf_avail, read_frames ;
	jack_ringbuffer_data_t vec [2] ;
	size_t bytes_per_frame = sample_size*info->channels ;

	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL) ;
	pthread_mutex_lock (&disk_thread_lock) ;

	while (1)
	{	jack_ringbuffer_get_write_vector (ringbuf, vec) ;

		read_frames = 0 ;

		if (vec [0].len)
		{	// Fill the first part of the ringbuffer. 
			buf_avail = vec [0].len / bytes_per_frame ;
			read_frames = sf_readf_float (info->sndfile, (float *) vec [0].buf, buf_avail) ;
			if (vec [1].len)
			{	// Fill the second part of the ringbuffer? 
				buf_avail = vec [1].len / bytes_per_frame ;
				read_frames += sf_readf_float (info->sndfile, (float *) vec [1].buf, buf_avail) ;
			}
		}

		if (read_frames == 0)
			break ; // EOF or sndfile error

		jack_ringbuffer_write_advance (ringbuf, read_frames * bytes_per_frame) ;

		// Tell process that we've filled the ringbuffer.
		info->can_process = 1 ;

		// Wait for the process thread to wake us up. 
		pthread_cond_wait (&data_ready, &disk_thread_lock) ;
	}

	// Tell that we're done reading the file. 
	info->read_done = 1 ;
	pthread_mutex_unlock (&disk_thread_lock) ;

	return 0 ;
}

static void jack_shutdown (void *arg)
{	(void) arg ;
	//Do nothing for now
	//fprintf (stderr, "ClipList jack_shutdown\n");
}

QString AudioPlayer::calcTimeString(int frames)
{
	QString time;
	float sec = frames / (1.0 * m_srate) ;
	int min = sec / 60.0 ;
	time.sprintf("%02d:%05.2f", min, fmod(sec, 60.0));
	return time;
}

void AudioPlayer::printTime()
{
	sf_count_t frames = 0;
	if(info.sndfile)
	{
		frames = sf_seek (info.sndfile, 0, SEEK_CUR);
		info.seek = frames;
	}
	if(!m_seeking)
		emit timeChanged((int)frames);
	emit timeChanged(calcTimeString((int)frames));
}

void AudioPlayer::setVolume(double val)
{
	double vol;
	if (val <= config.minSlider)
	{
		vol = 0.0;
		val -= 1.0; 
	}
	else
		vol = pow(10.0, val / 20.0);
	info.volume = vol;
}

void AudioPlayer::restoreVolume()
{
	if(m_oldVolume)
	{
		info.volume = m_oldVolume;
		m_oldVolume = 0.0;
	}
}

void AudioPlayer::stopSeek()
{
	//qDebug("AudioPlayer::stopSeek: m_oldVolume: %f", m_oldVolume);
	m_seeking = false;
	if(m_oldVolume)
		QTimer::singleShot(500, this, SLOT(restoreVolume()));
}

void AudioPlayer::seek(int val)/*{{{*/
{
	//qDebug("AudioPlayer::seek: val: %d", val);
	bool was_seeking = m_seeking;
	m_seeking = true;
	info.seek = val;
	
	if(!m_oldVolume)
	{
		m_oldVolume = info.volume;
		info.volume = 0.0;
	}
	
	sf_count_t frames = 0;
	if(info.sndfile && m_isPlaying)
	{
		frames = sf_seek(info.sndfile, val, SEEK_SET);
	}

	if(!was_seeking)
		QTimer::singleShot(1000, this, SLOT(stopSeek()));
}/*}}}*/

void AudioPlayer::stopClient()
{
	if(m_client)
	{
		jack_client_close (m_client) ;
		jack_ringbuffer_free (ringbuf) ;
		free (outs) ;
		free (output_port) ;
		m_client = 0;
		emit readyForPlay();
	}
}

//Make sure the client is dead and fire the ready signal
void AudioPlayer::check()
{
	if(!m_client)
	{
		emit readyForPlay();
	}
}

bool AudioPlayer::startClient()/*{{{*/
{
	// create jack client
	if(!m_client)
	{
		int channels = info.channels;
		jack_status_t status;
		m_client = jack_client_open("OOMidi_ClipList", JackNoStartServer, &status);
		if (!m_client)
		{
			fprintf (stderr, "Jack server not running?\n") ;
			emit playbackStopped(true);
			return false;
		}
		
		m_srate = jack_get_sample_rate (m_client) ;

		// Set up callbacks.
		jack_set_process_callback (m_client, AudioPlayer::process, &info) ;
		jack_on_shutdown (m_client, jack_shutdown, 0);

		//Default to two channels and we will just fill one if mono file
		output_port = (jack_port_t**)calloc (channels, sizeof (jack_port_t *)) ;
		outs = (jack_default_audio_sample_t**)calloc(channels, sizeof (jack_default_audio_sample_t *)) ;
		for (int i = 0 ; i < channels ; i++)
		{	
			char name [16] ;

			snprintf (name, sizeof (name), "out_%d", i + 1) ;
			output_port [i] = jack_port_register (m_client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0) ;
		}

		// Allocate and clear ringbuffer.
		ringbuf = jack_ringbuffer_create (sizeof (jack_default_audio_sample_t) * RB_SIZE) ;
		memset (ringbuf->buf, 0, ringbuf->size) ;

		// Activate client. 
		if (jack_activate (m_client))
		{
			fprintf (stderr, "Cannot activate client.\n") ;
			emit playbackStopped(true);
			m_client = 0;
			return false;
		}

		// Auto connect all channels.
		for (int i = 0 ; i < channels ; i++)
		{	
			char name [64] ;

			snprintf (name, sizeof (name), "system:playback_%d", i + 1) ;

			if(jack_connect (m_client, jack_port_name (output_port [i]), name))
				fprintf (stderr, "Cannot connect output port %d (%s).\n", i, name) ;

            // connect mono files to playback_2
            if (channels == 1)
            {
                snprintf (name, sizeof (name), "system:playback_%d", i + 2) ;

                if(jack_connect (m_client, jack_port_name (output_port [i]), name))
                    fprintf (stderr, "Cannot connect output port %d (%s).\n", i, name) ;
            }
		}
		return true;
	}
	else
	{
		return true;
	}
}/*}}}*/

void AudioPlayer::play(const QString &file)/*{{{*/
{
	SNDFILE *sndfile ;
	SF_INFO sndfileinfo ;

	if (file.isEmpty())
	{	
		fprintf (stderr, "no soundfile given\n");
		//TODO: emit error signals
		emit playbackStopped(true);
		return;
	}

	// Open the soundfile.
	sndfileinfo.format = 0 ;
	sndfile = sf_open (file.toUtf8().constData(), SFM_READ, &sndfileinfo) ;
	if (sndfile == NULL)
	{	
		//TODO: emit error signals
		fprintf (stderr, "Could not open soundfile '%s'\n", file.toUtf8().constData()) ;
		emit playbackStopped(true);
		return;
	}

	// Init the thread info struct.
	info.can_process = 0 ;
	info.read_done = 0 ;
	info.play_done = 0 ;
	info.sndfile = sndfile ;
	info.channels = sndfileinfo.channels ;
	info.client = m_client ;
	info.pos = 0 ;
	
	if(info.seek && info.seek < sndfileinfo.frames)
	{
		//qDebug("Preseting seek point");
		sf_seek(sndfile, info.seek, SEEK_CUR);
		info.seek = 0;
	}

	if(!startClient())
		return;
		
	// Start the disk thread. 
	pthread_create (&info.thread_id, NULL, AudioPlayer::read_file, &info);
	info.can_read = 1 ;
	
	m_isPlaying = true;

	emit nowPlaying(QString(file).append("@--,--@").append(calcTimeString((int)sndfileinfo.frames)), sndfileinfo.frames);

	while (!info.play_done)
	{	
		printTime() ;
		usleep (50000) ;
	}
	printTime();
	//memset (ringbuf->buf, 0, ringbuf->size);

	m_isPlaying = false;

	info.can_process = 0;
	
	stopClient();
	sf_close (sndfile) ;
	info.sndfile = 0;
	emit playbackStopped(true);
}/*}}}*/

bool AudioPlayer::isPlaying()
{
	return m_isPlaying;
}

void AudioPlayer::stop()
{
	info.play_done = 1;
}
