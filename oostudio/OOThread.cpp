//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "OOThread.h"
#include <QProcess>

OOThread::OOThread(QProcess* p, QObject* parent)
: QThread(parent),
  m_process(p)
{
}

void OOThread::run()
{
}

bool OOThread::startJob()
{
	return false;
}
