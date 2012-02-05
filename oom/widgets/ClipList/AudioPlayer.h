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

#ifndef _OOM_AUDIO_CLIP_PLAYER_
#define _OOM_AUDIO_CLIP_PLAYER_

#include <QObject>
#include <pthread.h>

#include <jack/jack.h>
#include <sndfile.h>

#define RB_SIZE (1 << 16)

typedef struct _thread_info
{
	pthread_t thread_id ;
	SNDFILE *sndfile ;
	jack_nframes_t pos ;
	jack_client_t *client ;
	unsigned int channels ;
	volatile int can_process ;
	volatile int can_read;
	volatile int read_done ;
	volatile int play_done ;
	volatile float volume;
	volatile int seek;
} thread_info_t ;
							

class AudioPlayer : public QObject
{
	Q_OBJECT;

	jack_client_t *m_client;
	thread_info_t info;
	int m_srate;

	bool m_isPlaying;
	bool m_seeking;
	float m_oldVolume;

	//Process the audio file
	static int process (jack_nframes_t nframes, void * arg);
	//Read audio file from disk
	static void* read_file(void* arg);
	void printTime();
	//Start jack client;
	bool startClient();
	void stopClient();

signals:
	void timeChanged(const QString&);
	void timeChanged(int);
	void playbackStopped(bool);
	void nowPlaying(const QString&, int samples);
	void readyForPlay();

private slots:
	void stopSeek();
	void restoreVolume();

public slots:
	void stop();
	void check();
	void setVolume(double value);
	void seek(int);

public:
	AudioPlayer();
	~AudioPlayer();
	void play(const QString& filename);
	bool isPlaying();
	QString calcTimeString(int);
};
#endif
