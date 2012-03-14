//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: app.cpp,v 1.113.2.68 2009/12/21 14:51:51 spamatica Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <QApplication>
#include <QDir>
#include <QKeyEvent>
#include <QMessageBox>
#include <QLocale>
#include <QSplashScreen>
#include <QTimer>
#include <QTranslator>

#include <signal.h>
#include <sys/mman.h>
#include <alsa/asoundlib.h>

#include "al/dsp.h"
#include "app.h"
#include "audio.h"
#include "audiodev.h"
#include "gconfig.h"
#include "globals.h"
#include "icons.h"
#include "sync.h"
#include "network/LSThread.h"

extern bool initDummyAudio();
extern void initIcons();
extern bool initJackAudio();
extern void initMidiController();
extern void initMetronome();
extern void initPlugins(bool ladspa, bool lv2, bool vst);
extern void initShortCuts();
extern void readConfiguration();

// used for plugins
void set_last_error(const char* error);

static QString locale_override;

#ifdef HAVE_LASH
#include <lash/lash.h>
extern lash_client_t * lash_client;
extern snd_seq_t * alsaSeq;
#endif

//GTK2 LV2 GUI support
#ifdef GTK2UI_SUPPORT
#undef signals 
#include <gtk/gtk.h>
#endif

//---------------------------------------------------------
//   getCapabilities
//---------------------------------------------------------

static void getCapabilities()
{
#ifdef RTCAP
#ifdef __linux__
	const char* napp = getenv("GIVERTCAP");
	if (napp == 0)
		napp = "givertcap";
	int pid = fork();
	if (pid == 0)
	{
		if (execlp(napp, napp, 0) == -1)
			perror("exec givertcap failed");
	}
	else if (pid == -1)
	{
		perror("fork givertcap failed");
	}
	else
	{
		waitpid(pid, 0, 0);
	}
#endif // __linux__
#endif
}

//---------------------------------------------------------
//   printVersion
//---------------------------------------------------------

static void printVersion(const char* prog)
{
	fprintf(stderr, "%s: OpenOctave Midi and Audio Editor; Version %s\n", prog, VERSION);
}

bool g_ladish_l1_save_requested = false;

static void ladish_l1_save(int sig)
{
	if (sig == SIGUSR1)
	{
		g_ladish_l1_save_requested = true;
	}
}

//Reconnect default project port state on SIGHUP
bool oom_reconnect_ports_requested = false;

static void oom_reconnect_default_ports(int sig)
{
	if(sig == SIGHUP)
	{
		oom_reconnect_ports_requested = true;
	}
}

//---------------------------------------------------------
//   OOMidiApplication
//---------------------------------------------------------

class OOMidiApplication : public QApplication
{
	OOMidi* oom;

public:

	OOMidiApplication(int& argc, char** argv)
	: QApplication(argc, argv)
	{
		oom = 0;
	}

	void setOOMidi(OOMidi* m)
	{
		oom = m;
		startTimer(300);
	}

	bool notify(QObject* receiver, QEvent* event)
	{
		//if (event->type() == QEvent::KeyPress)
		//  printf("notify key press before app::notify accepted:%d\n", event->isAccepted());  // REMOVE Tim
		try
		{
		if(!receiver || !event)
			return false;
		bool flag = QApplication::notify(receiver, event);
		if (event->type() == QEvent::KeyPress)
		{
			//printf("notify key press after app::notify accepted:%d\n", event->isAccepted());   // REMOVE Tim
			QKeyEvent* ke = (QKeyEvent*) event;
			globalKeyState = ke->modifiers();
			bool accepted = ke->isAccepted();
			if (!accepted)
			{
				int key = ke->key();
				if (((QInputEvent*) ke)->modifiers() & Qt::ShiftModifier)
					key += Qt::SHIFT;
				if (((QInputEvent*) ke)->modifiers() & Qt::AltModifier)
					key += Qt::ALT;
				if (((QInputEvent*) ke)->modifiers() & Qt::ControlModifier)
					key += Qt::CTRL;
				oom->kbAccel(key);
				return true;
			}
		}
		if (event->type() == QEvent::KeyRelease)
		{
			QKeyEvent* ke = (QKeyEvent*) event;
			globalKeyState = ke->modifiers();
		}

		return flag;
		}
		catch(std::exception e)
		{
			return false;
		}
	}

