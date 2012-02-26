//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: citem.h,v 1.2.2.1 2006/10/04 18:45:35 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CITEM_H__
#define __CITEM_H__

#include <map>
#include <QPoint>
#include <QRect>

#include "event.h"

class Event;
class Part;

//---------------------------------------------------------
//   CItem
//    virtuelle Basisklasse fr alle Canvas Item's
//---------------------------------------------------------

class CItem {
private:
    Event _event;
    Part* _part;
    int _zValue;

protected:
    bool _isMoving;
    QPoint moving;
    QRect _bbox;
    QPoint _pos;

public:
    CItem(const QPoint& p, const QRect& r);
    CItem(const QPoint& p, const QRect& r, bool moving);
    CItem();
    CItem(const Event& e, Part* p);

    bool isMoving() const {
        return _isMoving;
    }

    void setMoving(bool f) {
        _isMoving = f;
    }
    bool isSelected() const;
    void setSelected(bool f);

    int width() const {
        return _bbox.width();
    }

    int zValue(bool fromPart = false) const; 

    void setZValue(int value, bool updatePart = false);
	
    void setWidth(int l) {
        _bbox.setWidth(l);
    }

    void setHeight(int l) {
        _bbox.setHeight(l);
    }

    void setMp(const QPoint&p) {
        moving = p;
    }

    const QPoint mp() const {
        return moving;
    }

    int x() const {
        return _pos.x();
    }

    int y() const {
        return _pos.y();
    }

    void setY(int y) {
        _bbox.setY(y);
    }

    QPoint pos() const {
        return _pos;
    }

    void setPos(const QPoint& p) {
        _pos = p;
    }

    int height() const {
        return _bbox.height();
    }

    const QRect& bbox() const {
        return _bbox;
    }

    void setBBox(const QRect& r) {
        _bbox = r;
    }

    void move(const QPoint& tl) {
        _bbox.moveTopLeft(tl);
        _pos = tl;
    }

    bool contains(const QPoint& p) const {
        return _bbox.contains(p);
    }

    bool intersects(const QRect& r) const {
        return r.intersects(_bbox);
    }

    Event event() const {
        return _event;
    }

    void setEvent(Event& e) {
        _event = e;
    }

    Part* part() const {
        return _part;
    }

    void setPart(Part* p);
};

typedef std::multimap<int, CItem*, std::less<int> >::iterator iCItem;
//typedef std::multimap<int, CItem*, std::less<int> >::const_iterator ciCItem;
typedef std::multimap<int, CItem*, std::less<int> >::const_reverse_iterator rciCItem;

//---------------------------------------------------------
//   CItemList
//    Canvas Item List
//---------------------------------------------------------

class CItemList : public std::multimap<int, CItem*, std::less<int> > {
public:
    void add(CItem*);
    CItem* find(const QPoint& pos) const;
	int selectionCount();
};

#endif

