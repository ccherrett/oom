//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: help.cpp,v 1.7.2.4 2009/07/05 23:06:21 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <unistd.h>
#include <stdlib.h>

#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>

#include "app.h"
#include "globals.h"
#include "gconfig.h"
#include "icons.h"
#include "aboutbox_impl.h"

//---------------------------------------------------------
//   startHelpBrowser
//---------------------------------------------------------

void OOMidi::startHelpBrowser()
{
      QString oomHelp = QString("https://github.com/ccherrett/oom/wiki/Quick-Start-Manual");
      launchBrowser(oomHelp);
}

//---------------------------------------------------------
//   startHelpBrowser
//---------------------------------------------------------

void OOMidi::startHomepageBrowser()
{
      QString oomHome = QString("http://www.openoctave.org");

      launchBrowser(oomHome);
}

//---------------------------------------------------------
//   startBugBrowser
//---------------------------------------------------------

void OOMidi::startBugBrowser()
{
      //QString oomBugPage("http://www.openoctave.org/wiki/index.php/Report_a_bug");
      QString oomBugPage("http://www.openoctave.org/index.php/Report_a_bug");
      launchBrowser(oomBugPage);
}

//---------------------------------------------------------
//   about
//---------------------------------------------------------

void OOMidi::about()
{
      AboutBoxImpl ab;
      ab.show();
      ab.exec();
}

//---------------------------------------------------------
//   aboutQt
//---------------------------------------------------------

void OOMidi::aboutQt()
{
      QMessageBox::aboutQt(this, QString("OOMidi"));
}

void OOMidi::launchBrowser(QString &whereTo)
{
      if (! QDesktopServices::openUrl(QUrl(whereTo)))
      {
            QMessageBox::information(this, tr("Unable to launch help"), 
                                     tr("For some reason OOMidi has to launch the default\n"
                                        "browser on your machine."),
                                     QMessageBox::Ok, QMessageBox::Ok);
            printf("Unable to launch help\n");
      }
}