	virtual void timerEvent(QTimerEvent * /* e */)
	{
#ifdef HAVE_LASH
		if (useLASH)
			oom->lash_idle_cb();
#endif /* HAVE_LASH */
		if (g_ladish_l1_save_requested)
		{
			g_ladish_l1_save_requested = false;
			printf("ladish L1 save request\n");
			oom->save();
		}
		if(oom_reconnect_ports_requested)
		{
			oom_reconnect_ports_requested = false;
			printf("OOMidi reconnect ports called from SIGHUP\n");
			oom->connectDefaultSongPorts();
		}
	}

};

//---------------------------------------------------------
//   localeList
//---------------------------------------------------------

static QString localeList()
{
	// Find out what translations are available:
	QStringList deliveredLocaleListFiltered;
	QString distLocale = oomGlobalShare + "/locale";
	QFileInfo distLocaleFi(distLocale);
	if (distLocaleFi.isDir())
	{
		QDir dir = QDir(distLocale);
		QStringList deliveredLocaleList = dir.entryList();
		for (QStringList::iterator it = deliveredLocaleList.begin(); it != deliveredLocaleList.end(); ++it)
		{
			QString item = *it;
			if (item.endsWith(".qm"))
			{
				int inipos = item.indexOf("oom_") + 5;
				int finpos = item.lastIndexOf(".qm");
				deliveredLocaleListFiltered << item.mid(inipos, finpos - inipos);
			}
		}
		return deliveredLocaleListFiltered.join(",");
	}
	return QString("No translations found!");
}

//---------------------------------------------------------
//   usage
//---------------------------------------------------------

static void usage(const char* prog, const char* txt)
{
	fprintf(stderr, "%s: %s\nusage: %s flags midifile\n   Flags:\n",
			prog, txt, prog);
	fprintf(stderr, "   -h       this help\n");
	fprintf(stderr, "   -v       print version\n");
	fprintf(stderr, "   -d       debug mode: no threads, no RT\n");
	fprintf(stderr, "   -D       debug mode: enable some debug messages\n");
	fprintf(stderr, "   -m       debug mode: trace midi Input\n");
	fprintf(stderr, "   -M       debug mode: trace midi Output\n");
	fprintf(stderr, "   -s       debug mode: trace sync\n");
	fprintf(stderr, "   -a       no audio\n");
	fprintf(stderr, "   -P  n    set audio driver real time priority to n (Dummy only, default 40. Else fixed by Jack.)\n");
	fprintf(stderr, "   -Y  n    force midi real time priority to n (default: audio driver prio +2)\n");
	fprintf(stderr, "   -p       don't load LADSPA plugins\n");
#ifdef JACK_SESSION_SUPPORT
	fprintf(stderr, "   -U       Jack session UUID\n");
#endif
#ifdef ENABLE_PYTHON
	fprintf(stderr, "   -y       enable Python control support\n");
#endif
#ifdef HAVE_LASH
	fprintf(stderr, "   -L       don't use LASH\n");
#endif
	fprintf(stderr, "   -l  xx   force locale to the given language/country code (xx = %s)\n", localeList().toLatin1().constData());
}

//---------------------------------------------------------
//   main
//---------------------------------------------------------

