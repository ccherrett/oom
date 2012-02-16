//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: citem.cpp,v 1.2.2.3 2008/01/26 07:23:21 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "part.h"
#include "citem.h"
#include <stdio.h>

//---------------------------------------------------------
//   CItem
//---------------------------------------------------------

CItem::CItem()
{
	_isMoving = false;
	_part = 0;
	_zValue = 0;
}

CItem::CItem(const QPoint&p, const QRect& r)
{
	_pos = p;
	_bbox = r;
	_isMoving = false;
	_part = 0;
	_zValue = 0;
}

CItem::CItem(const Event& e, Part* p)
{
	_event = e;
	_part = p;
	_zValue = 0;
	if(p)
	{
		_zValue = p->getZIndex();
	}
	_isMoving = false;
}

void CItem::setZValue(int value, bool updatePart)
{
	_zValue = value;
	if(_part && updatePart)
		_part->setZIndex(value);
}

int CItem::zValue(bool fromPart) const 
{
	if(_part && fromPart)
		return _part->getZIndex();
	return _zValue;
}

void CItem::setPart(Part* p) {
    _part = p;
	if(p)
	{
		_zValue = p->getZIndex();
	}
}

//---------------------------------------------------------
//   isSelected
//---------------------------------------------------------

bool CItem::isSelected() const
{
	return _event.empty() ? _part->selected() : _event.selected();
}

//---------------------------------------------------------
//   setSelected
//---------------------------------------------------------

void CItem::setSelected(bool f)
{
	_event.empty() ? _part->setSelected(f) : _event.setSelected(f);
}

//---------------------------------------------------------
//   CItemList
//---------------------------------------------------------

CItem* CItemList::find(const QPoint& pos) const/*{{{*/
{
	rciCItem ius;
	bool usfound = false;
	for (rciCItem i = rbegin(); i != rend(); ++i)
	{
		if (i->second->contains(pos))
		{
			if (i->second->isSelected())
				return i->second;

			else
			{
				if (!usfound)
				{
					ius = i;
					usfound = true;
				}
			}
		}
	}
	if (usfound)
		return ius->second;
	else
		return 0;
}/*}}}*/

//---------------------------------------------------------
//   CItemList
//---------------------------------------------------------

void CItemList::add(CItem* item)
{
	std::multimap<int, CItem*, std::less<int> >::insert(std::pair<const int, CItem*> (item->bbox().x(), item));
}

int CItemList::selectionCount()/*{{{*/
{
	rciCItem ius;
	int rv = 0;
	for (rciCItem i = rbegin(); i != rend(); ++i)
	{
		if (i->second->isSelected())
			++rv;
	}
	return rv;
}/*}}}*/
