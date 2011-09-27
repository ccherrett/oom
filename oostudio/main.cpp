#include <QtGui>
#include "OOStudio.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	if (!QSystemTrayIcon::isSystemTrayAvailable()) 
	{
		QMessageBox::critical(0, QObject::tr("OOMIDI: Studio"),
			QObject::tr("I couldn't detect any system tray on this system."));
		return 1;
	}
	QApplication::setQuitOnLastWindowClosed(false);
	QIcon icon(":/images/oom_icon.png");
	app.setWindowIcon(icon);
	OOStudio studio;
	studio.show();
	return app.exec();
}
