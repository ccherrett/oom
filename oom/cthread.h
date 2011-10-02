#ifndef _OOM_CTHREAD_
#define _OOM_CTHREAD_

#include <QThread>
#include <QTcpSocket>
#include <QHash>


class OOMClientThread : public QThread
{
	Q_OBJECT

	public:
	OOMClientThread(int socket, QObject* parent = 0);

	void run();

	enum ServerCommand {
		SHOW_TRACKS=0, SHOW_INPUTS, SHOW_OUTPUTS, SHOW_BUSSES, SHOW_AUDIO, SHOW_SYNTHS, SHOW_AUXES,
		PLAY, STOP, PIPELINE_STOPED, PIPELINE_STARTED, RELOAD_ROUTES, SAVE_SONG, SAVE_SONG_AS, CURRENT_SONG,
		CURRENT_SONG_FILE, SAVE_AND_EXIT
	};

	signals:
	     void error(QTcpSocket::SocketError socketError);
		 void saveSong();
		 void saveSongAs();
		 void saveAndExit(bool);
		 void pipelineStateChanged(int);
		 void reloadRoutes();

	private:
		int socket;
		QHash<QString, int> cmdStrList;

};

#endif
