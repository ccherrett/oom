//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: ctrl.cpp,v 1.1.2.4 2009/06/10 00:34:59 terminator356 Exp $
//
//    controller handling for mixer automation
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//=========================================================


#include <QLocale>
#include <QColor>
//#include <stdlib.h>

#include "globals.h"
#include "ctrl.h"
#include "xml.h"
// #include "audio.h"

void CtrlList::initColor(int i)
{
	QColor collist[] = { QColor(255,90,0),
						 QColor(0,219,5), 
						 QColor(219,0,0), 
						 QColor(0,219,216), 
						 QColor(219,211,0), 
						 QColor(0,15,219), 
						 QColor(0,219,93), 
						 QColor(219,93,0), 
						 QColor(0,219,72), 
						 QColor(180,0,219), 
						 QColor(0,168,255), 
						 QColor(255,0,252), 
						 QColor(255,0,0), 
						 QColor(255,210,0), 
						 QColor(0,6,255), 
						 QColor(0,255,0), 
						 QColor(126,255,0) , 
						 QColor(255,174,0), 
						 QColor(0,255,138), 
						 };
	//if (i < 5)
	//{
		//_displayColor = collist[i%6];
		_displayColor = collist[i%19];
	//}
	//else
	//{
	//	_displayColor = Qt::gray;
	//}

	_displayColor.setAlpha(255);

	//_visible = false;
}



//---------------------------------------------------------
//   CtrlList
//---------------------------------------------------------

CtrlList::CtrlList(int id)
{
	_id = id;
	_default = 0.0;
	_curVal = 0.0;
	_mode = INTERPOLATE;
	_dontShow = false;
	_selected = false;
	_visible = false;
	initColor(id);
}
//---------------------------------------------------------
//   CtrlList
//---------------------------------------------------------

CtrlList::CtrlList(int id, QString name, double min, double max, bool dontShow)
{
	_id = id;
	_default = 0.0;
	_curVal = 0.0;
	_mode = INTERPOLATE;
	_name = name;
	_min = min;
	_max = max;
	_dontShow = dontShow;
	_selected = false;
	_visible = false;
	initColor(id);
}
//---------------------------------------------------------
//   CtrlList
//---------------------------------------------------------

CtrlList::CtrlList()
{
	_id = 0;
	_default = 0.0;
	_curVal = 0.0;
	_mode = INTERPOLATE;
	 _dontShow = false;
	 _selected = false;
	 _visible = false;
	initColor(-1);
}

//---------------------------------------------------------
//   value
//---------------------------------------------------------

double CtrlList::value(int frame)/*{{{*/
{
	if (!automation || empty())
	{
		return _curVal;
	}
	ciCtrl i = upper_bound(frame);
	if (i == end())
	{
		ciCtrl i = end();
		--i;
		const CtrlVal& val = i->second;
		_curVal = val.val;
	}
	else
		if (_mode == DISCRETE)
	{
		if (i == begin())
			_curVal = _default;
		else
		{
			--i;
			const CtrlVal& val = i->second;
			_curVal = val.val;
		}
	}
	else
	{
		int frame2 = i->second.getFrame();
		double val2 = i->second.val;
		int frame1;
		double val1;
		if (i == begin())
		{
			frame1 = 0;
			val1 = _default;
		}
		else
		{
			--i;
			frame1 = i->second.getFrame();
			val1 = i->second.val;
		}
		frame -= frame1;
		val2 -= val1;
		frame2 -= frame1;
		val1 += (frame * val2) / frame2;
		_curVal = val1;
	}
	// printf("autoVal %d %f\n", frame, _curVal);
	return _curVal;
}/*}}}*/

const CtrlVal CtrlList::cvalue(int frame)/*{{{*/
{
	if (!automation || empty())
	{
		return CtrlVal(-1, -1);
	}
	ciCtrl i = upper_bound(frame);
	if (i == end())
	{
		ciCtrl i = end();
		--i;
		const CtrlVal& val = i->second;
		return val;
	}
	else if (_mode == DISCRETE)
	{
		if (i == begin())
		{	
			const CtrlVal& val = i->second;
			return val;
		}
		else
		{
			--i;
			const CtrlVal& val = i->second;
			return val;
		}
	}
	// printf("autoVal %d %f\n", frame, _curVal);
	return CtrlVal(-1, -1);
}/*}}}*/

