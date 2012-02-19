//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: splitter.cpp,v 1.1.1.1 2003/10/27 18:54:59 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "splitter.h"
#include "xml.h"
#include "traverso_shared/TConfig.h"

#include <QList>
#include <QStringList>
#include <QShowEvent>

//---------------------------------------------------------
//   Splitter
//---------------------------------------------------------

Splitter::Splitter(Qt::Orientation o, QWidget* parent, const char* name)
: QSplitter(o, parent)
{
	setObjectName(name);
	setOpaqueResize(true);
	//connect(this, SIGNAL(splitterMoved(int, int)), this, SLOT(saveStateInfo()));
}

Splitter::Splitter(QWidget *parent) : QSplitter(parent)
{
}

Splitter::~Splitter()
{
	/*
	QList<int> vl = sizes();
	QString val;
	QList<int>::iterator ivl = vl.begin();
	for (; ivl != vl.end(); ++ivl)
	{
		val.append(QString::number(*ivl));
		val.append(" ");
	}
	//printf("Inside the Splitter Destructor: %s\n", val.toStdString().c_str());
	tconfig().set_property(objectName(), "sizes", val);
	//For whatever reason OOMidi destructor is called before this so call tconfig().save() again
    tconfig().save();
	*/
	//saveStateInfo();
}

void Splitter::saveStateInfo()
{
	QList<int> vl = sizes();
	QString val;
	QList<int>::iterator ivl = vl.begin();
	for (; ivl != vl.end(); ++ivl)
	{
		val.append(QString::number(*ivl));
		val.append(" ");
	}
	//printf("Inside the Splitter Destructor: %s\n", val.toStdString().c_str());
	tconfig().set_property(objectName(), "sizes", val);
	//For whatever reason OOMidi destructor is called before this so call tconfig().save() again
    tconfig().save();
}

void Splitter::showEvent(QShowEvent*)
{
	/*if(!m_configured)
	{
		QList<int> vl;
	
		//QString str = tconfig().get_property(objectName(), "sizes", "200 50").toString();
		//QStringList sl = str.split(QString(" "), QString::SkipEmptyParts);
		for (QStringList::Iterator it = sl.begin(); it != sl.end(); ++it)
		{
			int val = (*it).toInt();
			vl.append(val);
		}
	
		//setSizes(vl);
		m_configured = true;
	}*/
}

//---------------------------------------------------------
//   saveConfiguration
//---------------------------------------------------------

void Splitter::writeStatus(int level, Xml& xml)
{
	QList<int> vl = sizes();
	QString val;
	//xml.nput(level++, "<%s>", name());
	xml.nput(level++, "<%s>", Xml::xmlString(objectName()).toLatin1().constData());
	QList<int>::iterator ivl = vl.begin();
	for (; ivl != vl.end(); ++ivl)
	{
		val.append(QString::number(*ivl));
		val.append(" ");
		xml.nput("%d ", *ivl);
	}
	//xml.nput("</%s>\n", name());
	xml.nput("</%s>\n", Xml::xmlString(objectName()).toLatin1().constData());
	tconfig().set_property(objectName(), "sizes", val);
}

//---------------------------------------------------------
//   loadConfiguration
//---------------------------------------------------------

void Splitter::readStatus(Xml& /*xml*/)
{
	/*QList<int> vl;

	QString str = tconfig().get_property(objectName(), "sizes", "200 50").toString();
	QStringList sl = str.split(QString(" "), QString::SkipEmptyParts);
	for (QStringList::Iterator it = sl.begin(); it != sl.end(); ++it)
	{
		int val = (*it).toInt();
		vl.append(val);
	}

	setSizes(vl);
	return;*/
	/*for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				xml.unknown("Splitter");
				break;
			case Xml::Text:
			{
				//QStringList sl = QStringList::split(' ', tag);
			}
				break;
			case Xml::TagEnd:
				if (tag == objectName())
				{
				}
			default:
				break;
		}
	}*/
}

QSize Splitter::sizeHint() const
{
	QSize rv(1,1);
	/*switch(orientaton())
	{
		case Qt::Horizontal:
			rv = QSize(1,);
		break;
		case Qt::Vertical:
		break;
	}*/
	return rv;
}
