#include "aboutbox_impl.h"
#include "config.h"
#include "icons.h"

AboutBoxImpl::AboutBoxImpl()
{
	setupUi(this);
	imageLabel->setPixmap(*aboutOOMidiImage);
	QString version(VERSION);
	versionLabel->setText("Version: " + version);
}