//---------------------------------------------------------
//   setCurVal
//---------------------------------------------------------

void CtrlList::setCurVal(double val)
{
	_curVal = val;
	if (size() < 2)
	{
		add(0, val);
	}
}

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void CtrlList::add(int frame, double val)
{
	iCtrl e = find(frame);
	if (e != end())
	{
		e->second.val = val;
	}
	else
		insert(std::pair<const int, CtrlVal > (frame, CtrlVal(frame, val)));
}

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void CtrlList::del(int frame)
{
	iCtrl e = find(frame);
	if (e == end())
	{
		//printf("CtrlList::del(%d): not found\n", frame);
		return;
	}
	erase(e);
}

// sets a new frame value for a CtrlVal reference
// since the frame value is the 'key' in our 'map'
// we have to 'update' the key value in the map too
// which is to my knowledge only possible by removing
// the original entry, and inserting the modified one.
CtrlVal& CtrlList::setCtrlFrameValue(CtrlVal* ctrl, int frame)
{
	del(ctrl->getFrame());
	add(frame, ctrl->val);
	iCtrl e = find(frame);
	return e->second;
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void CtrlList::read(Xml& xml)
{
	QLocale loc = QLocale::c();
	bool ok;
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::Attribut:
				if (tag == "id")
				{
					_id = loc.toInt(xml.s2(), &ok);
					if (!ok)
					{
						printf("CtrlList::read failed reading _id string: %s\n", xml.s2().toLatin1().constData());
						initColor(0); //Set the default color if we have an error
					}
					else {
						initColor(_id);
					}
				}
				else if (tag == "cur")
				{
					_curVal = loc.toDouble(xml.s2(), &ok);
					if (!ok)
						printf("CtrlList::read failed reading _curVal string: %s\n", xml.s2().toLatin1().constData());
				}
				else if(tag == "visible")
				{
					_visible = (bool)xml.s2().toInt();
				}
				else if(tag == "color")
				{
					;//xml.skip(tag);
				}
				else
					printf("unknown tag %s\n", tag.toLatin1().constData());
				break;
			case Xml::Text:
			{
				int len = tag.length();
				int frame;
				double val;

				int i = 0;
				for (;;)
				{
					while (i < len && (tag[i] == ',' || tag[i] == ' ' || tag[i] == '\n'))
						++i;
					if (i == len)
						break;

					QString fs;
					while (i < len && tag[i] != ' ')
					{
						fs.append(tag[i]);
						++i;
					}
					if (i == len)
						break;

					// Works OK, but only because if current locale fails it falls back on 'C' locale.
					// So, let's skip the fallback and force use of 'C' locale.
					frame = loc.toInt(fs, &ok);
					if (!ok)
					{
						printf("CtrlList::read failed reading frame string: %s\n", fs.toLatin1().constData());
						break;
					}

					while (i < len && (tag[i] == ' ' || tag[i] == '\n'))
						++i;
					if (i == len)
						break;

					QString vs;
					while (i < len && tag[i] != ' ' && tag[i] != ',')
					{
						vs.append(tag[i]);
						++i;
					}

					// Works OK, but only because if current locale fails it falls back on 'C' locale.
					// So, let's skip the fallback and force use of 'C' locale.
					//val = vs.toDouble(&ok);
					val = loc.toDouble(vs, &ok);
					if (!ok)
					{
						printf("CtrlList::read failed reading value string: %s\n", vs.toLatin1().constData());
						break;
					}

					//printf("CtrlList::read i:%d len:%d fs:%s frame %d: vs:%s val %f \n", i, len, fs.toLatin1().constData(), frame, vs.toLatin1().constData(), val);

					add(frame, val);

					if (i == len)
						break;
				}
			}
				break;
			case Xml::TagEnd:
				if (xml.s1() == "controller")
				{
					return;
				}
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void CtrlListList::add(CtrlList* vl)
{
	//      printf("CtrlListList(%p)::add(id=%d) size %d\n", this, vl->id(), size());
	insert(std::pair<const int, CtrlList*>(vl->id(), vl));
}

void CtrlListList::deselectAll()
{
	for(CtrlListList::iterator icll = begin(); icll != end(); ++icll)
	{
		icll->second->setSelected(false);
	}
}
