#include "aboutbox_impl.h"
#include "config.h"
#include "icons.h"

AboutBoxImpl::AboutBoxImpl()
{
	setupUi(this);
	imageLabel->setPixmap(*aboutOOMidiImage);
	QString version(VERSION);
	QString svnrevision(SVNVERSION);
	versionLabel->setText("Version: " + version);
}
