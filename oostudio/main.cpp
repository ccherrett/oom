#include <QtGui>
#include "OOStudio.h"
#include "icons.h"

int main(int argc, char* argv[])
{
	//Q_INIT_RESOURCE(oom);
	QApplication app(argc, argv);
	if (!QSystemTrayIcon::isSystemTrayAvailable()) 
	{
		QMessageBox::critical(0, QObject::tr("Systray"),
			QObject::tr("I couldn't detect any system tray on this system."));
		return 1;
	}
	QApplication::setQuitOnLastWindowClosed(false);
	OOStudio studio;
	studio.show();
	return app.exec();
}