int main(int argc, char* argv[])
{

	//      error = ErrorHandler::create(argv[0]);
	Q_INIT_RESOURCE(oom);
	ruid = getuid();
	euid = geteuid();
	undoSetuid();
	getCapabilities();
	int noAudio = false;

	oomUser = QDir::homePath();//QString(getenv("HOME"));
	oomGlobalLib = QString(LIBDIR);
	oomGlobalShare = QString(SHAREDIR);
	oomProject = oomProjectInitPath; //getcwd(0, 0);
	oomInstruments = oomGlobalShare + QDir::separator() + QString("instruments");

	// Create config dir if it doesn't exists
	QDir cPath = QDir(configPath);
	if (!cPath.exists())
		cPath.mkpath(".");
	
	//Create the directory to store the internal routing map
	QDir rPath = QDir(routePath);
	if(!rPath.exists())
		rPath.mkpath(".");

#ifdef HAVE_LASH
	lash_args_t * lash_args = 0;
	if (useLASH)
		lash_args = lash_extract_args(&argc, &argv);
#endif

#ifdef GTK2UI_SUPPORT
	gtk_init(&argc, &argv);
#endif

	srand(time(0)); // initialize random number generator
	initMidiController();
	QApplication::setColorSpec(QApplication::ManyColor);
	OOMidiApplication app(argc, argv);
	app.setStyleSheet("QMessageBox{background-color: #595966;} QPushButton{border-radius: 3px; padding: 5px; background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,stop: 0 #626272, stop:0.1 #5b5b6b, stop: 1.0 #4d4d5b); border: 1px solid #393941; font-family: fixed-width;	font-weight: bold; font-size: 11px; color: #d0d4d0; } QPushButton:pressed, QPushButton::checked, QPushButton::hover { color: #e2e5e5; border-radius: 3px; padding: 3px; border: 1px solid #181819; background-color: #393941; }");
	QPalette p = QApplication::palette();
	p.setColor(QPalette::Disabled, QPalette::Light, QColor(89,89,102));
	QApplication::setPalette(p);

	initShortCuts();
	readConfiguration();

	oomUserInstruments = config.userInstrumentsDir;

	if (config.useDenormalBias)
		printf("Denormal protection enabled.\n");
	// SHOW SPLASH SCREEN
	if (config.showSplashScreen)
	{
		QPixmap splsh(oomGlobalShare + "/splash.png");

		if (!splsh.isNull())
		{
			QSplashScreen* oom_splash = new QSplashScreen(splsh,
					Qt::WindowStaysOnTopHint);
			oom_splash->setAttribute(Qt::WA_DeleteOnClose); // Possibly also Qt::X11BypassWindowManagerHint
			oom_splash->show();
			QTimer* stimer = new QTimer(0);
			oom_splash->connect(stimer, SIGNAL(timeout()), oom_splash, SLOT(close()));
			stimer->setSingleShot(true);
			stimer->start(6000);
		}
	}
	
	//Start Linuxsampler
	if(config.lsClientStartLS)
	{
		gLSThread = new LSThread();
		gLSThread->start();
	}

	int i;

	QString optstr("ahvdDmMsP:Y:l:py");
#ifdef HAVE_LASH
	optstr += QString("L");
#endif
#ifdef JACK_SESSION_SUPPORT
	optstr += QString("U");
#endif

	while ((i = getopt(argc, argv, optstr.toLatin1().constData())) != EOF)
	{
		char c = (char) i;
		switch (c)
		{
			case 'v': printVersion(argv[0]);
				return 0;
			case 'd':
				debugMode = true;
				realTimeScheduling = false;
				break;
			case 'a':
				noAudio = true;
				break;
			case 'D': debugMsg = true;
				break;
			case 'm': midiInputTrace = true;
				break;
			case 'M': midiOutputTrace = true;
				break;
			case 's': debugSync = true;
				break;
			case 'P': realTimePriority = atoi(optarg);
				break;
			case 'Y': midiRTPrioOverride = atoi(optarg);
				break;
			case 'p': loadPlugins = false;
				break;
			case 'L': useLASH = false;
				break;
			case 'y': usePythonBridge = true;
				break;
			case 'l': locale_override = QString(optarg);
				break;
			case 'U': gJackSessionUUID = QString(optarg);
				break;
			case 'h': usage(argv[0], argv[1]);
				return -1;
			default: usage(argv[0], "bad argument");
				return -1;
		}
	}

	AL::initDsp();

	if (debugMsg)
		printf("Start euid: %d ruid: %d, Now euid %d\n",
			euid, ruid, geteuid());
	if (debugMode)
	{
		initDummyAudio();
		realTimeScheduling = false;
	}
	else if (noAudio)
	{
		initDummyAudio();
		realTimeScheduling = true;
	}
	else if (initJackAudio())
	{
		if (!debugMode)
		{
			QMessageBox::critical(NULL, "OOStudio fatal error", "OOStudio <b>failed</b> to find a <b>Jack audio server</b>.<br><br>"
					"<i>OOStudio will continue without audio support (-a switch)!</i><br><br>"
					"If this was not intended check that Jack was started. "
					"If Jack <i>was</i> started check that it was\n"
					"started as the same user as OOMidi.\n");

			initDummyAudio();
			noAudio = true;
			realTimeScheduling = true;
			if (debugMode)
			{
				realTimeScheduling = false;
			}
		}
		else
		{
			fprintf(stderr, "fatal error: no JACK audio server found\n");
			fprintf(stderr, "no audio functions available\n");
			fprintf(stderr, "*** experimental mode -- no play possible ***\n");
			initDummyAudio();
		}
		realTimeScheduling = true;
	}
	else
		realTimeScheduling = audioDevice->isRealtime();

	useJackTransport.setValue(true);
	fifoLength = 131072 / segmentSize;


	argc -= optind;
	++argc;

	if (debugMsg)
	{//TODO: Change all these debugMsg/debugMode stuff to just use qDebug
		printf("global lib:       <%s>\n", oomGlobalLib.toLatin1().constData());
		printf("global share:     <%s>\n", oomGlobalShare.toLatin1().constData());
		printf("oom home:        <%s>\n", oomUser.toLatin1().constData());
		printf("project dir:      <%s>\n", oomProject.toLatin1().constData());
		printf("user instruments: <%s>\n", oomUserInstruments.toLatin1().constData());
	}

	static QTranslator translator(0);
	QString locale(QApplication::keyboardInputLocale().name());
	if (locale_override.length())
		locale = locale_override;
	if (locale != "C")
	{
		QString loc("oom_");
		loc += locale;
		if (translator.load(loc, QString(".")) == false)
		{
			QString lp(oomGlobalShare);
			lp += QString("/locale");
			if (translator.load(loc, lp) == false)
			{
				printf("no locale <%s>/<%s>\n", loc.toLatin1().constData(), lp.toLatin1().constData());
			}
		}
		app.installTranslator(&translator);
	}

	if (locale == "de")
	{
		printf("locale de\n");
		hIsB = false;
	}

	if (loadPlugins)
		initPlugins(config.loadLADSPA, config.loadLV2, config.loadVST);

	initIcons();

	initMetronome();

	QApplication::addLibraryPath(oomGlobalLib + "/qtplugins");
	if (debugMsg)
	{
		QStringList list = app.libraryPaths();
		QStringList::Iterator it = list.begin();
		printf("QtLibraryPath:\n");
		while (it != list.end())
		{
			printf("  <%s>\n", (*it).toLatin1().constData());
			++it;
		}
	}

	oom = new OOMidi(argc, &argv[optind]);
	app.setOOMidi(oom);
	oom->setWindowIcon(*oomIcon);

	if (!debugMode)
	{
		if (mlockall(MCL_CURRENT | MCL_FUTURE))
			perror("WARNING: Cannot lock memory:");
	}

	oom->show();
	oom->seqStart();
	//Finally launch the server on port 8415
	oom->startServer();

#ifdef HAVE_LASH
	{
		lash_client = 0;
		if (useLASH)
		{
			int lash_flags = LASH_Config_File;
			const char *oom_name = PACKAGE_NAME;
			lash_client = lash_init(lash_args, oom_name, lash_flags, LASH_PROTOCOL(2, 0));
			lash_alsa_client_id(lash_client, snd_seq_client_id(alsaSeq));
			if (!noAudio)
			{
				// p3.3.38
				//char *jack_name = ((JackAudioDevice*)audioDevice)->getJackName();
				const char *jack_name = audioDevice->clientName();
				lash_jack_client_name(lash_client, jack_name);
			}
		}
	}
#endif /* HAVE_LASH */

	// ladish L1
	signal(SIGUSR1, ladish_l1_save);
	
	// OOMidi ports reconnect
	signal(SIGHUP, oom_reconnect_default_ports);

	int rv = app.exec();
	if (debugMsg)
		printf("app.exec() returned:%d\nDeleting main OOMidi object\n", rv);
	if(lsClient && lsClientStarted)
	{
		lsClient->stopClient();
	}
	oom->stopServer();
	delete oom;
	if (debugMsg)
		printf("Finished! Exiting main, return value:%d\n", rv);
	return rv;

}
