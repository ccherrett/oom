#include "cthread.h"
#include <QtNetwork>

#include "song.h"
#include "globals.h"
#include "config.h"
#include "gconfig.h"
#include "midiport.h"
#include "minstrument.h"
#include "mididev.h"
#include "utils.h"
#include "audio.h"
#include "midi.h"
#include "midictrl.h"
#include "app.h"
#include "route.h"
#include "track.h"


OOMClientThread::OOMClientThread(int sock, QObject* parent) 
	: QThread(parent), socket(sock)
{
	//cmdStrList = new QHash<String, int>;
		//SHOW_TRACKS=0, SHOW_INPUTS, SHOW_OUTPUTS, SHOW_BUSSES, SHOW_AUDIO, SHOW_SYNTHS, SHOW_AUXES,
		//START, STOP, PIPELINE_STOPED, PIPELINE_STARTED, RELOAD_ROUTES, SAVE_SONG, SAVE_SONG_AS
	cmdStrList.insert("show_tracks", SHOW_TRACKS);
	cmdStrList.insert("show_inputs", SHOW_INPUTS);
	cmdStrList.insert("show_outputs", SHOW_OUTPUTS);
	cmdStrList.insert("show_busses", SHOW_BUSSES);
	cmdStrList.insert("show_audio", SHOW_AUDIO);
	cmdStrList.insert("show_synths", SHOW_SYNTHS);
	cmdStrList.insert("show_auxes", SHOW_AUXES);
	cmdStrList.insert("stop", STOP);
	cmdStrList.insert("play", PLAY);
	cmdStrList.insert("pipeline_stopped", PIPELINE_STOPED);
	cmdStrList.insert("reload_routes", RELOAD_ROUTES);
	cmdStrList.insert("song_save", SAVE_SONG);
	cmdStrList.insert("song_saveas", SAVE_SONG_AS);
	cmdStrList.insert("pipeline_started", PIPELINE_STARTED);
	cmdStrList.insert("current_song", CURRENT_SONG);
	cmdStrList.insert("current_song_file", CURRENT_SONG_FILE);
	cmdStrList.insert("save_and_exit", SAVE_AND_EXIT);
}

void OOMClientThread::run()
{
	QString rv;
	QTcpSocket tcpSocket;
	if(!tcpSocket.setSocketDescriptor(socket))
	{
		emit error(tcpSocket.error());
		return;
	}
	

	tcpSocket.waitForReadyRead(5000000);

	bool clean = true;
	QByteArray a;
	if(tcpSocket.canReadLine())
		a = tcpSocket.readLine(1024);
	else
		clean = false;
    
	if(a.endsWith("\n")) {
		if(a.endsWith("\r\n")) 
			a.chop(2); 
		else
			a.chop(1);
	}
	QString cd(a);

	//Proccess the command list and return any values or error.
	if(clean && cmdStrList.contains(cd))
	{
		//printf("Found in list\n");
		ServerCommand cmd = (ServerCommand)cmdStrList.value(cd);
		switch(cmd)
		{
			case SHOW_TRACKS:
				//rv.append("show tracks\n");
				for(ciTrack ci = song->tracks()->begin(); ci != song->tracks()->end(); ++ci)
				{
					rv.append((*ci)->name()+"\n");
				}
			break;
			case SHOW_INPUTS:
				//rv.append("show inputs\n");
				for(ciTrack ci = song->inputs()->begin(); ci != song->inputs()->end(); ++ci)
				{
					rv.append((*ci)->name()+"\n");
				}
			break;
			case SHOW_OUTPUTS:
				rv.append("show outputs\n");
				for(ciTrack ci = song->outputs()->begin(); ci != song->outputs()->end(); ++ci)
				{
					rv.append((*ci)->name()+"\n");
				}
			break;
			case SHOW_BUSSES:
				rv.append("show busses\n");
				for(ciTrack ci = song->groups()->begin(); ci != song->groups()->end(); ++ci)
				{
					rv.append((*ci)->name()+"\n");
				}
			break;
			case SHOW_AUDIO:
				rv.append("show audio\n");
				for(ciTrack ci = song->waves()->begin(); ci != song->waves()->end(); ++ci)
				{
					rv.append((*ci)->name()+"\n");
				}
			break;
			case SHOW_SYNTHS:
                //rv.append("show synths\n");
                //for(ciTrack ci = song->syntis()->begin(); ci != song->syntis()->end(); ++ci)
                //{
                //	rv.append((*ci)->name()+"\n");
                //}
			break;
			case SHOW_AUXES:
				rv.append("show auxes\n");
				for(ciTrack ci = song->auxs()->begin(); ci != song->auxs()->end(); ++ci)
				{
					rv.append((*ci)->name()+"\n");
				}
			break;
			case PLAY:
				rv.append("Start Playback Called\n");
				song->setPlay(true);
			break;
			case STOP:
				rv.append("Stop Playback Called\n");
				song->setStop(true);
			break;
			case PIPELINE_STOPED: //update a flag in oom that the pipeline is currently broken
				rv.append("Pipeline Stopped Called\n");
				emit pipelineStateChanged(0);
			break;
			case PIPELINE_STARTED: //update a flag in oom that the pipeline is currently repaired and running:
				rv.append("Pipeline Started Called\n");
				emit pipelineStateChanged(1);
			break;
			case RELOAD_ROUTES:
				rv.append("Reload Routes Called\n");
				emit reloadRoutes();//oom->connectDefaultSongPorts();
			break;
			case SAVE_SONG:
                rv.append("Song Save Called\n");
				emit saveSong();
			break;
			case SAVE_SONG_AS:
				rv.append("Song SaveAs called\n");
				emit saveSongAs();
			break;
			case SAVE_AND_EXIT:
			{
				printf("Save and Exit Call requested in command server\n");
				emit saveAndExit(true);
				rv.append("OK").append("\n");
			}
			break;
			case CURRENT_SONG:
				rv.append(oomProject);
				rv.append("\n");
			break;
			case CURRENT_SONG_FILE:
				rv.append(oomProjectFile);
				rv.append("\n");
			break;
			default:
				rv.append("OOMidi Error - Unknown Command: ");
				rv.append(cd);
				rv.append("\n");
			break;
		}
	}
	else
	{
		rv.append("OOMidi Error - Unknown Command: ");
		rv.append(cd);
		rv.append("\n");
	}

	//printf("Inside client thread\n");
	QByteArray block;
	QByteArray msg = rv.toUtf8();
	//QByteArray data = "MESSAGE " + QByteArray::number(msg.size()) + ' ' + msg;
	
	tcpSocket.write(msg);
	tcpSocket.disconnectFromHost();
	tcpSocket.waitForDisconnected();
												  
}
