//=========================================================
//  OOStudio
//  OpenOctave Midi and Audio Editor

//  (C) Copyright 2011 Andrew Williams
//  (C) Copyright 2011 Christopher Cherrett
//  (C) Copyright 2011 Remon Sijrier
//	14-feb:	Automation Curves:
//		Fixed multiple drawing issues, highlight lazy selected node,
//		implemented CurveNodeSelection to move a selection of nodes.
//
//    $Id: $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <values.h>
#include <uuid/uuid.h>
#include <math.h>

#include <QClipboard>
#include <QToolTip>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QUrl>
#include <QComboBox>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>
#include <QDomNode>

#include "widgets/toolbars/tools.h"
#include "ComposerCanvas.h"
#include "AbstractMidiEditor.h"
#include "globals.h"
#include "icons.h"
#include "event.h"
#include "xml.h"
#include "wave.h"
#include "audio.h"
#include "shortcuts.h"
#include "gconfig.h"
#include "app.h"
#include "filedialog.h"
#include "marker/marker.h"
#include "Composer.h"
//#include "tlist.h"
#include "utils.h"
#include "midimonitor.h"
#include "ctrl.h"
#include "FadeCurve.h"
#include "traverso_shared/AddRemoveCtrlValues.h"
#include "traverso_shared/CommandGroup.h"
#include "CreateTrackDialog.h"


class CurveNodeSelection
{
private:
	QList<CtrlVal*> m_nodes;

public:
	void addNodeToSelection(CtrlVal* node)
	{
		m_nodes.append(node);
	}

	void removeNodeFromSelection(CtrlVal* node)
	{
		m_nodes.removeAll(node);
	}

	void clearSelection()
	{
		m_nodes.clear();
	}

	QList<CtrlVal*> getNodes() const
	{
		return m_nodes;
	}

	int size() const {return m_nodes.size();}

	double getMaxValue() const
	{
		double maxValue = MINDOUBLE;

		foreach(const CtrlVal* ctrlVal, m_nodes)
		{
			if (ctrlVal->val > maxValue)
			{
				maxValue = ctrlVal->val;
			}
		}
		return maxValue;
	}

	double getMinValue() const
	{
		double minValue = MAXDOUBLE;

		foreach(const CtrlVal* ctrlVal, m_nodes)
		{
			if (ctrlVal->val < minValue)
			{
				minValue = ctrlVal->val;
			}
		}
		return minValue;
	}

	int getEndFrame() const
	{
		int maxFrame = INT_MIN;
		foreach(const CtrlVal* ctrlVal, m_nodes)
		{
			if (ctrlVal->getFrame() > maxFrame)
			{
				maxFrame = ctrlVal->getFrame();
			}
		}

		return maxFrame;
	}

	int getStartFrame() const
	{
		int minFrame = INT_MAX;
		foreach(const CtrlVal* ctrlVal, m_nodes)
		{
			if (ctrlVal->getFrame() != 0 && ctrlVal->getFrame() < minFrame)
			{
				minFrame = ctrlVal->getFrame();
			}
		}

		return minFrame;
	}

	bool isSelected (CtrlVal* node)
	{
		return m_nodes.contains(node);
	}

	void move(int frameDiff, double valDiff, double min, double max, CtrlList* list, CtrlVal* lazySelectedCtrlVal, bool dBCalculation)
	{//We need to make a copy of all the nodes in our internal list so we can operate on them without updating the CtrlList
	//This way we can do the actions at once and provide undo.
		if (lazySelectedCtrlVal)
		{
			addNodeToSelection(lazySelectedCtrlVal);
		}

		double range = max - min;

		double maxNodeValue, minNodeValue;

		if (dBCalculation)
		{
			maxNodeValue = dbToVal(getMaxValue()) * range;
			minNodeValue = dbToVal(getMinValue()) * range;
		}
		else
		{
			maxNodeValue = getMaxValue();
			minNodeValue = getMinValue();
		}

		// should use the dbToVal() I guess, doesn't work as expected atm.
		if ((maxNodeValue + valDiff) > max)
		{
			valDiff = 0;
		}

		if ((minNodeValue + valDiff) < min)
		{
			valDiff = 0;
		}

		if (dBCalculation)
		{
			foreach(CtrlVal* ctrlVal, m_nodes)
			{
				double newCtrlVal = dbToVal(ctrlVal->val) + valDiff;
				double cvval = valToDb(newCtrlVal);
				if (cvval < valToDb(min)) cvval = min;
				if (cvval > max) cvval = max;
				//Add undo
				ctrlVal->val = cvval;
			}
		}
		else
		{
			foreach(CtrlVal* ctrlVal, m_nodes)
			{
				double cvval = ctrlVal->val + (valDiff * range);

				if (cvval< min) cvval=min;
				if (cvval>max) cvval=max;
				//Add undo
				ctrlVal->val = cvval;
			}

		}

		int endFrame = getEndFrame();
		int startFrame = getStartFrame();
		int prevFrame = 0;
		int nextFrame = INT_MAX;

		// get previous and next frame position to give x bounds for this event.
		iCtrl ic= list->begin();
		for (; ic != list->end(); ic++)
		{
			CtrlVal &cv = ic->second;
			if (cv.getFrame() == startFrame)
			{
				break;
			}
			prevFrame = cv.getFrame();
		}

		ic= list->begin();
		for (; ic != list->end(); ic++)
		{
			CtrlVal &cv = ic->second;
			nextFrame = cv.getFrame();
			if (cv.getFrame() > endFrame)
			{
				break;
			}
		}

		if (nextFrame == endFrame)
		{
			nextFrame = INT_MAX;
		}

		if ((endFrame + frameDiff) >= nextFrame)
		{
			frameDiff = 0;
		}

		if ((startFrame + frameDiff) <= prevFrame)
		{
			frameDiff = 0;
		}

		if (frameDiff != 0)
		{
			foreach(CtrlVal* ctrlVal, m_nodes)
			{
				if (ctrlVal->getFrame() != 0)
				{
					int newFrame = ctrlVal->getFrame() + frameDiff;
					removeNodeFromSelection(ctrlVal);
					addNodeToSelection(&list->setCtrlFrameValue(ctrlVal, newFrame));
				}
			}
		}

		if (lazySelectedCtrlVal)
		{
			removeNodeFromSelection(lazySelectedCtrlVal);
		}

		song->dirty = true;
	}
};



//---------------------------------------------------------
//   NPart
//---------------------------------------------------------

NPart::NPart(Part* e) : CItem(Event(), e)
{
	int th = track()->height();
	int y = track()->y();
	//printf("NPart::NPart track name:%s, y:%d h:%d\n", track()->name().toLatin1().constData(), y, th);

	setPos(QPoint(e->tick(), y));

	// NOTE: For adjustable border size: If using a two-pixel border width while drawing, use second line.
	//       If one-pixel width, use first line. Tim.
	//setBBox(QRect(e->tick(), y, e->lenTick(), th));
	setBBox(QRect(e->tick(), y + 1, e->lenTick(), th));
}

//---------------------------------------------------------
//   ComposerCanvas
//---------------------------------------------------------

ComposerCanvas::ComposerCanvas(int* r, QWidget* parent, int sx, int sy)
: Canvas(parent, sx, sy)
{
	setAcceptDrops(true);
	_raster = r;
	m_PartZIndex = true;
	build_icons = true;

	setFocusPolicy(Qt::StrongFocus);
	// Defaults:
	lineEditor = 0;
	editMode = false;
	unselectNodes = false;
	trackOffset = 0;
	show_tip = false;

	tracks = song->visibletracks();
	m_selectedCurve = 0;
	setMouseTracking(true);
	_drag = DRAG_OFF;
	curColorIndex = 0;
	automation.currentCtrlList = 0;
	automation.currentCtrlVal = 0;
	automation.controllerState = doNothing;
	automation.moveController = false;
	_curveNodeSelection = new CurveNodeSelection;
	partsChanged();
}

//---------------------------------------------------------
//   y2pitch
//---------------------------------------------------------

int ComposerCanvas::y2pitch(int y) const
{
	TrackList* tl = song->visibletracks();
	int yy = 0;
	int idx = 0;
	for (iTrack it = tl->begin(); it != tl->end(); ++it, ++idx)
	{
		int h = (*it)->height();
		if (y < yy + h)
			break;
		yy += h;
	}
	return idx;
}

//---------------------------------------------------------
//   pitch2y
//---------------------------------------------------------

int ComposerCanvas::pitch2y(int p) const
{
	TrackList* tl = song->visibletracks();
	int yy = 0;
	int idx = 0;
	for (iTrack it = tl->begin(); it != tl->end(); ++it, ++idx)
	{
		if (idx == p)
			break;
		yy += (*it)->height();
	}
	return yy;
}

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void ComposerCanvas::leaveEvent(QEvent*)
{
	emit timeChanged(MAXINT);
}

//---------------------------------------------------------
//   returnPressed
//---------------------------------------------------------

void ComposerCanvas::returnPressed()
{
	if(editMode)
	{
		lineEditor->hide();
		Part* oldPart = editPart->part();
		Part* newPart = oldPart->clone();

		newPart->setName(lineEditor->text());
		// Indicate do undo, and do port controller values but not clone parts.
		audio->msgChangePart(oldPart, newPart, true, true, false);
		song->update(SC_PART_MODIFIED);
	}
	editMode = false;
}

//---------------------------------------------------------
//   viewMouseDoubleClick
//---------------------------------------------------------

void ComposerCanvas::viewMouseDoubleClickEvent(QMouseEvent* event)
{
	if (_tool != PointerTool)
	{
		viewMousePressEvent(event);
		return;
	}
	QPoint cpos = event->pos();
    _curItem = _items.find(cpos);
	bool shift = event->modifiers() & Qt::ShiftModifier;
    if (_curItem)
	{
		if (event->button() == Qt::LeftButton && shift)
		{
		}
		else if (event->button() == Qt::LeftButton)
		{
			deselectAll();
            selectItem(_curItem, true);
            emit dclickPart(((NPart*) (_curItem))->track());
		}
	}
		//
		// double click creates new part between left and
		// right mark

	else
	{
	//This changes to song->visibletracks()
		TrackList* tl = song->visibletracks();
		iTrack it;
		int yy = 0;
		int y = event->y();
		for (it = tl->begin(); it != tl->end(); ++it)
		{
			int h = (*it)->height();
			if (y >= yy && y < (yy + h))
				break;
			yy += h;
		}
        if (_pos[2] - _pos[1] > 0 && it != tl->end())
		{
			Track* track = *it;
			switch (track->type())
			{
				case Track::MIDI:
				case Track::DRUM:
				{
					MidiPart* part = new MidiPart((MidiTrack*) track);
                    part->setTick(_pos[1]);
                    part->setLenTick(_pos[2] - _pos[1]);
					part->setName(track->name());
					NPart* np = new NPart(part);
                    _items.add(np);
					deselectAll();
					part->setSelected(true);
					audio->msgAddPart(part);
				}
					break;
				case Track::WAVE:
				case Track::AUDIO_OUTPUT:
				case Track::AUDIO_INPUT:
				case Track::AUDIO_BUSS:
				case Track::AUDIO_AUX:
				case Track::AUDIO_SOFTSYNTH:
					break;
			}
		}
	}
}

//---------------------------------------------------------
//   startUndo
//---------------------------------------------------------

void ComposerCanvas::startUndo(DragType)
{
	song->startUndo();
}

//---------------------------------------------------------
//   endUndo
//---------------------------------------------------------

void ComposerCanvas::endUndo(DragType t, int flags)
{
	song->endUndo(flags | ((t == MOVE_COPY || t == MOVE_CLONE)
			? SC_PART_INSERTED : SC_PART_MODIFIED));
}

//---------------------------------------------------------
//   moveCanvasItems
//---------------------------------------------------------

void ComposerCanvas::moveCanvasItems(CItemList& items, int dp, int dx, DragType dtype, int*)
{
	for (iCItem ici = items.begin(); ici != items.end(); ++ici)
	{
		CItem* ci = ici->second;

		int x = ci->pos().x();
		int y = ci->pos().y();
		int nx = x + dx;
		int ny = pitch2y(y2pitch(y) + dp);
		QPoint newpos = raster(QPoint(nx, ny));
		selectItem(ci, true);

		if (moveItem(ci, newpos, dtype))
			ci->move(newpos);
		if (_moving.size() == 1)
		{
			itemReleased(_curItem, newpos);
		}
		if (dtype == MOVE_COPY || dtype == MOVE_CLONE)
			selectItem(ci, false);
	}
}

//---------------------------------------------------------
//   moveItem
//    return false, if copy/move not allowed
//---------------------------------------------------------

bool ComposerCanvas::moveItem(CItem* item, const QPoint& newpos, DragType t)
{
	tracks = song->visibletracks();
	NPart* npart = (NPart*) item;
	Part* spart = npart->part();
	Track* track = npart->track();
	unsigned dtick = newpos.x();
	unsigned ntrack = y2pitch(item->mp().y());
	Track::TrackType type = track->type();
	if (tracks->index(track) == ntrack && (dtick == spart->tick()))
	{
		return false;
	}
	//printf("ComposerCanvas::moveItem ntrack: %d  tracks->size(): %d\n",ntrack,(int)tracks->size());
	Track* dtrack = 0;
	bool newdest = false;
	if (ntrack >= tracks->size())
	{
		ntrack = tracks->size();
		Track* newTrack = 0;// = song->addTrack(int(type));
		if(type == Track::WAVE)
		{
			newTrack = song->addTrackByName(spart->name(), Track::WAVE, -1, true, true);
			song->updateTrackViews();
		}
		else
		{
			VirtualTrack* vt;
			CreateTrackDialog *ctdialog = new CreateTrackDialog(&vt, type, -1, this);
			ctdialog->lockType(true);
			if(ctdialog->exec() && vt)
			{
				qint64 nid = trackManager->addTrack(vt, -1);
				newTrack = song->findTrackById(nid);
			}
		}
		if (type == Track::WAVE && newTrack)
		{
			WaveTrack* st = (WaveTrack*) track;
			WaveTrack* dt = (WaveTrack*) newTrack;
			dt->setChannels(st->channels());
		}
		if(newTrack)
		{
			newdest = true;
			dtrack = newTrack;
			//midiMonitor->msgAddMonitoredTrack(newTrack);
		}
		else
		{
			printf("ComposerCanvas::moveItem failed to create new track\n");
			return false;
		}
		
		//printf("ComposerCanvas::moveItem new track type: %d\n",newTrack->type());

	}
	//FIXME: for some reason we are getting back an invalid track on this next call
	//This is a bug caused by removing the track view update from the undo section of song I think
	//the breakage happens in commit 4484dc5
	//it looks like cloning a part is calling the endMsgCmd in song.cpp and the track is 
	//not making it into the undo system or something like that
	if(!dtrack)
		dtrack = tracks->index(ntrack);
	
	//printf("ComposerCanvas::moveItem track type is: %d\n",dtrack->type());

	if (dtrack->type() != type)
	{
		QMessageBox::critical(this, QString("OOStudio"),
				tr("Cannot copy/move/clone to different Track-Type"));
		return false;
	}
	//printf("ComposerCanvas::moveItem did not crash\n");

	Part* dpart;
	bool clone = (t == MOVE_CLONE || (t == MOVE_COPY && spart->events()->arefCount() > 1));

	if (t == MOVE_MOVE)
	{
		// This doesn't increment aref count, and doesn't chain clones.
		// It also gives the new part a new serial number, but it is
		//  overwritten with the old one by Song::changePart(), from Audio::msgChangePart() below.
		dpart = spart->clone();
		dpart->setTrack(dtrack);
	}
	else
		// This increments aref count if cloned, and chains clones.
		// It also gives the new part a new serial number.
		dpart = dtrack->newPart(spart, clone);

	if(track->name() != dtrack->name() && dpart->name().contains(track->name()))
	{
		QString name = dpart->name();
		dpart->setName(name.replace(track->name(), dtrack->name()));
	}
	dpart->setTick(dtick);
	dpart->setLeftClip(spart->leftClip());
	dpart->setRightClip(spart->rightClip());

	//printf("ComposerCanvas::moveItem before add/changePart clone:%d spart:%p events:%p refs:%d Arefs:%d sn:%d dpart:%p events:%p refs:%d Arefs:%d sn:%d\n", clone, spart, spart->events(), spart->events()->refCount(), spart->events()->arefCount(), spart->sn(), dpart, dpart->events(), dpart->events()->refCount(), dpart->events()->arefCount(), dpart->sn());

	if (t == MOVE_MOVE)
		item->setPart(dpart);
	if (t == MOVE_COPY && !clone)
	{
		//
		// Copy Events
		//
		EventList* se = spart->events();
		EventList* de = dpart->events();
		for (iEvent i = se->begin(); i != se->end(); ++i)
		{
			Event oldEvent = i->second;
			Event ev = oldEvent.clone();
			ev.setRightClip(oldEvent.rightClip());
			de->add(ev);
		}
	}
	if (t == MOVE_COPY || t == MOVE_CLONE)
	{
		// These will not increment ref count, and will not chain clones...
		if (dtrack->type() == Track::WAVE)
			audio->msgAddPart((WavePart*) dpart, false);
		else
			audio->msgAddPart(dpart, false);
	}
	else if (t == MOVE_MOVE)
	{
		dpart->setSelected(spart->selected());
		// These will increment ref count if not a clone, and will chain clones...
		if (dtrack->type() == Track::WAVE)
			// Indicate no undo, and do not do port controller values and clone parts.
			audio->msgChangePart((WavePart*) spart, (WavePart*) dpart, false, false, false);
		else
			// Indicate no undo, and do port controller values but not clone parts.
			audio->msgChangePart(spart, dpart, false, true, false);

		spart->setSelected(false);
	}
	//printf("ComposerCanvas::moveItem after add/changePart spart:%p events:%p refs:%d Arefs:%d dpart:%p events:%p refs:%d Arefs:%d\n", spart, spart->events(), spart->events()->refCount(), spart->events()->arefCount(), dpart, dpart->events(), dpart->events()->refCount(), dpart->events()->arefCount());

	if (song->len() < (dpart->lenTick() + dpart->tick()))
		song->setLen(dpart->lenTick() + dpart->tick());
	//endUndo(t);
	if(newdest || track->parts()->empty())
		song->updateTrackViews();
		//emit tracklistChanged();
	return true;
}

//---------------------------------------------------------
//   raster
//---------------------------------------------------------

QPoint ComposerCanvas::raster(const QPoint& p) const
{
	int y = pitch2y(y2pitch(p.y()));
	int x = p.x();
	if (x < 0)
		x = 0;
	x = AL::sigmap.raster(x, *_raster);
	if (x < 0)
		x = 0;
	return QPoint(x, y);
}

//---------------------------------------------------------
//   partsChanged
//---------------------------------------------------------

void ComposerCanvas::partsChanged()
{
	tracks = song->visibletracks();
	_items.clear();
	int idx = 0;
	for (iTrack t = tracks->begin(); t != tracks->end(); ++t)
	{
		PartList* pl = (*t)->parts();
		for (iPart i = pl->begin(); i != pl->end(); ++i)
		{
			NPart* np = new NPart(i->second);
			_items.add(np);
			if (i->second->selected())
			{
				selectItem(np, true);
			}
		}
		++idx;
	}
	redraw();
}

//---------------------------------------------------------
//   updateSelection
//---------------------------------------------------------

void ComposerCanvas::updateSelection()
{
    for (iCItem i = _items.begin(); i != _items.end(); ++i)
	{
		NPart* part = (NPart*) (i->second);
		part->part()->setSelected(i->second->isSelected());
	}
	emit selectionChanged();
	redraw();
}

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

void ComposerCanvas::resizeItem(CItem* i, bool noSnap)/*{{{*/
{
	Track* t = ((NPart*) (i))->track();
	Part* p = ((NPart*) (i))->part();

	int pos = p->tick() + i->width();
	int snappedpos = AL::sigmap.raster(pos, *_raster);
	if (noSnap)
	{
		snappedpos = p->tick();
	}
	unsigned int newwidth = snappedpos - p->tick();
	if (newwidth == 0)
		newwidth = AL::sigmap.rasterStep(p->tick(), *_raster);

	song->cmdResizePart(t, p, newwidth);
}/*}}}*/

void ComposerCanvas::resizeItemLeft(CItem* i, QPoint pos, bool noSnap)/*{{{*/
{
	Track* t = ((NPart*) (i))->track();
	Part* p = ((NPart*) (i))->part();

	int endtick = (p->tick() + p->lenTick());
	int snappedpos = AL::sigmap.raster(i->x(), *_raster);
	//printf("ComposerCanvas::resizeItemLeft snap pos:%d , nosnap pos: %d\n", snappedpos, i->x());
	if (noSnap)
	{
		snappedpos = i->x();
	}
	song->cmdResizePartLeft(t, p, snappedpos, endtick, pos);
	//redraw();
}/*}}}*/

CItem* ComposerCanvas::addPartAtCursor(Track* track)
{
	if (!track)
		return 0;

	unsigned pos = song->cpos();
	Part* pa = 0;
	NPart* np = 0;
	switch (track->type())
	{
		case Track::MIDI:
		case Track::DRUM:
			pa = new MidiPart((MidiTrack*) track);
			pa->setTick(pos);
			pa->setLenTick(8000);
			break;
		case Track::WAVE:
			//pa = new WavePart((WaveTrack*) track);
			//pa->setTick(pos);
			//pa->setLenTick(0);
			//break;
		case Track::AUDIO_OUTPUT:
		case Track::AUDIO_INPUT:
		case Track::AUDIO_BUSS:
		case Track::AUDIO_AUX:
		case Track::AUDIO_SOFTSYNTH:
			return 0;
	}
	pa->setName(track->name());
	pa->setColorIndex(track->getDefaultPartColor());
	np = new NPart(pa);
	return np;
}

//---------------------------------------------------------
//   newItem
//    first create local Item
//---------------------------------------------------------

CItem* ComposerCanvas::newItem(const QPoint& pos, int)
{
	tracks = song->visibletracks();
	int x = pos.x();
	if (x < 0)
		x = 0;
	x = AL::sigmap.raster(x, *_raster);
	unsigned trackIndex = y2pitch(pos.y());
	if (trackIndex >= tracks->size())
		return 0;
	Track* track = tracks->index(trackIndex);
	if (!track)
		return 0;

	Part* pa = 0;
	NPart* np = 0;
	switch (track->type())
	{
		case Track::MIDI:
		case Track::DRUM:
			pa = new MidiPart((MidiTrack*) track);
			pa->setTick(x);
			pa->setLenTick(0);
			break;
		case Track::WAVE:
			//pa = new WavePart((WaveTrack*) track);
			//pa->setTick(x);
			//pa->setLenTick(0);
			//break;
		case Track::AUDIO_OUTPUT:
		case Track::AUDIO_INPUT:
		case Track::AUDIO_BUSS:
		case Track::AUDIO_AUX:
		case Track::AUDIO_SOFTSYNTH:
			return 0;
	}
	pa->setName(track->name());
	pa->setColorIndex(track->getDefaultPartColor());
	np = new NPart(pa);
	return np;
}

//---------------------------------------------------------
//   newItem
//---------------------------------------------------------

void ComposerCanvas::newItem(CItem* i, bool noSnap)
{
	Part* p = ((NPart*) (i))->part();

	int len = i->width();
	if (!noSnap)
		len = AL::sigmap.raster(len, *_raster);
	if (len == 0)
		len = AL::sigmap.rasterStep(p->tick(), *_raster);
	p->setLenTick(len);
	p->setSelected(true);
	audio->msgAddPart(p);
}

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool ComposerCanvas::deleteItem(CItem* i)
{
	Part* p = ((NPart*) (i))->part();
	audio->msgRemovePart(p); //Invokes songChanged which calls partsChanged which makes it difficult to delete them there
	return true;
}

//---------------------------------------------------------
//   splitItem
//---------------------------------------------------------

void ComposerCanvas::splitItem(CItem* item, const QPoint& pt)
{
	NPart* np = (NPart*) item;
	Track* t = np->track();
	Part* p = np->part();
	int x = pt.x();
	if (x < 0)
		x = 0;
	song->cmdSplitPart(t, p, AL::sigmap.raster(x, *_raster));
}

//---------------------------------------------------------
//   glueItem
//---------------------------------------------------------

void ComposerCanvas::glueItem(CItem* item)
{
	NPart* np = (NPart*) item;
	Track* t = np->track();
	Part* p = np->part();
	song->cmdGluePart(t, p);
}

//---------------------------------------------------------
//   genItemPopup
//---------------------------------------------------------

QMenu* ComposerCanvas::genItemPopup(CItem* item)/*{{{*/
{
	NPart* npart = (NPart*) item;
	Track::TrackType trackType = npart->track()->type();

	QMenu* partPopup = new QMenu(this);
	QMenu* colorPopup = partPopup->addMenu(tr("Part Color"));

	QMenu* colorSub; 
	for (int i = 0; i < NUM_PARTCOLORS; ++i)
	{
		QString colorname(config.partColorNames[i]);
		if(colorname.contains("menu:", Qt::CaseSensitive))
		{
			colorSub = colorPopup->addMenu(colorname.replace("menu:", ""));
		}
		else
		{
			if(npart->part()->colorIndex() == i)
			{
				colorname = QString(config.partColorNames[i]);
				colorPopup->setIcon(partColorIconsSelected.at(i));
				colorPopup->setTitle(colorSub->title()+": "+colorname);

				colorname = QString("* "+config.partColorNames[i]);
				QAction *act_color = colorSub->addAction(partColorIconsSelected.at(i), colorname);
				act_color->setData(20 + i);
			}
			else
			{
				colorname = QString("     "+config.partColorNames[i]);
				QAction *act_color = colorSub->addAction(partColorIcons.at(i), colorname);
				act_color->setData(20 + i);
			}
		}	
	}
	QString zvalue = QString::number(item->zValue(true));
	QMenu* layerMenu = partPopup->addMenu(tr("Part Layers: ")+zvalue);
	QAction *act_front = layerMenu->addAction(tr("Top"));
	act_front->setData(4003);
	QAction *act_up = layerMenu->addAction(tr("Up"));
	act_up->setData(4002);
	QAction *act_down = layerMenu->addAction(tr("Down"));
	act_down->setData(4001);
	QAction *act_back = layerMenu->addAction(tr("Bottom"));
	act_back->setData(4000);

	QAction *act_cut = partPopup->addAction(*editcutIconSet, tr("C&ut"));
	act_cut->setData(4);
	act_cut->setShortcut(Qt::CTRL + Qt::Key_X);

	QAction *act_copy = partPopup->addAction(*editcopyIconSet, tr("&Copy"));
	act_copy->setData(5);
	act_copy->setShortcut(Qt::CTRL + Qt::Key_C);

	partPopup->addSeparator();
	int rc = npart->part()->events()->arefCount();
	QString st = QString(tr("S&elect "));
	if (rc > 1)
		st += (QString().setNum(rc) + QString(" "));
	st += QString(tr("clones"));
	QAction *act_select = partPopup->addAction(st);
	act_select->setData(18);

	partPopup->addSeparator();
	
	QAction *act_rename = partPopup->addAction(tr("Rename"));
	act_rename->setData(0);



	QAction *act_delete = partPopup->addAction(QIcon(*deleteIcon), tr("Delete")); // ddskrjo added QIcon to all
	act_delete->setData(1);
	QAction *act_split = partPopup->addAction(QIcon(*cutIcon), tr("Split"));
	act_split->setData(2);
	QAction *act_glue = partPopup->addAction(QIcon(*glueIcon), tr("Glue"));
	act_glue->setData(3);
	QAction *act_declone = partPopup->addAction(tr("De-clone"));
	act_declone->setData(15);

	partPopup->addSeparator();
	switch (trackType)
	{
		case Track::MIDI:
		{
			QAction *act_performer = partPopup->addAction(QIcon(*pianoIconSet), tr("Performer"));
			act_performer->setData(10);
			QAction *act_mlist = partPopup->addAction(QIcon(*edit_listIcon), tr("List"));
			act_mlist->setData(12);
			QAction *act_mexport = partPopup->addAction(tr("Export"));
			act_mexport->setData(16);
		}
			break;
		case Track::DRUM:
		{
			QAction *act_dlist = partPopup->addAction(QIcon(*edit_listIcon), tr("List"));
			act_dlist->setData(12);
			QAction *act_drums = partPopup->addAction(QIcon(*edit_drummsIcon), tr("Drums"));
			act_drums->setData(13);
			QAction *act_dexport = partPopup->addAction(tr("Export"));
			act_dexport->setData(16);
		}
			break;
		case Track::WAVE:
		{
// the wave editor is destructive, don't us it anymore!
//			QAction *act_wedit = partPopup->addAction(QIcon(*edit_waveIcon), tr("wave edit"));
//			act_wedit->setData(14);
			QAction *act_wexport = partPopup->addAction(tr("Export"));
			act_wexport->setData(16);
			QAction *act_wfinfo = partPopup->addAction(tr("File info"));
			act_wfinfo->setData(17);
		}
			break;
		case Track::AUDIO_OUTPUT:
		case Track::AUDIO_INPUT:
		case Track::AUDIO_BUSS:
		case Track::AUDIO_AUX:
		case Track::AUDIO_SOFTSYNTH:
			break;
	}

	act_select->setEnabled(rc > 1);
	act_delete->setEnabled(true);
	act_cut->setEnabled(true);
	act_declone->setEnabled(rc > 1);

	return partPopup;
}/*}}}*/

//---------------------------------------------------------
//   itemPopup
//---------------------------------------------------------

void ComposerCanvas::itemPopup(CItem* item, int n, const QPoint& pt)/*{{{*/
{
	PartList* pl = new PartList;
	NPart* npart = (NPart*) (item);
	pl->add(npart->part());
	int zvalue = item->zValue(true);
	switch (n)
	{
		case 0: // rename
		{
			editPart = npart;
            QRect r = map(_curItem->bbox());
			if (lineEditor == 0)
			{
				lineEditor = new QLineEdit(this);
				lineEditor->setFrame(true);
			}
			lineEditor->setText(editPart->name());
			lineEditor->setFocus();
			lineEditor->show();
			lineEditor->setGeometry(r);
			editMode = true;
		}
			break;
		case 1: // delete
			deleteItem(item);
			break;
		case 2: // split
			splitItem(item, pt);
			break;
		case 3: // glue
			glueItem(item);
			break;
		case 4:
			copy(pl);
			audio->msgRemovePart(npart->part());
			break;
		case 5:
			copy(pl);
			break;
		case 10: // performer edit
			emit startEditor(pl, 0);
			return;
		case 12: // list edit
			emit startEditor(pl, 1);
			return;
		case 13: // drum edit
			emit startEditor(pl, 3);
			return;
		case 14: // wave edit
		{
			emit startEditor(pl, 4);
		}
			return;
		case 15: // declone
		{
			Part* spart = npart->part();
			Track* track = npart->track();
			Part* dpart = track->newPart(spart, false);
			//printf("ComposerCanvas::itemPopup: #1 spart %s %p next:%s %p prev:%s %p\n", spart->name().toLatin1().constData(), spart, spart->nextClone()->name().toLatin1().constData(), spart->nextClone(), spart->prevClone()->name().toLatin1().constData(), spart->prevClone());
			//printf("ComposerCanvas::itemPopup: #1 dpart %s %p next:%s %p prev:%s %p\n", dpart->name().toLatin1().constData(), dpart, dpart->nextClone()->name().toLatin1().constData(), dpart->nextClone(), dpart->prevClone()->name().toLatin1().constData(), dpart->prevClone());

			EventList* se = spart->events();
			EventList* de = dpart->events();
			for (iEvent i = se->begin(); i != se->end(); ++i)
			{
				Event oldEvent = i->second;
				Event ev = oldEvent.clone();
				de->add(ev);
			}
			song->startUndo();
			// Indicate no undo, and do port controller values but not clone parts.
			audio->msgChangePart(spart, dpart, false, true, false);
			//printf("ComposerCanvas::itemPopup: #2 spart %s %p next:%s %p prev:%s %p\n", spart->name().toLatin1().constData(), spart, spart->nextClone()->name().toLatin1().constData(), spart->nextClone(), spart->prevClone()->name().toLatin1().constData(), spart->prevClone());
			//printf("ComposerCanvas::itemPopup: #2 dpart %s %p next:%s %p prev:%s %p\n", dpart->name().toLatin1().constData(), dpart, dpart->nextClone()->name().toLatin1().constData(), dpart->nextClone(), dpart->prevClone()->name().toLatin1().constData(), dpart->prevClone());

			song->endUndo(SC_PART_MODIFIED);
			break; // Has to be break here, right?
		}
		case 16: // Export to file
		{
			const Part* part = item->part();
			bool popenFlag = false;
			//QString fn = getSaveFileName(QString(""), part_file_pattern, this, tr("OOStudio: save part"));
			QString fn = getSaveFileName(QString(""), part_file_save_pattern, this, tr("OOStudio: save part"));
			if (!fn.isEmpty())
			{
				FILE* fp = fileOpen(this, fn, ".mpt", "w", popenFlag, false, false);
				if (fp)
				{
					Xml tmpXml = Xml(fp);
					//part->write(0, tmpXml);
					// Write the part. Indicate that it's a copy operation - to add special markers,
					//  and force full wave paths.
					part->write(0, tmpXml, true, true);
					fclose(fp);
				}
			}
			break;
		}

		case 17: // File info
		{
			Part* p = item->part();
			EventList* el = p->events();
			QString str = tr("Part name") + ": " + p->name() + "\n" + tr("Files") + ":";
			for (iEvent e = el->begin(); e != el->end(); ++e)
			{
				Event event = e->second;
				SndFileR f = event.sndFile();
				if (f.isNull())
					continue;
				//str.append("\n" + f.path());
				str.append(QString("\n@") + QString().setNum(event.tick()) + QString(" len:") +
						QString().setNum(event.lenTick()) + QString(" ") + f.path());
			}
			QMessageBox::information(this, "File info", str, "Ok", 0);
			break;
		}
		case 18: // Select clones
		{
			Part* part = item->part();

			// Traverse and process the clone chain ring until we arrive at the same part again.
			// The loop is a safety net.
			Part* p = part;
			int j = part->cevents()->arefCount();
			if (j > 0)
			{
				for (int i = 0; i < j; ++i)
				{
					//printf("ComposerCanvas::itemPopup i:%d %s %p events %p refs:%d arefs:%d\n", i, p->name().toLatin1().constData(), p, part->cevents(), part->cevents()->refCount(), j);

					p->setSelected(true);
					p = p->nextClone();
					if (p == part)
						break;
				}
				//song->update();
				song->update(SC_SELECTION);
			}

			break;
		}
		case 20 ... NUM_PARTCOLORS + 20:
		{
			curColorIndex = n - 20;
			bool single = true;
			//Loop through all parts and set color on selected:
			CItemList list = getSelectedItems();
			if(list.size() && list.size() > 1)
			{
				single = false;
            	for (iCItem i = list.begin(); i != list.end(); i++)
				{
					i->second->part()->setColorIndex(curColorIndex);
				}
			}

			// If no items selected, use the one clicked on.
			if (single)
				item->part()->setColorIndex(curColorIndex);

			song->update(SC_PART_COLOR_MODIFIED);
			redraw();
			break;
		}
		case 4000: //Move to zero
		{
			item->setZValue(0, true);
			break;
		}
		case 4001: //down one layer
		{
			if(item->zValue(true))
				item->setZValue(zvalue-1, true);
			break;
		}
		case 4002: //up one layer
		{
			item->setZValue(zvalue+1, true);
			break;
		}
		case 4003: // move to top layer
		{
			zvalue = item->part()->track()->maxZIndex();
			
			item->setZValue(zvalue+1, true);
			break;
		}
		default:
			printf("unknown action %d\n", n);
			break;
	}
	delete pl;
}/*}}}*/

CItemList ComposerCanvas::getSelectedItems()
{
	CItemList list;
    for (iCItem i = _items.begin(); i != _items.end(); i++)/*{{{*/
	{
		if (i->second->isSelected())
		{
			list.add(i->second);
		}
	}/*}}}*/
	return list;
}

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void ComposerCanvas::mousePress(QMouseEvent* event)/*{{{*/
{
//	qDebug("ComposerCanvas::mousePress: One pixel = %d ticks", AL::sigmap.raster(1, *_raster));
//	qDebug("ComposerCanvas::mousePress: width = %d ticks, length: %d", AL::sigmap.raster(width(), *_raster), song->len());
	if (event->modifiers() & Qt::ShiftModifier && _tool != AutomationTool)
	{
		return;
	}

	QPoint pt = event->pos();
	CItem* item = _items.find(pt);

	if (item == 0 && _tool!=AutomationTool)
	{
		return;
	}

	switch (_tool)
	{
		default:
			emit trackChanged(item->part()->track());
			break;
		case PointerTool:
		{ //Find out if we are clicking on a fade controll node and override drag move
			emit trackChanged(item->part()->track());
			if(_drag == DRAG_MOVE_START)
			{
				int boxSize = 14;
				int currX =  mapx(pt.x());
				int currY =  mapy(pt.y());
				NPart* np = (NPart*) item;
				Part* p = np->part();
				Track* track = p->track();
				if(track && track->type() == Track::WAVE)
				{
					//int x = pt.x();
					//int y = pt.y();
					int tracktop = track2Y(track)-ypos;
					WavePart* part = (WavePart*)p;
					long pstart = part->frame();
					long pend = pstart+part->lenFrame();
					//long currFrame = tempomap.tick2frame(pt.x());
					QPoint click(currX, currY);
					FadeCurve* fadeIn = part->fadeIn();
					FadeCurve* fadeOut = part->fadeOut();
					if(fadeIn)
					{
						long fwidth = fadeIn->width();
						long fiPos = pstart+fwidth;
						fiPos = mapx(tempomap.frame2tick(fiPos));
						QRect fiRect(fiPos-(boxSize/2), tracktop, boxSize, boxSize);
						if(fiRect.contains(click))
						{
							m_selectedCurve = fadeIn;
							_drag = DRAG_OFF;
							return; //dont process fadeOut
						}
					}
					if(fadeOut)
					{
						long fwidth = fadeOut->width();
						long foPos = pend-fwidth;
						foPos = mapx(tempomap.frame2tick(foPos));
						QRect foRect(foPos-(boxSize/2), tracktop, boxSize, boxSize);
						//qDebug() << "Rect: " << foRect;
						//qDebug() << "click pos:" << click;
						if(foRect.contains(click))
						{
							m_selectedCurve = fadeOut;
							_drag = DRAG_OFF;
							return; //dont process fadeOut
						}
					}
				}
			}
		}
		break;
		case CutTool:
			splitItem(item, pt);
			break;
		case GlueTool:
			glueItem(item);
			break;
		case MuteTool:
		{
			NPart* np = (NPart*) item;
			Part* p = np->part();
			p->setMute(!p->mute());
			song->update(SC_MUTE);
			redraw();
			break;
		}
		case AutomationTool:
		{
			automation.mousePressPos = event->pos();

			if (event->modifiers() & Qt::ShiftModifier && event->button() & Qt::LeftButton && !automation.currentCtrlVal)
			{
				automation.moveController = true;
				return;
			}
			if (event->modifiers() & Qt::ControlModifier && event->button() & Qt::LeftButton)
			{
				addNewAutomation(event);
				return;
			}

			if (automation.controllerState != doNothing)
			{
				automation.moveController=true;
				if (automation.currentCtrlVal)
				{
					QWidget::setCursor(Qt::BlankCursor);
					//printf("Current After foundIt Controller: %s\n", automation.currentCtrlList->name().toLatin1().constData());
					if(automation.currentCtrlList)
					{
						automation.currentCtrlList->setSelected(true);
					}
					Track * t = y2Track(event->pos().y());
                    if (t && t->isMidiTrack())
                        t = ((MidiTrack*)t)->getAutomationTrack();
					if (t) {
						CtrlListList* cll = ((AudioTrack*) t)->controller();
						for(CtrlListList::iterator icll = cll->begin(); icll != cll->end(); ++icll)
						{
							//iCtrlList *icl = icll->second;
							CtrlList *cl = icll->second;
							if(cl != automation.currentCtrlList)
							{
								cl->setSelected(false);
							}
						}
					}	
				}	
			}
			else
			{
				Track * t = y2Track(event->pos().y());
                if (t && t->isMidiTrack())
                    t = ((MidiTrack*)t)->getAutomationTrack();
				if(t)
					selectAutomation(t, event->pos());
				if(automation.currentCtrlList && automation.controllerState == doNothing)
				{
					//qDebug("Automation curve selected enable LASSO mode");
					_drag = DRAG_LASSO_START;
				}
			}

			if (automation.currentCtrlVal && event->modifiers() & Qt::ShiftModifier)
			{
				CtrlVal* cv = automation.currentCtrlVal;
				if (_curveNodeSelection->isSelected(cv))
				{
					_curveNodeSelection->removeNodeFromSelection(cv);
					automation.currentCtrlVal = 0;
					redraw();
				}
				else
				{
					_curveNodeSelection->addNodeToSelection(cv);
				}
			}
			else
			{
				if (! (event->modifiers() & Qt::ShiftModifier))
				{
					_curveNodeSelection->clearSelection();
					redraw();
				}
			}

			break;
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void ComposerCanvas::mouseRelease(const QPoint& pos)/*{{{*/
{
	if(_drag == DRAG_LASSO && _tool == AutomationTool)
	{
		//qDebug("ComposerCanvas::mouseRelease lasso mode");
		_drag = DRAG_OFF;
		if(automation.currentCtrlList)
		{
			//qDebug("ComposerCanvas::mouseRelease found selected curve");
			double top, bottom, min, max;
			automation.currentCtrlList->range(&min,&max);
			double range = max - min;
			//int left = tempomap.tick2frame(_lasso.left());
			//int right = tempomap.tick2frame(_lasso.right());
			int left = tempomap.tick2frame(_start.x());
			int right = tempomap.tick2frame(pos.x());
			int startY = _start.y();
			int endY = pos.y();
			int tempX, tempY;
			
			if(left > right)
			{
				tempX = left;
				left = right;
				right = tempX;
			}
			if(endY < startY)
			{
				tempY = startY;
				startY = endY;
				endY = tempY;
			}

			if(startY > automation.currentTrack->y())
				startY = automation.currentTrack->y();
			if(endY > (automation.currentTrack->y() + automation.currentTrack->height()))
				endY = (automation.currentTrack->y() + automation.currentTrack->height());
			
			top = double(startY - track2Y(automation.currentTrack)) / automation.currentTrack->height();
			bottom = double(endY - track2Y(automation.currentTrack)) / automation.currentTrack->height();
			
			if(top > max)
				top = max;
			if(bottom < min)
				bottom = min;
			//qDebug("ComposerCanvas::mouseRelease top: %f, bottom: %f, start: %d, end: %d\n", top, bottom, left, right);
			//double relativeY = double(event->pos().y() - track2Y(automation.currentTrack)) / automation.currentTrack->height();

			if (automation.currentCtrlList->id() == AC_VOLUME )
			{
				top = dbToVal(max) - top;
				top = valToDb(top);
				bottom = dbToVal(max) - bottom;
				bottom = valToDb(bottom);
			}
			else
			{
				top = max - (top * range);
				bottom = max - (bottom * range);
			}
			//qDebug("ComposerCanvas::mouseRelease top: %f, bottom: %f, start: %d, end: %d\n", top, bottom, left, right);
			_curveNodeSelection->clearSelection();
			iCtrl ic = automation.currentCtrlList->begin();
			for (; ic !=automation.currentCtrlList->end(); ic++)
			{
				int frame = ic->second.getFrame();
				double val = ic->second.val;
				if(frame >= left && frame <= right && val >= bottom && val <= top)
				{
					CtrlVal &cv = ic->second;
					automation.currentCtrlVal = &cv;
					automation.controllerState = movingController;
					//qDebug("ComposerCanvas::mouseRelease: Adding node at frame: %d, value: %f\n", cv.getFrame(),  cv.val);
					_curveNodeSelection->addNodeToSelection(automation.currentCtrlVal);
				}
			}
		}
		_lasso.setRect(-1, -1, -1, -1);
		redraw();
	}
	else
	{//process the lasso and select at node in range
		if(automation.movingStarted)
		{//m_automationMoveList
			if (automation.currentCtrlList)
			{
				QList<CtrlVal> valuesToAdd;

				QList<CtrlVal*> selectedNodes = _curveNodeSelection->getNodes();
				foreach(CtrlVal* val, selectedNodes)
				{
					valuesToAdd.append(CtrlVal(val->getFrame(), val->val));
				}
				bool singleSelect = false;
				if(valuesToAdd.isEmpty() && automation.currentCtrlVal)
				{//its a single node move
					singleSelect = true;
					valuesToAdd.append(CtrlVal(automation.currentCtrlVal->getFrame(), automation.currentCtrlVal->val));
				}
				//Delete nodes from controller
				QList<int> delList;
				iCtrl ictl = automation.currentCtrlList->begin();
				for (; ictl != automation.currentCtrlList->end(); ictl++)
				{
					int frame = ictl->second.getFrame();
					foreach(CtrlVal val, valuesToAdd)
					{
						if(frame == val.getFrame())
						{
							delList.append(val.getFrame());
							break;
						}
					}
				}
				//Clear the node selection list since it will contain invalid pointers after below
				_curveNodeSelection->clearSelection();
				//Delete the new moved nodes
				foreach(int frame, delList)
				{
					automation.currentCtrlList->del(frame);
				}
				//Readd the old node positions 
				foreach(CtrlVal v, m_automationMoveList)
				{
					automation.currentCtrlList->add(v.getFrame(), v.val);
				}
				//Let undo/redo do the job
				AddRemoveCtrlValues* modifyCommand = new AddRemoveCtrlValues(automation.currentCtrlList, m_automationMoveList, valuesToAdd, OOMCommand::MODIFY);

				CommandGroup* group = new CommandGroup(tr("Move Automation Nodes"));
				group->add_command(modifyCommand);

				song->pushToHistoryStack(group);

				//Repopulate selection list
				if(!singleSelect)
				{
					iCtrl ic = automation.currentCtrlList->begin();
					for (; ic != automation.currentCtrlList->end(); ic++)
					{
						int frame = ic->second.getFrame();
						double value = ic->second.val;
						foreach(CtrlVal val, valuesToAdd)
						{
							if(frame == val.getFrame() && value == val.val)
							{
								CtrlVal &cv = ic->second;
								automation.currentCtrlVal = &cv;
								automation.controllerState = movingController;
								//qDebug("ComposerCanvas::mouseRelease: Adding node at frame: %d, value: %f\n", cv.getFrame(),  cv.val);
								_curveNodeSelection->addNodeToSelection(automation.currentCtrlVal);
								break;
							}
						}
					}
				}
			}
		}
		// clear all the automation parameters
		automation.moveController = false;
		automation.controllerState = doNothing;
		automation.currentCtrlVal = 0;
		automation.movingStarted = false;
		m_automationMoveList.clear();
		m_selectedCurve = 0;
		_drag = DRAG_OFF;
	}
}/*}}}*/

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void ComposerCanvas::mouseMove(QMouseEvent* event)/*{{{*/
{
	int x = event->pos().x();
	int y = event->pos().y();
	if (x < 0)
		x = 0;

	emit timeChanged(AL::sigmap.raster(x, *_raster));

	if(_tool == PointerTool && m_selectedCurve)
	{
		long currFrame = tempomap.tick2frame(event->pos().x());
		WavePart *part = m_selectedCurve->part();
		if(part)
		{
			long pstart = part->frame();
			long pend = pstart+part->lenFrame();
			switch(m_selectedCurve->type())
			{
				case FadeCurve::FadeIn:
				{
					if(currFrame <= pstart)
					{
						m_selectedCurve->setWidth(0);
					}
					else if(currFrame >= pend)
					{
						m_selectedCurve->setWidth(part->lenFrame());
					}
					else
					{
						m_selectedCurve->setWidth(currFrame-pstart);
					}
					//printf("FadeIn Width: %ld\n", long(m_selectedCurve->width()));
				}
				break;
				case FadeCurve::FadeOut:
				{
					if(currFrame <= pstart)
					{
						m_selectedCurve->setWidth(part->lenFrame());
					}
					else if(currFrame >= pend)
					{
						m_selectedCurve->setWidth(0);
					}
					else
					{
						m_selectedCurve->setWidth(pend-currFrame);
					}
				}
				break;
			}
			redraw();
			return;
		}
	}
	if(_drag != DRAG_LASSO)
	{
		processAutomationMovements(event);

		if (show_tip && _tool == AutomationTool && automation.currentCtrlList && !automation.moveController) 
		{
			Track* t = y2Track(y);
			if(t)
			{
				CtrlListList *cll;
				CtrlListList *inputCll;
                if (t->isMidiTrack())
                {
                    AudioTrack* atrack = t->wantsAutomation() ? ((MidiTrack*)t)->getAutomationTrack() : 0;
                    if (!atrack)
                        return;
                    cll = atrack->controller();
					Track *input = t->inputTrack();
					if(input)
					{
						inputCll = ((AudioTrack*)input)->controller();
					}
                }
                else
                    cll = ((AudioTrack*)t)->controller();
                //TODO: process input controllers for midi tracks
				for(CtrlListList::iterator ic = cll->begin(); ic != cll->end(); ++ic)
				{
					CtrlList* cl = ic->second;
					if(cl->selected())
					{
						QString dbString;
						double min, max;
						cl->range(&min,&max);
						double range = max - min;
						double relativeY = double(y - track2Y(t)) / t->height();
						double newValue;

						if(cl && cl->id() == AC_VOLUME)
						{
							newValue = dbToVal(max) - relativeY;
							newValue = valToDb(newValue);
							if (newValue < 0.0001f)
							{
								newValue = 0.0001f;
							}
							newValue = 20.0f * log10 (newValue);
							if(newValue < -60.0f)
								newValue = -60.0f;
							 dbString += QString::number (newValue, 'f', 2) + " dB";
						}
						else
						{
							newValue = max - (relativeY * range);
							dbString += QString::number(newValue, 'f', 2);
						}
                        if(cl->unit().isEmpty() == false)
                            dbString.append(" "+cl->unit());
						if(cl->pluginName().isEmpty())
							dbString.append("  "+cl->name());
						else
							dbString.append("  "+cl->name()).append(" : ").append(cl->pluginName());
						QPoint cursorPos = QCursor::pos();
						QToolTip::showText(cursorPos, dbString, this, QRect(cursorPos.x(), cursorPos.y(), 2, 2));
						break;
					}
				}
			}
		}//
	}
}/*}}}*/

//---------------------------------------------------------
//   y2Track
//---------------------------------------------------------

Track* ComposerCanvas::y2Track(int y) const
{
	//This changes to song->visibletracks()
	TrackList* l = song->visibletracks();
	int ty = 0;
	for (iTrack it = l->begin(); it != l->end(); ++it)
	{
		int h = (*it)->height();
		if (y >= ty && y < ty + h)
			return *it;
		ty += h;
	}
	return 0;
}


// Return the track Y position, if track was not found, return -1
int ComposerCanvas::track2Y(Track * track) const
{
	//This changes to song->visibletracks()
	TrackList* l = song->visibletracks();
	int trackYPos = -1;

	for (iTrack it = l->begin(); it != l->end(); ++it)
	{
		if ((*it) == track)
		{
			return trackYPos;
		}

		int h = (*it)->height();
		trackYPos += h;
	}

	return trackYPos;
}


//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------

void ComposerCanvas::keyPress(QKeyEvent* event)/*{{{*/
{
	int key = event->key();
	if (editMode)
	{
		if (key == Qt::Key_Return || key == Qt::Key_Enter)
		{
			//qDebug("ComposerCanvas::keyPress Qt::Key_Return pressed");
			returnPressed();
			return;
		}
		else if (key == Qt::Key_Escape)
		{
			lineEditor->hide();
			editMode = false;
			return;
		}
	}
	//qDebug("ComposerCanvas::keyPress Not editing");

	if (event->modifiers() & Qt::ShiftModifier)
		key += Qt::SHIFT;
	if (event->modifiers() & Qt::AltModifier)
		key += Qt::ALT;
	if (event->modifiers() & Qt::ControlModifier)
		key += Qt::CTRL;
	if (((QInputEvent*) event)->modifiers() & Qt::MetaModifier)
		key += Qt::META;


	if (key == shortcuts[SHRT_DELETE].key)
	{
		if (getCurrentDrag())
		{
			//printf("dragging!!\n");
			return;
		}

		if (_tool == AutomationTool)
		{
			return;
		}

		song->startUndo();
		audio->msgRemoveParts(song->selectedParts());
		song->endUndo(SC_PART_REMOVED);

		return;
	}
	else if (key == shortcuts[SHRT_POS_DEC].key)
	{
        int spos = _pos[0];
		if (spos > 0)
		{
			spos -= 1; // Nudge by -1, then snap down with raster1.
			spos = AL::sigmap.raster1(spos, *_raster);
		}
		if (spos < 0)
			spos = 0;
		Pos p(spos, true);
		song->setPos(0, p, true, true, true);
		return;
	}
	else if (key == shortcuts[SHRT_POS_INC].key)
	{
		int spos = AL::sigmap.raster2(_pos[0] + 1, *_raster); // Nudge by +1, then snap up with raster2.
		Pos p(spos, true);
		song->setPos(0, p, true, true, true);
		return;
	}
	else if (key == shortcuts[SHRT_POS_DEC_NOSNAP].key)
	{
		int spos = _pos[0] - AL::sigmap.rasterStep(_pos[0], *_raster);
		if (spos < 0)
			spos = 0;
		Pos p(spos, true);
		song->setPos(0, p, true, true, true);
		return;
	}
	else if (key == shortcuts[SHRT_POS_INC_NOSNAP].key)
	{
                Pos p(_pos[0] + AL::sigmap.rasterStep(_pos[0], *_raster), true);
		song->setPos(0, p, true, true, true);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_POINTER].key)
	{
		emit setUsedTool(PointerTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_PENCIL].key)
	{
		emit setUsedTool(PencilTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_LINEDRAW].key)
	{
		emit setUsedTool(AutomationTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_RUBBER].key)
	{
		emit setUsedTool(RubberTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_SCISSORS].key)
	{
		emit setUsedTool(CutTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_GLUE].key)
	{
		emit setUsedTool(GlueTool);
		return;
	}
	else if (key == shortcuts[SHRT_TOOL_MUTE].key)
	{
		emit setUsedTool(MuteTool);
		return;
	}
	else if (key == shortcuts[SHRT_SEL_TRACK_ABOVE].key)
	{
		emit selectTrackAbove();
		return;
	}
	else if (key == shortcuts[SHRT_SEL_TRACK_BELOW].key)
	{
		emit selectTrackBelow();
		return;
	}
	else if (key == shortcuts[SHRT_SEL_TRACK_ABOVE_ADD].key)
	{
		TrackList* tl = song->visibletracks();
		TrackList selectedTracks = song->getSelectedTracks();
		if (!selectedTracks.size())
		{
			return;
		}

		iTrack t = tl->end();
		while (t != tl->begin())
		{
			--t;

			if ((*t) == *(selectedTracks.begin()))
			{
				if (*t != *(tl->begin()))
				{
					Track* previous = *(--t);
					previous->setSelected(true);
					song->update(SC_SELECTION);
					return;
				}

			}
		}

		return;
	}
	else if (key == shortcuts[SHRT_SEL_TRACK_BELOW_ADD].key)
	{
		TrackList* tl = song->visibletracks();
		TrackList selectedTracks = song->getSelectedTracks();
		if (!selectedTracks.size())
		{
			return;
		}

		for (iTrack t = tl->begin(); t != tl->end(); ++t)
		{
			if (*t == *(--selectedTracks.end()))
			{
				if (*t != *(--tl->end()))
				{
					Track* next = *(++t);
					next->setSelected(true);
					song->update(SC_SELECTION);
					return;
				}

			}
		}
		return;
	}
	else if (key == shortcuts[SHRT_SEL_ALL_TRACK].key)
	{
		//printf("Select all tracks called\n");
		TrackList* tl = song->visibletracks();
		TrackList selectedTracks = song->getSelectedTracks();
		bool select = true;
		if (selectedTracks.size() == tl->size())
		{
			select = false;
		}

		for (iTrack t = tl->begin(); t != tl->end(); ++t)
		{
			(*t)->setSelected(select);
		}
		song->update(SC_SELECTION);
		return;
	}
	else if (key == shortcuts[SHRT_TRACK_TOGGLE_SOLO].key)
	{
		Track* t =oom->composer->curTrack();
		if (t)
		{
			audio->msgSetSolo(t, !t->solo());
			song->update(SC_SOLO);
		}
		return;
	}
	else if (key == shortcuts[SHRT_TRACK_TOGGLE_MUTE].key)
	{
		Track* t =oom->composer->curTrack();
		if (t)
		{
			t->setMute(!t->mute());
			song->update(SC_MUTE);
		}
		return;
	}
	else if (key == shortcuts[SHRT_MIDI_PANIC].key)
	{
		song->panic();
		return;
	}
	else if (key == shortcuts[SHRT_SET_QUANT_0].key)
	{
		oom->composer->_setRaster(0);
		oom->composer->raster->setCurrentIndex(0);
		return;
	}
	else if (key == shortcuts[SHRT_SET_QUANT_1].key)
	{
		oom->composer->_setRaster(1);
		oom->composer->raster->setCurrentIndex(1);
		return;
	}
	else if (key == shortcuts[SHRT_SET_QUANT_2].key)
	{
		oom->composer->_setRaster(2);
		oom->composer->raster->setCurrentIndex(2);
		return;
	}
	else if (key == shortcuts[SHRT_SET_QUANT_3].key)
	{
		oom->composer->_setRaster(3);
		oom->composer->raster->setCurrentIndex(3);
		return;
	}
	else if (key == shortcuts[SHRT_SET_QUANT_4].key)
	{
		oom->composer->_setRaster(4);
		oom->composer->raster->setCurrentIndex(4);
		return;
	}
	else if (key == shortcuts[SHRT_SET_QUANT_5].key)
	{
		oom->composer->_setRaster(5);
		oom->composer->raster->setCurrentIndex(5);
		return;
	}
	else if (key == shortcuts[SHRT_TRACK_HEIGHT_DEFAULT].key)
	{
		TrackList* tl = song->visibletracks();
		for (iTrack t = tl->begin(); t != tl->end(); ++t)
		{
			Track* tr = *t;
			if (tr->selected())
			{
				tr->setHeight(DEFAULT_TRACKHEIGHT);
			}
		}
		emit trackHeightChanged();
		song->update(SC_TRACK_MODIFIED);
		return;
	}
	else if (key == shortcuts[SHRT_TRACK_HEIGHT_FULL_SCREEN].key)
	{
		TrackList tl = song->getSelectedTracks();
		for (iTrack t = tl.begin(); t != tl.end(); ++t)
		{
			Track* tr = *t;
			tr->setHeight(height());
		}
		if (tl.size())
		{
			Track* tr = *tl.begin();
			oom->composer->verticalScrollSetYpos(track2Y(tr));
		}
		emit trackHeightChanged();
		song->update(SC_TRACK_MODIFIED);
		return;
	}
	else if (key == shortcuts[SHRT_TRACK_HEIGHT_SELECTION_FITS_IN_VIEW].key)
	{
		TrackList tl = song->getSelectedTracks();
		for (iTrack t = tl.begin(); t != tl.end(); ++t)
		{
			Track* tr = *t;
			tr->setHeight(height() / tl.size());
		}
		if (tl.size())
		{
			Track* tr = *tl.begin();
			oom->composer->verticalScrollSetYpos(track2Y(tr));
		}
		emit trackHeightChanged();
		song->update(SC_TRACK_MODIFIED);
		return;
	}

	else if (key == shortcuts[SHRT_TRACK_HEIGHT_2].key)
	{
		TrackList* tl = song->visibletracks();
		for (iTrack t = tl->begin(); t != tl->end(); ++t)
		{
			Track* tr = *t;
			if (tr->selected())
			{
				tr->setHeight(MIN_TRACKHEIGHT);
			}
		}
		emit trackHeightChanged();
		song->update(SC_TRACK_MODIFIED);
		return;
	}
	else if (key == shortcuts[SHRT_TRACK_HEIGHT_3].key)
	{
		TrackList* tl = song->visibletracks();
		for (iTrack t = tl->begin(); t != tl->end(); ++t)
		{
			Track* tr = *t;
			if (tr->selected())
			{
				tr->setHeight(100);
			}
		}
		emit trackHeightChanged();
		song->update(SC_TRACK_MODIFIED);
		return;
	}
	else if (key == shortcuts[SHRT_TRACK_HEIGHT_4].key)
	{
		TrackList* tl = song->visibletracks();
		for (iTrack t = tl->begin(); t != tl->end(); ++t)
		{
			Track* tr = *t;
			if (tr->selected())
			{
				tr->setHeight(180);
			}
		}
		emit trackHeightChanged();
		song->update(SC_TRACK_MODIFIED);
		return;
	}
	else if (key == shortcuts[SHRT_TRACK_HEIGHT_5].key)
	{
		TrackList* tl = song->visibletracks();
		for (iTrack t = tl->begin(); t != tl->end(); ++t)
		{
			Track* tr = *t;
			if (tr->selected())
			{
				tr->setHeight(320);
			}
		}
		emit trackHeightChanged();
		song->update(SC_TRACK_MODIFIED);
		return;
	}
	else if (key == shortcuts[SHRT_TRACK_HEIGHT_6].key)
	{
		TrackList* tl = song->visibletracks();
		for (iTrack t = tl->begin(); t != tl->end(); ++t)
		{
			Track* tr = *t;
			if (tr->selected())
			{
				tr->setHeight(640);
			}
		}
		emit trackHeightChanged();
		song->update(SC_TRACK_MODIFIED);
		return;
	}
	else if(key == shortcuts[SHRT_INSERT_PART].key)
	{
		TrackList sel = song->getSelectedTracks();
		if(sel.size() == 1)
		{
			Track* trk = sel.front();
			if(trk)
			{
				//printf("Found one track selected: %s\n", trk->name().toUtf8().constData());
				CItem* ci = addPartAtCursor(trk);
				if(ci)
				{
					newItem(ci, false);
				}
			}
		}
		return;
	}
	else if(key == shortcuts[SHRT_RENAME_TRACK].key)
	{
		TrackList sel = song->getSelectedTracks();
		if(sel.size() == 1)
		{
			Track* trk = sel.front();
			if(trk)
			{
				//printf("Found one track selected: %s\n", trk->name().toUtf8().constData());
				emit renameTrack(trk);
			}
		}
		return;
	}
	else if(key == shortcuts[SHRT_MOVE_TRACK_UP].key)
	{
		emit moveSelectedTracks(1);
		return;
	}
	else if(key == shortcuts[SHRT_MOVE_TRACK_DOWN].key)
	{
		emit moveSelectedTracks(-1);
		return;
	}

	//
	// Shortcuts that require selected parts from here
	//
    if (!_curItem)
	{
        if (_items.size() == 0)
		{
			event->ignore(); // give global accelerators a chance
			return;
		}
        for (iCItem i = _items.begin(); i != _items.end(); ++i)
		{
			NPart* part = (NPart*) (i->second);
			if (part->isSelected())
			{
                _curItem = part;
				break;
			}
		}
        if (!_curItem)
            _curItem = (NPart*) _items.begin()->second; // just grab the first part
	}

	CItem* newItem = 0;
	bool singleSelection = isSingleSelection();
	bool add = false;
	//Locators to selection
	if (key == shortcuts[SHRT_LOCATORS_TO_SELECTION].key)
	{
		CItem *leftmost = 0, *rightmost = 0;
		for (iCItem i = _items.begin(); i != _items.end(); i++)
		{
			if (i->second->isSelected())
			{
				// Check leftmost:
				if (!leftmost)
					leftmost = i->second;
				else
					if (leftmost->x() > i->second->x())
						leftmost = i->second;

				// Check rightmost:
				if (!rightmost)
					rightmost = i->second;
				else
					if (rightmost->x() < i->second->x())
						rightmost = i->second;
			}
		}

		int left_tick = leftmost->part()->tick();
		int right_tick = rightmost->part()->tick() + rightmost->part()->lenTick();
		Pos p1(left_tick, true);
		Pos p2(right_tick, true);
		song->setPos(1, p1);
		song->setPos(2, p2);
		return;
	}

		// Select part to the right
	else if (key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key)
	{
		if (key == shortcuts[SHRT_SEL_RIGHT_ADD].key)
			add = true;

                Part* part = _curItem->part();
		Track* track = part->track();
		unsigned int tick = part->tick();
		bool afterthis = false;
        for (iCItem i = _items.begin(); i != _items.end(); ++i)
		{
			NPart* npart = (NPart*) (i->second);
			Part* ipart = npart->part();
			if (ipart->track() != track)
				continue;
			if (ipart->tick() < tick)
				continue;
			if (ipart == part)
			{
				afterthis = true;
				continue;
			}
			if (afterthis)
			{
				newItem = i->second;
				break;
			}
		}
	}
		// Select part to the left
	else if (key == shortcuts[SHRT_SEL_LEFT].key || key == shortcuts[SHRT_SEL_LEFT_ADD].key)
	{
		if (key == shortcuts[SHRT_SEL_LEFT_ADD].key)
			add = true;

                Part* part = _curItem->part();
		Track* track = part->track();
		unsigned int tick = part->tick();

        for (iCItem i = _items.begin(); i != _items.end(); ++i)
		{
			NPart* npart = (NPart*) (i->second);
			Part* ipart = npart->part();

			if (ipart->track() != track)
				continue;
			if (ipart->tick() > tick)
				continue;
			if (ipart == part)
				break;
			newItem = i->second;
		}
	}

		// Select nearest part on track above
	else if (key == shortcuts[SHRT_SEL_ABOVE].key || key == shortcuts[SHRT_SEL_ABOVE_ADD].key)
	{
		if (key == shortcuts[SHRT_SEL_ABOVE_ADD].key)
			add = true;
		//To get an idea of which track is above us:
		int stepsize = rmapxDev(1);
                Track* track = _curItem->part()->track(); //top->part()->track();
		track = y2Track(track->y() - 1);

		//If we're at topmost, leave
		if (!track)
		{
			printf("no track above!\n");
			return;
		}
        int middle = _curItem->x() + _curItem->part()->lenTick() / 2;
		CItem *aboveL = 0, *aboveR = 0;
		//Upper limit: song end, lower limit: song start
		int ulimit = song->len();
		int llimit = 0;

		while (newItem == 0)
		{
			int y = track->y() + 2;
			int xoffset = 0;
			int xleft = middle - xoffset;
			int xright = middle + xoffset;
			while ((xleft > llimit || xright < ulimit) && (aboveL == 0) && (aboveR == 0))
			{
				xoffset += stepsize;
				xleft = middle - xoffset;
				xright = middle + xoffset;
				if (xleft >= 0)
                                        aboveL = _items.find(QPoint(xleft, y));
				if (xright <= ulimit)
                                        aboveR = _items.find(QPoint(xright, y));
			}

			if ((aboveL || aboveR) != 0)
			{ //We've hit something
				CItem* above = 0;
				above = (aboveL != 0) ? aboveL : aboveR;
				newItem = above;
			}
			else
			{ //We didn't hit anything. Move to track above, if there is one
				track = y2Track(track->y() - 1);
				if (track == 0)
					return;
			}
		}
		emit trackChanged(track);
	}
		// Select nearest part on track below
	else if (key == shortcuts[SHRT_SEL_BELOW].key || key == shortcuts[SHRT_SEL_BELOW_ADD].key)
	{
		if (key == shortcuts[SHRT_SEL_BELOW_ADD].key)
			add = true;

		//To get an idea of which track is below us:
		int stepsize = rmapxDev(1);
                Track* track = _curItem->part()->track(); //bottom->part()->track();
		track = y2Track(track->y() + track->height() + 1);
                int middle = _curItem->x() + _curItem->part()->lenTick() / 2;
		//If we're at bottommost, leave
		if (!track)
			return;

		CItem *belowL = 0, *belowR = 0;
		//Upper limit: song end , lower limit: song start
		int ulimit = song->len();
		int llimit = 0;
		while (newItem == 0)
		{
			int y = track->y() + 1;
			int xoffset = 0;
			int xleft = middle - xoffset;
			int xright = middle + xoffset;
			while ((xleft > llimit || xright < ulimit) && (belowL == 0) && (belowR == 0))
			{
				xoffset += stepsize;
				xleft = middle - xoffset;
				xright = middle + xoffset;
				if (xleft >= 0)
                                        belowL = _items.find(QPoint(xleft, y));
				if (xright <= ulimit)
                                        belowR = _items.find(QPoint(xright, y));
			}

			if ((belowL || belowR) != 0)
			{ //We've hit something
				CItem* below = 0;
				below = (belowL != 0) ? belowL : belowR;
				newItem = below;
			}
			else
			{
				//Get next track below, or abort if this is the lowest
				track = y2Track(track->y() + track->height() + 1);
				if (track == 0)
					return;
			}
		}
		emit trackChanged(track);
	}
    else if (key == shortcuts[SHRT_EDIT_PART].key && _curItem)
	{ //This should be the other way around - singleSelection first.
		if (!singleSelection)
		{
			event->ignore();
			return;
		}
		PartList* pl = new PartList;
                NPart* npart = (NPart*) (_curItem);
		Track* track = npart->part()->track();
		pl->add(npart->part());
		int type = 0;

		//  Check if track is wave or drum,
		//  else track is midi

		switch (track->type())
		{
			case Track::DRUM:
				type = 3;
				break;

			case Track::WAVE:
				type = 4;
				break;

			case Track::MIDI:
			case Track::AUDIO_OUTPUT:
			case Track::AUDIO_INPUT:
			case Track::AUDIO_BUSS:
			case Track::AUDIO_AUX:
			case Track::AUDIO_SOFTSYNTH: //TODO
				break;
		}
		emit startEditor(pl, type);
	}
    else if (key == shortcuts[SHRT_INC_POS].key)
    {
            _curItem->setWidth(_curItem->width() + 200);
            redraw();
            resizeItem(_curItem, false);
    }
    else if (key == shortcuts[SHRT_DEC_POS].key)
    {
            Part* part = _curItem->part();
			if(part->lenTick() > 200)
            	part->setLenTick(part->lenTick() - 200);
    }
	else if (key == shortcuts[SHRT_SELECT_ALL_NODES].key)
	{
		//printf("Should be select all here\n");
		if (_tool == AutomationTool)
		{
			if (automation.currentCtrlList)
			{
				if (_curveNodeSelection->size())
				{
					_curveNodeSelection->clearSelection();
				}
				else
				{
					iCtrl ic=automation.currentCtrlList->begin();
					for (; ic !=automation.currentCtrlList->end(); ic++)
					{
						CtrlVal &cv = ic->second;
						_curveNodeSelection->addNodeToSelection(&cv);
					}
				}
				redraw();
			}
		}
		return;
	}
	else
	{
		event->ignore(); // give global accelerators a chance
		return;
	}


	// Check if anything happened to the selected parts
	if (newItem)
	{
		//If this is a single selection, toggle previous item
		if (singleSelection && !add)
                        selectItem(_curItem, false);
		else if (!add)
			deselectAll();

                _curItem = newItem;
		selectItem(newItem, true);

		//Check if we've hit the upper or lower boundaries of the window. If so, set a new position
		if (newItem->x() < mapxDev(0))
		{
                        int curpos = _pos[0];
			setPos(0, newItem->x(), true);
			setPos(0, curpos, false); //Dummy to put the current position back once we've scrolled
		}
		else if (newItem->x() > mapxDev(width()))
		{
                        int curpos = _pos[0];
			setPos(0, newItem->x(), true);
			setPos(0, curpos, false); //Dummy to put the current position back once we've scrolled
		}
		redraw();
		emit selectionChanged();
	}
}/*}}}*/

//---------------------------------------------------------
//   drawPart
//    draws a part
//---------------------------------------------------------

void ComposerCanvas::drawItem(QPainter& p, const CItem* item, const QRect& rect)/*{{{*/
{
	int from = rect.x();
	int to = from + rect.width();

	//printf("from %d to %d\n", from,to);
	Part* part = ((NPart*) item)->part();
	
	MidiPart* mp = 0;
	WavePart* wp = 0;
	Track::TrackType type = part->track()->type();
	if (type == Track::WAVE)
	{
		wp = (WavePart*) part;
	}
	else
	{
		mp = (MidiPart*) part;
	}
	
	int i = part->colorIndex();
	QColor partWaveColor(config.partWaveColors[i]);
	QColor partColor(config.partColors[i]);
	
	QColor partWaveColorAutomation(config.partWaveColorsAutomation[i]);
	QColor partColorAutomation(config.partColorsAutomation[i]);
	
	int pTick = part->tick();
	from -= pTick;
	to -= pTick;
	if (from < 0)
		from = 0;
	if ((unsigned int) to > part->lenTick())
		to = part->lenTick();

	// Item bounding box x is in tick coordinates, same as rectangle.
	if (item->bbox().intersect(rect).isNull())
	{
		//printf("ComposerCanvas::drawItem rectangle is null\n");
		return;
	}

	QRect r = item->bbox();

	//printf("ComposerCanvas::drawItem %s evRefs:%d pTick:%d pLen:%d\nbb.x:%d bb.y:%d bb.w:%d bb.h:%d\n"
	//       "rect.x:%d rect.y:%d rect.w:%d rect.h:%d\nr.x:%d r.y:%d r.w:%d r.h:%d\n",
	//  part->name().toLatin1().constData(), part->events()->arefCount(), pTick, part->lenTick(),
	//  bb.x(), bb.y(), bb.width(), bb.height(),
	//  rect.x(), rect.y(), rect.width(), rect.height(),
	//  r.x(), r.y(), r.width(), r.height());

	p.setPen(Qt::black);
	if(item->isMoving())
	{
		QColor c(Qt::gray);
		c.setAlpha(config.globalAlphaBlend);
		p.setBrush(c);
	}
	else if (part->selected())
	{
		partWaveColor.setAlpha(150);
		partWaveColorAutomation.setAlpha(150);
		
		p.setBrush(partWaveColor);
		if (wp)
		{
			if(m_myLeftFlag || m_myRightFlag)
				p.setPen(partColor);
			else
				p.setPen(Qt::NoPen);
			if (_tool == AutomationTool)
				p.setBrush(partWaveColorAutomation);
		}
		else if(mp)
			p.setPen(partColor);
	}
	else
	{
		partColor.setAlpha(150);
		partColorAutomation.setAlpha(150);
	
		p.setBrush(partColor);
		if (wp)
		{
			p.setPen(Qt::NoPen);
			if (_tool == AutomationTool)
				p.setBrush(partColorAutomation);
		}
		else if(mp)
			p.setPen(partWaveColor);
			
		
	}
	p.drawRect(QRect(r.x(), r.y(), r.width(), mp ? r.height()-2 : r.height()-1));
	if (part->mute() || part->track()->mute())
	{
		QBrush muteBrush;
		muteBrush.setStyle(Qt::HorPattern);
		if(part->selected())
		{
			partColor.setAlpha(120);
			muteBrush.setColor(partColor);
		}
		else
		{
			partWaveColor.setAlpha(120);
			muteBrush.setColor(partWaveColor);
		}
		p.setBrush(muteBrush);

		// NOTE: For one-pixel border use first line For two-pixel border use second.
		p.drawRect(QRect(r.x(), r.y(), r.width(), r.height()-1));
	}

	trackOffset += part->track()->height();
	partColor.setAlpha(255);
	partWaveColor.setAlpha(255);

	if (wp)
		drawWavePart(p, rect, wp, r);
	else if (mp)
	{
		if(part->selected())
			drawMidiPart(p, rect, mp->events(),(MidiTrack*)part->track(), r, mp->tick(), from, to, partColor);
		else
			drawMidiPart(p, rect, mp->events(),(MidiTrack*)part->track(), r, mp->tick(), from, to, partWaveColor);
	}

	if (config.canvasShowPartType & 1)
	{ // show names
		// draw name
		QRect rr = map(r);
		rr.setX(rr.x() + 3);
		rr.setHeight(rr.height()-2);
		p.save();
		p.setFont(config.fonts[1]);
		p.setWorldMatrixEnabled(false);
		if(part->selected())
		{
			if (_tool == AutomationTool)
				p.setPen(partColorAutomation);
			else
				p.setPen(partColor);
			p.setFont(QFont("fixed-width", 7, QFont::Bold));
		}
		else
		{
			if (_tool == AutomationTool)
				p.setPen(partWaveColorAutomation);
			else
				p.setPen(partWaveColor);
			p.setFont(QFont("fixed-width", 7, QFont::Bold));
		}
		p.drawText(rr, Qt::AlignBottom | Qt::AlignLeft, part->name());
		p.restore();
	}
}/*}}}*/

//---------------------------------------------------------
//   drawMoving
//    draws moving items
//---------------------------------------------------------

void ComposerCanvas::drawMoving(QPainter& p, const CItem* item, const QRect&)/*{{{*/
{
	p.setPen(Qt::black);

	Part* part = ((NPart*) item)->part();
	QColor c(config.partColors[part->colorIndex()]);

	c.setAlpha(128); // Fix this regardless of global setting. Should be OK.

	p.setBrush(c);

	// NOTE: For one-pixel border use second line. For two-pixel border use first.
	p.drawRect(item->mp().x(), item->mp().y(), item->width(), item->height());
}/*}}}*/

void ComposerCanvas::drawMidiPart(QPainter& p, const QRect&, EventList* events, MidiTrack *mt, const QRect& r, int pTick, int from, int to, QColor c)/*{{{*/
{
	if (config.canvasShowPartType & 2) {      // show events
		// Do not allow this, causes segfault.
		if(from <= to)
		{
			p.setPen(c);
			iEvent ito(events->lower_bound(to));

			for (iEvent i = events->lower_bound(from); i != ito; ++i) {
				EventType type = i->second.type();
				if (
						((config.canvasShowPartEvent & 1) && (type == Note))
						|| ((config.canvasShowPartEvent & 2) && (type == PAfter))
						|| ((config.canvasShowPartEvent & 4) && (type == Controller))
						|| ((config.canvasShowPartEvent &16) && (type == CAfter))
						|| ((config.canvasShowPartEvent &64) && (type == Sysex || type == Meta))
						) {
					int t = i->first + pTick;
					int th = mt->height();
					if(t >= r.left() && t <= r.right())
						p.drawLine(t, r.y()+2, t, r.y()+th-4);
				}
			}
		}
	}
	else
	{      // show Cakewalk Style
		p.setPen(c);
		iEvent ito(events->lower_bound(to));

		for (iEvent i = events->begin(); i != ito; ++i)
		{
            int t  = i->first + pTick;
            int te = t + i->second.lenTick();

            if (t > (to + pTick))
				break;

            if (te < (from + pTick))
				continue;

            if (te > (to + pTick))
				te = to + pTick;

            EventType type = i->second.type();
			if (type == Note)
			{
				int pitch = i->second.pitch();
				int th = int(mt->height() * 0.75);
				int hoffset = (mt->height() - th ) / 2;
				int y     =  hoffset + (r.y() + th - (pitch * (th) / 127));
				p.drawLine(t, y, te, y);
            }
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   drawWavePart
//    bb - bounding box of paint area
//    pr - part rectangle
//---------------------------------------------------------

void ComposerCanvas::drawWavePart(QPainter& p, const QRect& bb, WavePart* wp, const QRect& _pr)/*{{{*/
{
	int i = wp->colorIndex();
	QColor waveFill(config.partWaveColors[i]);
	QColor waveEdge = QColor(211,193,224);
	
	//printf("ComposerCanvas::drawWavePart bb.x:%d bb.y:%d bb.w:%d bb.h:%d  pr.x:%d pr.y:%d pr.w:%d pr.h:%d\n",
	//  bb.x(), bb.y(), bb.width(), bb.height(), _pr.x(), _pr.y(), _pr.width(), _pr.height());
	QPolygonF m_monoPolygonTop;
	QPolygonF m_monoPolygonBottom;
	
	QPolygonF m_stereoOnePolygonTop;
	QPolygonF m_stereoOnePolygonBottom;

	QPolygonF m_stereoTwoPolygonTop;
	QPolygonF m_stereoTwoPolygonBottom;
	
	int firstIn = 1;
	int stereoOneFirstIn = 1;
	int stereoTwoFirstIn = 1;
	
	p.setPen(QColor(255,0,0));

	QColor green = QColor(49, 175, 197);
	QColor yellow = QColor(127,12,128);
	QColor red = QColor(197, 49, 87);
	QColor rms_color = QColor(0,19,23);
	
	if(wp->selected())
		waveFill = QColor(config.partColors[i]);
	
	if (_tool == AutomationTool)
	{
		if(wp->selected())
			waveFill = QColor(config.partColorsAutomation[i]);
		else
			waveFill = QColor(config.partWaveColorsAutomation[i]);
	}


	QRect rr = p.worldMatrix().mapRect(bb);
	QRect pr = p.worldMatrix().mapRect(_pr);

	p.save();
	p.resetTransform();

	int x2 = 1;
	int x1 = rr.x() > pr.x() ? rr.x() : pr.x();
	x2 += rr.right() < pr.right() ? rr.right() : pr.right();
	//printf("x1 = %d, x2 = %d\n", x1, x2);
	if (x1 < 0)
		x1 = 0;
	if (x2 > width())
		x2 = width();
	int hh = pr.height();
	int h = hh / 2;
	int y = pr.y() + h;
	int drawLines = 1;
	EventList* el = wp->events();
	for (iEvent e = el->begin(); e != el->end(); ++e)/*{{{*/
	{
		
		int cc = hh % 2 ? 0 : 1;
		Event event = e->second;
		SndFileR f = event.sndFile();
		if (f.isNull())
			continue;
		unsigned channels = f.channels();
		if (channels == 0)
		{
			printf("drawWavePart: channels==0! %s\n", f.name().toLatin1().constData());
			continue;
		}

		//printf("SndFileR samples=%d channels=%d event samplepos=%d clipframes=%d event lenframe=%d, event.frame=%d part_start=%d part_length=%d\n", 
		//		f.samples(), f.channels(), event.spos(), clipframes, event.lenFrame(), event.frame(), wp->frame(), wp->lenFrame());

		int xScale;
		int pos;
		int tickstep = rmapxDev(1);
		int postick = tempomap.frame2tick(wp->frame() + event.frame());
		int eventx = mapx(postick);
		int drawoffset;
		if ((x1 - eventx) < 0)
			drawoffset = 0;
		else
			drawoffset = rmapxDev(x1 - eventx);
		postick += drawoffset;
		pos = event.spos() + tempomap.tick2frame(postick) - wp->frame() - event.frame();

		int i;
		if (x1 < eventx)
			i = eventx;
		else
			i = x1;
		int ex = mapx(tempomap.frame2tick(wp->frame() + event.frame() + event.lenFrame()));
		if (ex > x2)
			ex = x2;
		if (h < 41)
		{
			//
			//    combine multi channels into one waveform
			//
			//printf("ComposerCanvas::drawWavePart i:%d ex:%d\n", i, ex);  // REMOVE Tim.
			if(drawLines == 0)
			{
				for (; i < ex; i++)//{{{
				{
					int hm = hh / 2;
					SampleV sa[channels];
					xScale = tempomap.deltaTick2frame(postick, postick + tickstep);
					f.read(sa, xScale, pos);
					postick += tickstep;
					pos += xScale;
					int peak = 0;
					int rms = 0;
					for (unsigned k = 0; k < channels; ++k)
					{
						if (sa[k].peak > peak)
							peak = sa[k].peak;
						rms += sa[k].rms;
					}
					rms /= channels;
					peak = (peak * (hh - 2)) >> 9;
					rms = (rms * (hh - 2)) >> 9;
					
					p.setPen(green);
					
					p.drawLine(i, y - peak - cc, i, y + peak);
					p.setPen(rms_color);
					p.drawLine(i, y - rms - cc, i, y + rms);
				
					if(peak >= (hm - 2))
					{
						p.setPen(QColor(255,0,0));
						p.drawLine(i, y - peak - cc, i, y - peak - cc + 1);
						p.drawLine(i, y + peak - 1, i, y + peak);
					}	
				}//}}}
			}
			else
			{
				int hm = 0;
				for (; i < ex; i++)//{{{//drawMonoWave
				{
					if(firstIn == 1)
					{
						m_monoPolygonTop.append(QPointF(i, y));
						m_monoPolygonBottom.append(QPointF(i, y));
						firstIn = 0;
					}
					hm = hh / 2;
					SampleV sa[channels];
					xScale = tempomap.deltaTick2frame(postick, postick + tickstep);
					
					if(xmag <= -301)/*{{{*/
					{
						if( i % 10==1 || i % 10==2 || i % 10==3 || i % 10==4 || i % 10==5 || i % 10 == 7 || i % 10==8 || i % 10==9)
						{ 
							postick += tickstep;
							pos += xScale;
	  						continue;
	 					}
					}
					else if(xmag <= -41)
					{
						if( i % 10==1 || i % 10==2 || i % 10==4 || i % 10==5 || i % 10 == 7 || i % 10==8)
						{ 
							postick += tickstep;
							pos += xScale;
	  						continue;
	 					}
					}
					else if(xmag <= -15)
					{
						if( i % 2==0)
						{ 
							postick += tickstep;
							pos += xScale;
	  						continue;
	 					}
					}/*}}}*/
					f.read(sa, xScale, pos); //this read is slow
					postick += tickstep;
					pos += xScale;
					int peak = 0;
					int rms = 0;
					for (unsigned k = 0; k < channels; ++k)
					{
						if (sa[k].peak > peak)
							peak = sa[k].peak;
						rms += sa[k].rms;
					}
					rms /= channels;
					peak = (peak * (hh - 2)) >> 9;
					rms = (rms * (hh - 2)) >> 9;
				
					m_monoPolygonTop.append(QPointF(i, y-peak));
					m_monoPolygonBottom.append(QPointF(i, y+peak));
					
					if(peak >= (hm - 2))
					{
						p.drawLine(i, y - peak - cc, i, y - peak - cc + 1);
						p.drawLine(i, y + peak - 1, i, y + peak);
					}
					
				}//}}}
				m_monoPolygonTop.append(QPointF(i, y));
				m_monoPolygonBottom.append(QPointF(i, y));
				
				p.setPen(Qt::NoPen);//this is the outline of the wave
				p.setBrush(waveFill);//waveFill);//this is the fill color of the wave
				
				p.drawPolygon(m_monoPolygonTop);
				p.drawPolygon(m_monoPolygonBottom);
				firstIn = 1;
				
			}
		}
		else
		{
			//
			//  multi channel display
			//
			int hm = hh / (channels * 2);
			int cliprange = 1;
			//TODO: I need to base the calculation for clipping on the actual sample and db and not on the height of the track.
			if(hm < 50)
				cliprange = 2;
			else if(hm >= 50 && hm <= 200)
				cliprange = 3;
			else if(hm < 300)
				cliprange = 5;
			else
				cliprange = 6;

			int cc = hh % (channels * 2) ? 0 : 1;
			//printf("channels = %d, pr = %d, h = %d, hh = %d, hm = %d\n", channels, pr.height(), h, hh, hm);
			//printf("canvas height: %d\n", height());
			if(drawLines == 0)
			{
				for (; i < ex; i++)//{{{ //stereo line draw mode
				{
					y = pr.y() + hm;
					SampleV sa[channels];
					xScale = tempomap.deltaTick2frame(postick, postick + tickstep);
					f.read(sa, xScale, pos);
					postick += tickstep;
					pos += xScale;
					for (unsigned k = 0; k < channels; ++k)
					{
						int peak = (sa[k].peak * (hm - 1)) >> 8;
						int rms = (sa[k].rms * (hm - 1)) >> 8;
						if(k == 0)
						{
							p.setPen(green);
							p.drawLine(i, y - peak - cc, i, y + peak);
						}
						else
						{
							p.setPen(green);
							p.drawLine(i, y - peak - cc, i, y + peak);
							
						}
						//printf("peak value: %d hm value: %d\n", peak, hm);
						if(peak >= (hm - cliprange))
						{
							p.setPen(QColor(255,0,0));
							p.drawLine(i, y - peak - cc, i, y - peak - cc + 1);
							p.drawLine(i, y + peak - 1, i, y + peak);
						}	
						p.setPen(rms_color);
						p.drawLine(i, y - rms - cc, i, y + rms);
						
						if(k == 0)
						{
							p.setPen(QColor(102,177,205));
							p.drawLine(0, y, width(), y);
						}
						else
						{
							p.setPen(QColor(213,93,93));
							p.drawLine(0, y, width(), y);
						}

						y += 2 * hm;
					}
				}//}}}
			}
			else
			{
				int stereoOneY = 0;
				int stereoTwoY = 0;
				for (; i < ex; i++)//{{{ //stereo curve draw mode
				{
					y = pr.y() + hm;
					SampleV sa[channels];
					xScale = tempomap.deltaTick2frame(postick, postick + tickstep);
					if(xmag <= -301)/*{{{*/
					{
						if( i % 10==1 || i % 10==2 || i % 10==3 || i % 10==4 || i % 10==5 || i % 10 == 7 || i % 10==8 || i % 10==9)
						{ 
							postick += tickstep;
							pos += xScale;
	  						continue;
	 					}
					}
					else if(xmag <= -41)
					{
						if( i % 10==1 || i % 10==2 || i % 10==4 || i % 10==5 || i % 10 == 7 || i % 10==8)
						{ 
							postick += tickstep;
							pos += xScale;
	  						continue;
	 					}
					}
					else if(xmag <= -15)
					{
						if( i % 2==0)
						{ 
							postick += tickstep;
							pos += xScale;
	  						continue;
	 					}
					}/*}}}*/
					f.read(sa, xScale, pos);
					postick += tickstep;
					pos += xScale;
					for (unsigned k = 0; k < channels; ++k)
					{
						int peak = (sa[k].peak * (hm - 1)) >> 8;
						if(k == 0)
						{
							stereoOneY = y;
							if(stereoOneFirstIn == 1)
							{
								m_stereoOnePolygonTop.append(QPointF(i, stereoOneY));
								m_stereoOnePolygonBottom.append(QPointF(i, stereoOneY));
								stereoOneFirstIn = 0;
							}
							m_stereoOnePolygonTop.append(QPointF(i, y-peak));
							m_stereoOnePolygonBottom.append(QPointF(i, y+peak));
						}
						else
						{
							stereoTwoY = y;
							if(stereoTwoFirstIn == 1)
							{
								m_stereoTwoPolygonTop.append(QPointF(i, stereoTwoY));
								m_stereoTwoPolygonBottom.append(QPointF(i, stereoTwoY));
								stereoTwoFirstIn = 0;
							}
							m_stereoTwoPolygonTop.append(QPointF(i, y-peak));
							m_stereoTwoPolygonBottom.append(QPointF(i, y+peak));
						}
						//printf("peak value: %d hm value: %d\n", peak, hm);
						if(peak >= (hm - cliprange))
						{
							p.drawLine(i, y - peak - cc, i, y - peak - cc + 1);
							p.drawLine(i, y + peak - 1, i, y + peak);
						}	

						y += 2 * hm;
					}
				}//}}}
				m_stereoOnePolygonTop.append(QPointF(i, stereoOneY));
				m_stereoOnePolygonBottom.append(QPointF(i, stereoOneY));
				m_stereoTwoPolygonTop.append(QPointF(i, stereoTwoY));
				m_stereoTwoPolygonBottom.append(QPointF(i, stereoTwoY));
				
				p.setPen(Qt::NoPen);//this is the outline of the wave
				p.setBrush(waveFill);//this is the fill color of the wave
				
				p.drawPolygon(m_stereoOnePolygonTop);
				p.drawPolygon(m_stereoOnePolygonBottom);
				p.drawPolygon(m_stereoTwoPolygonTop);
				p.drawPolygon(m_stereoTwoPolygonBottom);
				stereoOneFirstIn = 1;
				stereoTwoFirstIn = 1;
			}
		}
	}/*}}}*/
	QColor fadeColor(config.partColors[i]);
	fadeColor.setAlpha(120);
	QPen greenPen(fadeColor);
	greenPen.setCosmetic(true);
	p.setPen(greenPen);
	FadeCurve* fadeIn = wp->fadeIn();
	FadeCurve* fadeOut = wp->fadeOut();
	//printf("FadeIn width:%ld\n", long(fadeIn->width()));
	//printf("FadeOut width:%ld\n", long(fadeOut->width()));
	p.setRenderHint(QPainter::Antialiasing, true);
	if(fadeIn)
	{
		long pstart = tempomap.frame2tick(wp->frame());
		long partx = mapx(pstart);
		long fpos = tempomap.frame2tick(wp->frame()+fadeIn->width());
		long fadex = mapx(fpos);
		
		if(fadeIn->width() > 0)
		{
			int fiy = pr.top();
			int fix = partx;
			int fiw = int(fadex);
			int fih = pr.bottom();
			QPolygon fadeInCurve(5);
			fadeInCurve.setPoint(0, fix, fiy);
			fadeInCurve.setPoint(1, fix, fih);
			fadeInCurve.setPoint(2, fix, fih);
			fadeInCurve.setPoint(3, fix, fih);
			fadeInCurve.setPoint(4, fiw, fiy);
			p.setBrush(fadeColor);
			p.drawPolygon(fadeInCurve);
		}

		int lpos = fadex-3;
		QRect picker((lpos < partx) ? partx : lpos, pr.top(), 6, 6);
		p.setBrush(waveFill);
		p.drawRect(picker);
	}
	if(fadeOut)
	{
		long pend = tempomap.frame2tick(wp->frame() + wp->lenFrame());
		long fpos = (wp->frame() + wp->lenFrame())-fadeOut->width();
		fpos = tempomap.frame2tick(fpos);
		long fadex = mapx(fpos);
		long endx = mapx(pend);

		if(fadeOut->width() > 0)
		{
			int fiy = pr.top();
			int fix = endx;
			int fiw = int(fadex);
			int fih = pr.bottom();
			QPolygon fadeOutCurve(5);

			fadeOutCurve.setPoint(0, fix, fiy);
			fadeOutCurve.setPoint(1, fix, fih);
			fadeOutCurve.setPoint(2, fix, fih);
			fadeOutCurve.setPoint(3, fix, fih);
			fadeOutCurve.setPoint(4, fiw, fiy);

			p.setBrush(fadeColor);
			p.drawPolygon(fadeOutCurve);
		}
		
		int rpos = fadex-3;
		if((rpos + 6) > endx)
			rpos = (endx - 6);
		QRect picker(rpos, pr.top(), 6, 6);
		p.setBrush(waveFill);
		p.drawRect(picker);
	}
	p.setRenderHint(QPainter::Antialiasing, false);
	p.restore();
}/*}}}*/

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void ComposerCanvas::cmd(int cmd)/*{{{*/
{
	PartList pl;
        for (iCItem i = _items.begin(); i != _items.end(); ++i)
	{
		if (!i->second->isSelected())
			continue;
		NPart* npart = (NPart*) (i->second);
		pl.add(npart->part());
	}
	switch (cmd)
	{
		case CMD_CUT_PART:
			copy(&pl);
			song->startUndo();

			bool loop;
			do
			{
				loop = false;
                                for (iCItem i = _items.begin(); i != _items.end(); ++i)
				{
					if (!i->second->isSelected())
						continue;
					NPart* p = (NPart*) (i->second);
					Part* part = p->part();
					audio->msgRemovePart(part);

					loop = true;
					break;
				}
			} while (loop);
			song->endUndo(SC_PART_REMOVED);
			break;
		case CMD_COPY_PART:
			copy(&pl);
			break;
		case CMD_PASTE_PART:
			paste(false, false);
			break;
		case CMD_PASTE_CLONE_PART:
			paste(true, false);
			break;
		case CMD_PASTE_PART_TO_TRACK:
			paste();
			break;
		case CMD_PASTE_CLONE_PART_TO_TRACK:
			paste(true);
			break;
		case CMD_INSERT_PART:
			paste(false, false, true);
			break;
		case CMD_INSERT_EMPTYMEAS:
		{
			song->startUndo();
			int startPos = song->vcpos();
			int oneMeas = AL::sigmap.ticksMeasure(startPos);
			movePartsTotheRight(startPos, oneMeas);
			song->endUndo(SC_PART_INSERTED);
			break;
		}
		case CMD_REMOVE_SELECTED_AUTOMATION_NODES:
		{
			if (_tool == AutomationTool)
			{
				if (automation.currentCtrlList && _curveNodeSelection->size())/*{{{*/
				{
					QList<CtrlVal> valuesToRemove;

					QList<CtrlVal*> selectedNodes = _curveNodeSelection->getNodes();
					foreach(CtrlVal* val, selectedNodes)
					{
						if (val->getFrame() != 0)
						{
							valuesToRemove.append(CtrlVal(val->getFrame(), val->val));
							//automation.currentCtrlList->del(val->getFrame());
						}
					}
					AddRemoveCtrlValues* removeCommand = new AddRemoveCtrlValues(automation.currentCtrlList, valuesToRemove, OOMCommand::REMOVE);

					CommandGroup* group = new CommandGroup(tr("Delete Automation Nodes"));
					group->add_command(removeCommand);

					song->pushToHistoryStack(group);
					_curveNodeSelection->clearSelection();
					redraw();
					return;
				}/*}}}*/

				if (automation.currentCtrlVal && automation.currentCtrlList)
				{
					CtrlVal& firstCtrlVal = automation.currentCtrlList->begin()->second;
					if (automation.currentCtrlVal->getFrame() != firstCtrlVal.getFrame())
					{
						AddRemoveCtrlValues* removeCommand = new AddRemoveCtrlValues(automation.currentCtrlList, CtrlVal(automation.currentCtrlVal->getFrame(), automation.currentCtrlVal->val), OOMCommand::REMOVE);

						CommandGroup* group = new CommandGroup(tr("Delete Automation Node"));
						group->add_command(removeCommand);

						song->pushToHistoryStack(group);
						_curveNodeSelection->clearSelection();
						//automation.currentCtrlList->del(automation.currentCtrlVal->getFrame());
						redraw();
					}
				}
			}
			break;
		}
		case CMD_COPY_AUTOMATION_NODES:
		{
			if (_tool == AutomationTool)
			{
				//printf("Copy automation called\n");
				copyAutomation();
			}
		}
		break;
		case CMD_PASTE_AUTOMATION_NODES:
		{
			if (_tool == AutomationTool)
			{
				//printf("Paste automation called\n");
				pasteAutomation();
			}
		}
		break;
		case CMD_SELECT_ALL_AUTOMATION:
		{
			if (_tool == AutomationTool)
			{
				if (automation.currentCtrlList)
				{
					iCtrl ic = automation.currentCtrlList->begin();
					for (; ic !=automation.currentCtrlList->end(); ic++) {
						CtrlVal &cv = ic->second;
						automation.currentCtrlVal = &cv;
						automation.controllerState = movingController;
						_curveNodeSelection->addNodeToSelection(automation.currentCtrlVal);
					}
					redraw();
				}

			}
			break;
		}
	}
}/*}}}*/

/**
 * Copy portion of the copy paste of automation curves
 * Copies the first selected autiomation curve from the selected track
 * to the Clipboard as a xml text. The text has been converted to a byte array.
 */
void ComposerCanvas::copyAutomation()/*{{{*/
{
	if(automation.currentCtrlList)
	{
		const CtrlList* cl = automation.currentCtrlList;
		QDomDocument doc("AutomationCurve");
		QDomElement root = doc.createElement("AutomationCurve");
		doc.appendChild(root);

		QDomElement control = doc.createElement("controller");
		root.appendChild(control);
		control.setAttribute("id", cl->id());
		control.setAttribute("cur", cl->curVal());
		control.setAttribute("color", cl->color().name());
		control.setAttribute("visible", cl->isVisible());

		if(_curveNodeSelection->size())
		{
			control.setAttribute("partial", true);
			QList<CtrlVal*> selectedNodes = _curveNodeSelection->getNodes();
			foreach(CtrlVal* val, selectedNodes)
			{
				QDomElement node = doc.createElement("node");
				control.appendChild(node);
				node.setAttribute("frame", val->getFrame());
				node.setAttribute("value", val->val);
			}
		}
		else
		{
			control.setAttribute("partial", false);
			for (ciCtrl ic = cl->begin(); ic != cl->end(); ++ic)
			{
				QDomElement node = doc.createElement("node");
				control.appendChild(node);
				node.setAttribute("frame", ic->second.getFrame());
				node.setAttribute("value", ic->second.val);
			}
		}
		QString xml = doc.toString();
		//qDebug() << xml;
		QByteArray data = xml.toUtf8();
		QMimeData* md = new QMimeData();
		md->setData("text/x-oom-automationcurve", data);
		QApplication::clipboard()->setMimeData(md, QClipboard::Clipboard);
	}
}/*}}}*/


void ComposerCanvas::pasteAutomation()/*{{{*/
{
	TrackList selectedTracks = song->getSelectedTracks();
	if (!selectedTracks.size())
	{
		if(debugMsg)
			printf("No selected track for paste\n");
		return;
	}
	if(selectedTracks.size() > 1)
	{
		QMessageBox::critical(this, tr("Copy Automation"), tr("You cannot copy automation with multiple tracks selected."));
		return;
	}
	iTrack itrack = selectedTracks.begin();
    if((*itrack)->isMidiTrack() && (*itrack)->wantsAutomation() == false)
	{
		if(debugMsg)
			printf("Midi track cannot paste automation\n");
		return;
	}
	AudioTrack *track = (AudioTrack*)*itrack;
	QString subtype("x-oom-automationcurve");
	QString format("text/" + subtype);
	QClipboard* cb = QApplication::clipboard();
	const QMimeData* md = cb->mimeData(QClipboard::Clipboard);
	if (!md->hasFormat(format))
	{
		if(debugMsg)
			printf("No automation curves in clipboard");
		return;
	}
	QString xml = cb->text(subtype, QClipboard::Clipboard);
	//qDebug() << xml;
	if(xml.isEmpty())
	{
		if(debugMsg)
			printf("Empty xml string from clipboard\n");
		return;
	}
	QDomDocument doc("AutomationCurve");
	if(!doc.setContent(xml))
	{//message dialog here???
		if(debugMsg)
			printf("failed to set content on dom document\n");
		return;
	}
	if(track)
	{
		QDomElement root = doc.documentElement();
		if(debugMsg)
			printf("Root document name: %s\n", root.tagName().toUtf8().constData());
		if(!root.isNull())
		{
			QDomNodeList cList = root.elementsByTagName("controller");
			for(int i = 0; i < cList.count(); ++i)
			{
				QDomElement control = cList.at(i).toElement();
				int id = control.attribute("id").toInt();
				bool partial = control.attribute("partial").toInt();
				bool visible = control.attribute("visible").toInt();
				double curVal = control.attribute("cur").toDouble();
				CtrlList *cl = 0;
				for (ciCtrlList icl = track->controller()->begin(); icl != track->controller()->end(); ++icl)
				{
					if(icl->second->id() == id)
					{
						cl = icl->second;
						break;
					}
				}
				if(cl)
				{
					QList<CtrlVal> valuesToAdd;
					QList<CtrlVal> valuesToRemove;

					if(partial)
					{//Add nodes from PB forward
						QDomNodeList nodeList = control.elementsByTagName("node");
						if(!nodeList.count())
							return;
						cl->setVisible(visible);
						cl->setCurVal(curVal);
						int spos = tempomap.tick2frame(song->cpos());
						int lastframe = spos;
						int lastreal;
						if(nodeList.count() > 1)
						{
							QDomElement first = nodeList.at(0).toElement();
							QDomElement last = nodeList.at(nodeList.count()-1).toElement();
							int fframe = first.attribute("frame").toInt();
							int lframe = last.attribute("frame").toInt();
							int range = lframe-fframe;
							int endframe = spos+range;
							for (ciCtrl ic = cl->begin(); ic != cl->end(); ++ic)
							{
								if(ic->second.getFrame() > spos && ic->second.getFrame() < endframe)
								{
									valuesToRemove.append(ic->second);
								}
							}
						}
						for(int n = 0; n < nodeList.count(); ++n)
						{
							QDomElement node = nodeList.at(n).toElement();
							int frame = node.attribute("frame").toInt();
							if(n == 0)
							{	
								lastreal = frame;
								frame = spos;
							}
							else
							{
								int diff = frame - lastreal;
								lastreal = frame;
								frame = lastframe+diff;
								lastframe = frame;
							}
							double val = node.attribute("value").toDouble();
							valuesToAdd.append(CtrlVal(frame, val));
						}
					}
					else
					{//Just add the whole line
						QDomNodeList nodeList = control.elementsByTagName("node");
						if(!nodeList.count())
							return;
						cl->clear();
						cl->setVisible(visible);
						cl->setCurVal(curVal);
						for(int n = 0; n < nodeList.count(); ++n)
						{
							QDomElement node = nodeList.at(n).toElement();
							int frame = node.attribute("frame").toInt();
							double val = node.attribute("value").toDouble();
							valuesToAdd.append(CtrlVal(frame, val));
						}
					}

					AddRemoveCtrlValues* addCommand = new AddRemoveCtrlValues(cl, valuesToAdd, OOMCommand::ADD);
					AddRemoveCtrlValues* removeCommand = new AddRemoveCtrlValues(cl, valuesToRemove, OOMCommand::REMOVE);

					CommandGroup* group = new CommandGroup(tr("Copy Automation Nodes"));
					group->add_command(addCommand);
					group->add_command(removeCommand);

					song->pushToHistoryStack(group);
				}
				//in the future we can support copying multiple lanes at once
				break;
			}
		}
	}
	update();
}/*}}}*/

//---------------------------------------------------------
//   copy
//    cut copy paste
//---------------------------------------------------------

void ComposerCanvas::copy(PartList* pl)/*{{{*/
{
	//printf("void ComposerCanvas::copy(PartList* pl)\n");
	if (pl->empty())
		return;
	bool wave = false;
	bool midi = false;
	for (ciPart p = pl->begin(); p != pl->end(); ++p)
	{
		if (p->second->track()->isMidiTrack())
			midi = true;
		else
			if (p->second->track()->type() == Track::WAVE)
			wave = true;
		if (midi && wave)
			break;
	}
	if (!(midi || wave))
		return;

	//---------------------------------------------------
	//    write parts as XML into tmp file
	//---------------------------------------------------

	FILE* tmp = tmpfile();
	if (tmp == 0)
	{
		fprintf(stderr, "ComposerCanvas::copy() fopen failed: %s\n",
				strerror(errno));
		return;
	}
	Xml xml(tmp);

	// Clear the copy clone list.
	cloneList.clear();

	int level = 0;
	int tick = 0;
	for (ciPart p = pl->begin(); p != pl->end(); ++p)
	{
		// Indicate this is a copy operation. Also force full wave paths.
		p->second->write(level, xml, true, true);

		int endTick = p->second->endTick();
		if (endTick > tick)
			tick = endTick;
	}
	Pos p(tick, true);
	song->setPos(0, p);

	//---------------------------------------------------
	//    read tmp file into QTextDrag Object
	//---------------------------------------------------

	fflush(tmp);
	struct stat f_stat;
	if (fstat(fileno(tmp), &f_stat) == -1)
	{
		fprintf(stderr, "ComposerCanvas::copy() fstat failed:<%s>\n",
				strerror(errno));
		fclose(tmp);
		return;
	}
	int n = f_stat.st_size;
	char* fbuf = (char*) mmap(0, n + 1, PROT_READ | PROT_WRITE,
			MAP_PRIVATE, fileno(tmp), 0);
	fbuf[n] = 0;

	QByteArray data(fbuf);
	QMimeData* md = new QMimeData();


	if (midi && wave)
		md->setData("text/x-oom-mixedpartlist", data); // By T356. Support mixed .mpt files.
	else
		if (midi)
		md->setData("text/x-oom-midipartlist", data);
	else
		if (wave)
		md->setData("text/x-oom-wavepartlist", data);

	QApplication::clipboard()->setMimeData(md, QClipboard::Clipboard);

	munmap(fbuf, n);
	fclose(tmp);
}/*}}}*/

//---------------------------------------------------------
//   pasteAt
//---------------------------------------------------------

int ComposerCanvas::pasteAt(const QString& pt, Track* track, unsigned int pos, bool clone, bool toTrack)/*{{{*/
{
	//printf("int ComposerCanvas::pasteAt(const QString& pt, Track* track, int pos)\n");
	QByteArray ba = pt.toLatin1();
	const char* ptxt = ba.constData();
	Xml xml(ptxt);
	bool firstPart = true;
	int posOffset = 0;
	//int  finalPos=0;
	unsigned int finalPos = pos;
	int notDone = 0;
	int done = 0;
	bool end = false;

	//song->startUndo();
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				end = true;
				break;
			case Xml::TagStart:
				if (tag == "part")
				{
					/*
					Part* p = 0;
					if(clone)
					{
					  if(!(p = readClone(xml, track, toTrack)))
						break;
					}
					else
					{
					  if (track->type() == Track::MIDI || track->type() == Track::DRUM)
						p = new MidiPart((MidiTrack*)track);
					  else if (track->type() == Track::WAVE)
						p = new WavePart((WaveTrack*)track);
					  else
						break;
					  p->read(xml, 0, toTrack);
					}
					 */

					// Read the part.
					Part* p = 0;
					p = readXmlPart(xml, track, clone, toTrack);
					// If it could not be created...
					if (!p)
					{
						// Increment the number of parts not done and break.
						++notDone;
						break;
					}

					// Increment the number of parts done.
					++done;

					if (firstPart)
					{
						firstPart = false;
						posOffset = pos - p->tick();
					}
					p->setTick(p->tick() + posOffset);
					if (p->tick() + p->lenTick() > finalPos)
					{
						finalPos = p->tick() + p->lenTick();
					}
					//pos += p->lenTick();
					audio->msgAddPart(p, false);
				}
				else
					xml.unknown("ComposerCanvas::pasteAt");
				break;
			case Xml::TagEnd:
				break;
			default:
				end = true;
				break;
		}
		if (end)
			break;
	}

	//song->endUndo(SC_PART_INSERTED);
	//return pos;

	if (notDone)
	{
		int tot = notDone + done;
		QMessageBox::critical(this, QString("OOStudio"),
				QString().setNum(notDone) + (tot > 1 ? (tr(" out of ") + QString().setNum(tot)) : QString("")) +
				(tot > 1 ? tr(" parts") : tr(" part")) +
				tr(" could not be pasted.\nLikely the selected track is the wrong type."));
	}

	return finalPos;
}/*}}}*/

//---------------------------------------------------------
//   paste
//    paste part to current selected track at cpos
//---------------------------------------------------------

void ComposerCanvas::paste(bool clone, bool toTrack, bool doInsert)/*{{{*/
{
	Track* track = 0;

	if (doInsert) // logic depends on keeping track of newly selected tracks
		deselectAll();


	// If we want to paste to a selected track...
	if (toTrack)
	{
	//This changes to song->visibletracks()
		TrackList* tl = song->visibletracks();
		for (iTrack i = tl->begin(); i != tl->end(); ++i)
		{
			if ((*i)->selected())
			{
				if (track)
				{
					QMessageBox::critical(this, QString("OOStudio"),
							tr("Cannot paste: multiple tracks selected"));
					return;
				}
				else
					track = *i;
			}
		}
		if (track == 0)
		{
			QMessageBox::critical(this, QString("OOStudio"),
					tr("Cannot paste: no track selected"));
			return;
		}
	}

	QClipboard* cb = QApplication::clipboard();
	const QMimeData* md = cb->mimeData(QClipboard::Clipboard);

	QString pfx("text/");
	QString mdpl("x-oom-midipartlist");
	QString wvpl("x-oom-wavepartlist");
	QString mxpl("x-oom-mixedpartlist");
	QString txt;

	if (md->hasFormat(pfx + mdpl))
	{
		// If we want to paste to a selected track...
		if (toTrack && !track->isMidiTrack())
		{
			QMessageBox::critical(this, QString("OOStudio"),
					tr("Can only paste to midi track"));
			return;
		}
		txt = cb->text(mdpl, QClipboard::Clipboard);
	}
	else if (md->hasFormat(pfx + wvpl))
	{
		// If we want to paste to a selected track...
		if (toTrack && track->type() != Track::WAVE)
		{
			QMessageBox::critical(this, QString("OOStudio"),
					tr("Can only paste to wave track"));
			return;
		}
		txt = cb->text(wvpl, QClipboard::Clipboard);
	}
	else if (md->hasFormat(pfx + mxpl))
	{
		// If we want to paste to a selected track...
		if (toTrack && !track->isMidiTrack() && track->type() != Track::WAVE)
		{
			QMessageBox::critical(this, QString("OOStudio"),
					tr("Can only paste to midi or wave track"));
			return;
		}
		txt = cb->text(mxpl, QClipboard::Clipboard);
	}
	else
	{
		QMessageBox::critical(this, QString("OOStudio"),
				tr("Cannot paste: wrong data type"));
		return;
	}

	int endPos = 0;
	unsigned int startPos = song->vcpos();
	if (!txt.isEmpty())
	{
		song->startUndo();
		endPos = pasteAt(txt, track, startPos, clone, toTrack);
		Pos p(endPos, true);
		song->setPos(0, p);
		if (!doInsert)
			song->endUndo(SC_PART_INSERTED);
	}

	if (doInsert)
	{
		int offset = endPos - startPos;
		movePartsTotheRight(startPos, offset);
		song->endUndo(SC_PART_INSERTED);
	}
}/*}}}*/

//---------------------------------------------------------
//   movePartsToTheRight
//---------------------------------------------------------

void ComposerCanvas::movePartsTotheRight(unsigned int startTicks, int length)/*{{{*/
{
	// all parts that start after the pasted parts will be moved the entire length of the pasted parts
        for (iCItem i = _items.begin(); i != _items.end(); ++i)
	{
		if (!i->second->isSelected())
		{
			Part* part = i->second->part();
			if (part->tick() >= startTicks)
			{
				//void Audio::msgChangePart(Part* oldPart, Part* newPart, bool doUndoFlag, bool doCtrls, bool doClones)
				Part *newPart = part->clone();
				newPart->setTick(newPart->tick() + length);
				if (part->track()->type() == Track::WAVE)
				{
					audio->msgChangePart((WavePart*) part, (WavePart*) newPart, false, false, false);
				}
				else
				{
					audio->msgChangePart(part, newPart, false, false, false);
				}

			}
		}
	}
	// perhaps ask if markers should be moved?
	MarkerList *markerlist = song->marker();
	for (iMarker i = markerlist->begin(); i != markerlist->end(); ++i)
	{
		Marker* m = &i->second;
		if (m->tick() >= startTicks)
		{
			Marker *oldMarker = new Marker();
			*oldMarker = *m;
			m->setTick(m->tick() + length);
			song->undoOp(UndoOp::ModifyMarker, oldMarker, m);
		}
	}
}/*}}}*/
//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void ComposerCanvas::startDrag(CItem* item, DragType t)/*{{{*/
{
	//printf("ComposerCanvas::startDrag(CItem* item, DragType t)\n");
	NPart* p = (NPart*) (item);
	Part* part = p->part();

	//---------------------------------------------------
	//    write part as XML into tmp file
	//---------------------------------------------------

	FILE* tmp = tmpfile();
	if (tmp == 0)
	{
		fprintf(stderr, "ComposerCanvas::startDrag() fopen failed: %s\n",
				strerror(errno));
		return;
	}
	Xml xml(tmp);
	int level = 0;
	part->write(level, xml);

	//---------------------------------------------------
	//    read tmp file into QTextDrag Object
	//---------------------------------------------------

	fflush(tmp);
	struct stat f_stat;
	if (fstat(fileno(tmp), &f_stat) == -1)
	{
		fprintf(stderr, "ComposerCanvas::startDrag fstat failed:<%s>\n",
				strerror(errno));
		fclose(tmp);
		return;
	}
	int n = f_stat.st_size + 1;
	char* fbuf = (char*) mmap(0, n, PROT_READ | PROT_WRITE,
			MAP_PRIVATE, fileno(tmp), 0);
	fbuf[n] = 0;

	QByteArray data(fbuf);
	QMimeData* md = new QMimeData();

	md->setData("text/x-oom-partlist", data);

	// "Note that setMimeData() assigns ownership of the QMimeData object to the QDrag object.
	//  The QDrag must be constructed on the heap with a parent QWidget to ensure that Qt can
	//  clean up after the drag and drop operation has been completed. "
	QDrag* drag = new QDrag(this);
	drag->setMimeData(md);

	if (t == MOVE_COPY || t == MOVE_CLONE)
		drag->exec(Qt::CopyAction);
	else
		drag->exec(Qt::MoveAction);

	munmap(fbuf, n);
	fclose(tmp);
}/*}}}*/

//---------------------------------------------------------
//   dragEnterEvent
//---------------------------------------------------------

void ComposerCanvas::dragEnterEvent(QDragEnterEvent* event)
{
	QString text;
	if (event->mimeData()->hasFormat("text/partlist"))
	{
		//qDebug("ComposerCanvas::dragEnterEvent: Found partList");
		event->acceptProposedAction();
	}
	else if (event->mimeData()->hasUrls())
	{
		text = event->mimeData()->urls()[0].path();

		if (text.endsWith(".wav", Qt::CaseInsensitive) ||
				text.endsWith(".ogg", Qt::CaseInsensitive) ||
				text.endsWith(".mpt", Qt::CaseInsensitive) ||
				text.endsWith(".oom", Qt::CaseInsensitive) ||
				text.endsWith(".mid", Qt::CaseInsensitive))
		{
			/*if(text.endsWith(".mpt", Qt::CaseInsensitive))
			{
			}
			else
			{
				SndFile* f = getWave(text, true);
				if(f)
				{
					int samples = f->samples();
					//
				}
			}*/
			//qDebug("ComposerCanvas::dragEnterEvent: Found Audio file");
			event->acceptProposedAction();
		}
		else
			event->ignore();
	}
	else
		event->ignore();
}

//---------------------------------------------------------
//   dragMoveEvent
//---------------------------------------------------------

void ComposerCanvas::dragMoveEvent(QDragMoveEvent* ev)
{
	//      printf("drag move %x\n", this);
	QPoint p = mapDev(ev->pos());
	int x = AL::sigmap.raster(p.x(), *_raster);
	if(x < 0)
		x = 0;
	emit timeChanged(x);
	//event->acceptProposedAction();
}

//---------------------------------------------------------
//   dragLeaveEvent
//---------------------------------------------------------

void ComposerCanvas::dragLeaveEvent(QDragLeaveEvent*)
{
	//      printf("drag leave\n");
	//event->acceptProposedAction();
}

//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void ComposerCanvas::viewDropEvent(QDropEvent* event)
{
	tracks = song->visibletracks();
	//printf("void ComposerCanvas::viewDropEvent(QDropEvent* event)\n");
	if (event->source() == this)
	{
		printf("local DROP\n");
		//event->ignore();
		return;
	}
	int type = 0; // 0 = unknown, 1 = partlist, 2 = uri-list
	QString text;

	if (event->mimeData()->hasFormat("text/partlist"))
		type = 1;
	else if (event->mimeData()->hasUrls())
		type = 2;
	else
	{
		if (debugMsg && event->mimeData()->formats().size() != 0)
			printf("Drop with unknown format. First format:<%s>\n", event->mimeData()->formats()[0].toLatin1().constData());
		event->ignore();
		return;
	}

	// Make a backup of the current clone list, to retain any 'copy' items,
	//  so that pasting works properly after.
	CloneList copyCloneList = cloneList;
	// Clear the clone list to prevent any dangerous associations with
	//  current non-original parts.
	cloneList.clear();

	if (type == 1)
	{
		text = QString(event->mimeData()->data("text/partlist"));

		int x = AL::sigmap.raster(event->pos().x(), *_raster);
		if (x < 0)
			x = 0;
		unsigned trackNo = y2pitch(event->pos().y());
		Track* track = 0;
		if (trackNo < tracks->size())
			track = tracks->index(trackNo);
		if (track)
		{
			song->startUndo();
			pasteAt(text, track, x);
			song->endUndo(SC_PART_INSERTED);
		}
	}
	else if (type == 2)
	{
		// TODO:Multiple support. Grab the first one for now.
		text = event->mimeData()->urls()[0].path();

		if (text.endsWith(".wav", Qt::CaseInsensitive) ||
				text.endsWith(".ogg", Qt::CaseInsensitive) ||
				text.endsWith(".mpt", Qt::CaseInsensitive))
		{
			int x = AL::sigmap.raster(event->pos().x(), *_raster);
			if (x < 0)
				x = 0;
			//qDebug("ComposerCanvas::dropEvent: x: %d cursor x: %d", x, AL::sigmap.raster(QCursor::pos().x(), *_raster));
			unsigned trackNo = y2pitch(event->pos().y());
			Track* track = 0;
			if (trackNo < tracks->size())
				track = tracks->index(trackNo);
			else
			{//Create the track
		    	event->acceptProposedAction();
				Track::TrackType t = Track::MIDI;
				if(text.endsWith(".wav", Qt::CaseInsensitive) || text.endsWith(".ogg", Qt::CaseInsensitive))
				{
					QFileInfo f(text);
					track = song->addTrackByName(f.baseName(), Track::WAVE, -1, true, true);
					song->updateTrackViews();
				}
				else
				{
					VirtualTrack* vt;
					CreateTrackDialog *ctdialog = new CreateTrackDialog(&vt, t, -1, this);
					ctdialog->lockType(true);
					if(ctdialog->exec() && vt)
					{
						TrackManager* tman = new TrackManager();
						qint64 nid = tman->addTrack(vt);
						track = song->findTrackById(nid);
					}
				}
			}
			
			if (track)
			{
				if (track->type() == Track::WAVE &&
						(text.endsWith(".wav", Qt::CaseInsensitive) ||
						(text.endsWith(".ogg", Qt::CaseInsensitive))))
				{
					unsigned tick = x;
					oom->importWaveToTrack(text, tick, track);
				}
				else if ((track->isMidiTrack() || track->type() == Track::WAVE) && text.endsWith(".mpt", Qt::CaseInsensitive))
				{//Who saves a wave part as anything but a wave file?
					unsigned tick = x;
					oom->importPartToTrack(text, tick, track);
				}
			}
		}
		else if (text.endsWith(".oom", Qt::CaseInsensitive))
		{
			emit dropSongFile(text);
		}
		else if (text.endsWith(".mid", Qt::CaseInsensitive))
		{
			emit dropMidiFile(text);
		}
		else
		{
			printf("dropped... something...  no hable...\n");
		}
	}

	// Restore backup of the clone list, to retain any 'copy' items,
	//  so that pasting works properly after.
	cloneList.clear();
	cloneList = copyCloneList;
}

//---------------------------------------------------------
//   drawCanvas
//---------------------------------------------------------

void ComposerCanvas::drawCanvas(QPainter& p, const QRect& rect)
{
	int x = rect.x();
	int y = rect.y();
	int w = rect.width();
	int h = rect.height();

	//////////
	// GRID //
	//////////
	QColor baseColor(config.partCanvasBg.light(104));
	p.setPen(baseColor);

	//--------------------------------
	// vertical lines
	//-------------------------------
	//printf("raster=%d\n", *_raster);
	if (config.canvasShowGrid)
	{
		int bar, beat;
		unsigned tick;

		AL::sigmap.tickValues(x, &bar, &beat, &tick);
		for (;;)
		{
			int xt = AL::sigmap.bar2tick(bar++, 0, 0);
			if (xt >= x + w)
				break;
			if (!((bar - 1) % 4))
				p.setPen(baseColor.dark(115));
			else
				p.setPen(baseColor);
			p.drawLine(xt, y, xt, y + h);

			// append
			int noDivisors = 0;
			if (*_raster == config.division * 2) // 1/2
				noDivisors = 2;
			else if (*_raster == config.division) // 1/4
				noDivisors = 4;
			else if (*_raster == config.division / 2) // 1/8
				noDivisors = 8;
			else if (*_raster == config.division / 4) // 1/16
				noDivisors = 16;
			else if (*_raster == config.division / 8) // 1/16
				noDivisors = 32;
			else if (*_raster == config.division / 16) // 1/16
				noDivisors = 64;

			int r = *_raster;
			int rr = rmapx(r);
			if (*_raster > 1)
			{
				while (rr < 4)
				{
					r *= 2;
					rr = rmapx(r);
					noDivisors = noDivisors / 2;
				}
				p.setPen(baseColor);
				for (int t = 1; t < noDivisors; t++)
					p.drawLine(xt + r * t, y, xt + r * t, y + h);
			}
		}
	}
	//--------------------------------
	// horizontal lines
	//--------------------------------

	//This changes to song->visibletracks()
	TrackList* tl = song->visibletracks();
	int yy = 0;
	int th;
	for (iTrack it = tl->begin(); it != tl->end(); ++it)
	{
		if (yy > y + h)
			break;
		Track* track = *it;
		th = track->height();
		if (config.canvasShowGrid)
		{
			//printf("ComposerCanvas::drawCanvas track name:%s, y:%d h:%d\n", track->name().toLatin1().constData(), yy, th);
			p.setPen(baseColor.dark(130));
			p.drawLine(x, yy + th, x + w, yy + th); 
			p.setPen(baseColor);
		}
		yy += track->height();
	}
	
	//If this is the first run build the icons list
	if(build_icons)
	{
		partColorIcons.clear();
		partColorIconsSelected.clear();
		for (int i = 0; i < NUM_PARTCOLORS; ++i)
		{
			partColorIcons.append(colorRect(config.partColors[i], config.partWaveColors[i], 80, 80));
			partColorIconsSelected.append(colorRect(config.partWaveColors[i], config.partColors[i], 80, 80));
		}
		build_icons = false;
	}
}

void ComposerCanvas::drawTopItem(QPainter& p, const QRect& rect)
{
	int x = rect.x();
	int y = rect.y();
	int w = rect.width();
	int h = rect.height();

	QColor baseColor(config.partCanvasBg.light(104));
	p.setPen(baseColor);

	//This changes to song->visibletracks()
	TrackList* tl = song->visibletracks();

	show_tip = true;
	int yy = 0;
	int th;
	for (iTrack it = tl->begin(); it != tl->end(); ++it)
    {
		if (yy > y + h)
			break;
		Track* track = *it;
		th = track->height();
        
        // draw automation
		if (track->isMidiTrack())
        {
			Track *input = track->inputTrack();
			if(input)
			{
				QRect r = rect & QRect(x, yy, w, track->height());
				drawAutomation(p, r, (AudioTrack*)input, track);
				p.setPen(baseColor);
			}
            if (track->wantsAutomation())
            {
                AudioTrack* atrack = ((MidiTrack*)track)->getAutomationTrack();
                if (atrack)
                {
                    QRect r = rect & QRect(x, yy, w, track->height());
                    drawAutomation(p, r, atrack, track);
                    p.setPen(baseColor);
                }
            }
        }
        else
        {
            QRect r = rect & QRect(x, yy, w, track->height());
            drawAutomation(p, r, (AudioTrack*)track);
            p.setPen(baseColor);
        }
        yy += track->height();
    }


	QRect rr = p.worldMatrix().mapRect(rect);

	unsigned int startPos = audio->getStartRecordPos().tick();
    if (song->punchin())
		startPos = song->lpos();
    int start = mapx(startPos);
    int ww = mapx(song->cpos()) - mapx(startPos);

	if (song->cpos() < startPos)
        return;

	if (song->punchout() && song->cpos() > song->rpos())
		return;

	p.save();
	p.resetTransform();
	//Draw part as recorded
	if (song->record() && audio->isPlaying())
	{
		for (iTrack it = tl->begin(); it != tl->end(); ++it)
		{
			Track* track = *it;
			if (track && track->recordFlag())
			{
				int mypos = track2Y(track)-ypos;
				p.fillRect(start, mypos, ww, track->height(), config.partColors[track->getDefaultPartColor()]);
				p.setPen(Qt::black); //TODO: Fix colors
				p.drawLine(start, mypos, start+ww, mypos);
				p.drawLine(start, mypos+1, start+ww, mypos+1);
				p.drawLine(start, mypos+track->height(), start+ww, mypos+track->height());
				p.drawLine(start, mypos+track->height()-1, start+ww, mypos+track->height()-1);
			}
		}
	}
	p.restore();
	
	//Draw notes they are recorded
	if (song->record() && audio->isPlaying())
	{
		for (iTrack it = tl->begin(); it != tl->end(); ++it)
		{
			Track* track = *it;

			if (track->isMidiTrack() && track->recordFlag())
			{
				MidiTrack *mt = (MidiTrack*)track;
				int mypos = track2Y(track);
				QRect partRect(startPos, mypos, song->cpos()-startPos, track->height());
				EventList newEventList;
				MPEventList *el = mt->mpevents();
				if (!el->size())
					continue;

				for (iMPEvent i = el->begin(); i != el->end(); ++i)
				{
					MidiPlayEvent pe = *i;

					if (pe.isNote() && !pe.isNoteOff()) {
						Event e(Note);
						e.setPitch(pe.dataA());
						e.setTick(pe.time()-startPos);
						e.setLenTick(song->cpos()-pe.time());
						e.setC(1);
						newEventList.add(e);
					}
					else if (pe.isNoteOff())
					{
						for (iEvent i = newEventList.begin(); i != newEventList.end(); ++i)
						{
							Event &e = i->second;
							if (e.pitch() == pe.dataA() && e.dataC() == 1)
							{
								e.setLenTick(pe.time() - e.tick()- startPos);
								e.setC(0);
								continue;
							}
						}
					}
				}
				QColor c(0,0,0);
				drawMidiPart(p, rect, &newEventList, mt, partRect, startPos, 0, (song->cpos() - startPos), c);
			}
		}
    }
}

//---------------------------------------------------------
//   drawAudioTrack
//---------------------------------------------------------

void ComposerCanvas::drawAudioTrack(QPainter& p, const QRect& r, AudioTrack* /* t */)
{
	// NOTE: For one-pixel border use first line and don't bother with setCosmetic.
	//       For a two-pixel border use second line and MUST use setCosmetic! Tim.
	QPen pen(Qt::black, 0, Qt::SolidLine);
	//p.setPen(QPen(Qt::black, 2, Qt::SolidLine));
	//pen.setCosmetic(true);
	p.setPen(pen);
	//p.setBrush(Qt::gray);
	QColor c(Qt::gray);
	c.setAlpha(config.globalAlphaBlend);
	p.setBrush(c);

	// Factor in pen stroking size:
	//QRect rr(r);
	//rr.setHeight(rr.height() -1);

	p.drawRect(r);
}

double ComposerCanvas::getControlValue(CtrlList *cl, double val)
{
	double prevVal = val;
	//FIXME: this check needs to be more robust to deal with plugin automation
	//that have different type ie: sec, milisec, also db which is not covered by our controllers
	if (cl->id() == AC_VOLUME)/*{{{*/
	{ // use db scale for volume
		//printf("volume cvval=%f\n", cvFirst.val);
		prevVal = dbToVal(val); // represent volume between 0 and 1
		if (prevVal < 0) prevVal = 0.0;
	}
	else
	{
		double min, max;
		cl->range(&min, &max);
		prevVal = (prevVal - min) / (max - min);
	}/*}}}*/
	return prevVal;
}

//---------------------------------------------------------
//   drawAutomation
//---------------------------------------------------------

void ComposerCanvas::drawAutomation(QPainter& p, const QRect& r, AudioTrack *t, Track *rt)/*{{{*/
{
	//printf("ComposerCanvas::drawAutomation\n");
	QRect tempRect = r;
    if (!rt) rt = t;
	tempRect.setBottom(track2Y(rt) + rt->height());
	QRect rr = p.worldMatrix().mapRect(tempRect);

	p.save();
	p.resetTransform();

	int height = rt->height() - 4; // limit height
	bool paintdBLines = false;
	bool paintLines = false;
	bool paintdBText = true;
	bool paintTextAsDb = false;

	CtrlListList* cll = t->controller();

	// set those when there is a node 'lazy selected', then the
	// dB indicator will be painted on top of the curve, not below it
	int lazySelectedNodeFrame = -1;
	double lazySelectedNodePrevVal;
	double lazySelectedNodeVal;

	bool firstRun = true;
	for (CtrlListList::iterator icll = cll->begin(); icll != cll->end(); ++icll)
	{
		QPolygonF automationLine;
		CtrlList *cl = icll->second;

		if (cl->dontShow())
		{
			continue;
		}
		
		//printf("Controller - Name: %s ID: %d Visible: %d\n", cl->name().toLatin1().constData(), cl->id(), cl->isVisible());
		if (!cl->isVisible())
			continue; // skip this iteration if this controller isn't in the visible list

		//printf("4444444444444444444444444444444444444444444444444444\n");
		QColor curveColor = cl->color();
		if (!cl->selected() || _tool != AutomationTool) {
			curveColor.setAlpha(100);
		}
		/*if(_tool == AutomationTool && automation.currentCtrlList == cl)
		{
			curveColor = cl->color();
		}*/
		p.setPen(QPen(curveColor, 2, Qt::SolidLine));
		p.setRenderHint(QPainter::Antialiasing, true);

		double prevVal;
		iCtrl ic = cl->begin();
		// First check that there ARE automation, ic == cl->end means no automation
		if (ic != cl->end())
		{
			CtrlVal cvFirst = ic->second;
			int prevPosFrame = cvFirst.getFrame();
			prevVal = cvFirst.val;
			//TODO:Write out automation as its being recorded
			/*CtrlRecList::iterator recIter;
			if(audio->isPlaying())
			{
				CtrlRecList* recList = t->recEvents();
				recIter = recList->find(precPosFrame);
			}*/

			// prepare prevVal
			if (cl->id() == AC_VOLUME)/*{{{*/
			{ // use db scale for volume
//				printf("volume cvval=%f\n", cvFirst.val);
				prevVal = dbToVal(cvFirst.val); // represent volume between 0 and 1
				if (prevVal < 0) prevVal = 0.0;
				//paintdBText = true;
				paintTextAsDb = true;
				paintLines = true;
				paintdBLines = true;
			}
			else
			{
				if(cl->id() == AC_PAN)
				{
					//paintdBText = true;
					paintTextAsDb = false;
					paintLines = true;
				}
				paintTextAsDb = false;
				// we need to set curVal between 0 and 1
				double min, max;
				cl->range(&min, &max);
				prevVal = (prevVal - min) / (max - min);
			}/*}}}*/

			// draw a square around the point
			p.drawRect(mapx(tempomap.frame2tick(prevPosFrame))-1, (rr.bottom()-2)-prevVal*height-1, 3, 3);


			//printf("list size: %d\n", (int)cl->size());
			for (; ic != cl->end(); ++ic)
			{
				CtrlVal &cv = ic->second;
				double nextVal = cv.val; // was curVal
				if (cl->id() == AC_VOLUME)
				{ // use db scale for volume
					nextVal = dbToVal(cv.val); // represent volume between 0 and 1
					if (nextVal < 0) nextVal = 0.0;
				}
				else
				{
					// we need to set curVal between 0 and 1
					double min, max;
					cl->range(&min, &max);
					nextVal = (nextVal - min) / (max - min);
				}
				int leftX = mapx(tempomap.frame2tick(prevPosFrame));
				if (firstRun && leftX>rr.x()) {
					leftX=rr.x();
				}

				int currentPixel = mapx(tempomap.frame2tick(cv.getFrame()));
				p.setPen(QPen(curveColor, 2, Qt::SolidLine));
				
				//This draws the connecting lines between nodes
				//p.drawLine(leftX,(rr.bottom()-2)-prevVal*height,currentPixel,(rr.bottom()-2)-nextVal*height);
				
				automationLine.append(QPointF(currentPixel,(rr.bottom()-2)-nextVal*height));
				
				firstRun = false;
				//printf("draw line: %d %f %d %f\n",tempomap.frame2tick(lastPos),rr.bottom()-lastVal*height,tempomap.frame2tick(cv.frame),rr.bottom()-curVal*height);
				prevPosFrame = cv.getFrame();
				prevVal = nextVal;
				if (currentPixel < rr.x()+ rr.width())
				{
					//printf("pppppppppuuuuuuuuuuuuuuuuuuuuuuuuuuuuuukkkkkkkkkkkkkkkkkkkeeeeeeeeeeeeeeeee\n");
					//goto quitDrawing;

					bool ctrListNodesMoving = false;
					if (automation.currentCtrlList == cl && automation.moveController) {
						ctrListNodesMoving = true;
					}

					bool paintNodeHighlighted = false;
					if (ctrListNodesMoving) {
						if ((automation.currentCtrlVal && *automation.currentCtrlVal == cv) || _curveNodeSelection->isSelected(&cv))
						{
							paintNodeHighlighted = true;
						}
					}
					else if ((automation.currentCtrlVal && *automation.currentCtrlVal == cv) || _curveNodeSelection->isSelected(&cv))
					{
						paintNodeHighlighted = true;
					}

					// draw a square around the point
					// If the point is selected, paint it with a different color to make the selected one more visible to the user
					if (paintNodeHighlighted)
					{
						lazySelectedNodePrevVal = prevVal;
						lazySelectedNodeFrame = prevPosFrame;
						lazySelectedNodeVal = cv.val;

						QPen pen2(QColor(131,254,255), 3);
						p.setPen(pen2);
						QBrush brush(QColor(1,41,59));
						p.setBrush(brush);

						p.drawEllipse(mapx(tempomap.frame2tick(lazySelectedNodeFrame)) - 5, (rr.bottom()-2)-lazySelectedNodePrevVal*height - 5, 8, 8);

						if ((lazySelectedNodeFrame >= 0) && paintdBText)
						{
							if (automation.currentCtrlVal && *automation.currentCtrlVal == cv)
							{
								drawTooltipText(p, 
												rr,
												height,
												lazySelectedNodeVal,
												lazySelectedNodePrevVal,
												lazySelectedNodeFrame,
												false, //use painting to show tip
												cl);
								show_tip = false;
							}
							else
								show_tip = true;
						}
					}
					else
					{
						p.setBrush(Qt::NoBrush);
						p.drawRect(mapx(tempomap.frame2tick(prevPosFrame))-2, (rr.bottom()-2)-prevVal*height-2, 4, 4);
					}
				}	

				//paint the lines
				/*if (paintdBLines)
				{
					printf("222222222222222222222222222222222222\n");
					drawPositionLines(p, rr, height, paintTextAsDb);
				}*/
				//paint the text
			}
			//printf("outer draw %f\n", cvFirst.val );
			
			//This next line draw is drawing the end line
			//p.drawLine(mapx(tempomap.frame2tick(prevPosFrame)),(rr.bottom()-2)-prevVal*height,rr.x()+rr.width(),(rr.bottom()-2)-prevVal*height);
			automationLine.append(QPointF(rr.x()+rr.width(),(rr.bottom()-2)-prevVal*height));
			//printf("draw last line: %d %f %d %f\n",tempomap.frame2tick(prevPosFrame),(rr.bottom()-2)-prevVal*height,tempomap.frame2tick(prevPosFrame)+rr.width(),(rr.bottom()-2)-prevVal*height);
		}//END if(ci = end())
		p.setPen(QPen(curveColor, 2, Qt::SolidLine));
		QPainterPath automationPath;
		automationPath.addPolygon(automationLine);
		p.setBrush(Qt::NoBrush);
		//Set the CtrlList curvePath for use in selection
		cl->setCurvePath(automationPath);
		p.drawPath(automationPath);
	} //END first for loop

	
	if (paintLines)
	{
		
		double zerodBHeight = rr.bottom() - dbToVal(1.0) * height;
		double minusTwelvedBHeight = rr.bottom() - dbToVal(0.25) * height;
		double panZero = rr.bottom() - height/2;

		// line color
		p.setRenderHint(QPainter::Antialiasing, false);
		p.setPen(QColor(255, 255, 255, 60));
		p.setFont(QFont("fixed-width", 8, QFont::Bold));
		//p.setPen(QColor(255, 0, 0, 255));
		if(paintdBLines)
		{
			p.drawLine(0, zerodBHeight, width(), zerodBHeight);
			p.drawLine(0, minusTwelvedBHeight, width(), minusTwelvedBHeight);
			if(height >= 180)
			{
				//p.drawText(rr.left() + 5, zerodBHeight - 4, "  0 dB");
				p.drawText(5, zerodBHeight - 4, "  0 dB");
				p.drawText(5, minusTwelvedBHeight - 4, "-12 dB");
			}	
		}
		if(!paintTextAsDb)
			p.drawLine(0, panZero, width(), panZero);

	}

	p.restore();
}/*}}}*/

void ComposerCanvas::drawTooltipText(QPainter& p, /*{{{*/
		const QRect& rr, 
		int height,
		double lazySelNodeVal,
		double lazySelNodePrevVal,
		int lazySelNodeFrame,
		bool useTooltip,
		CtrlList* cl)
{
	// calculate the dB value for the dB string.
	double vol = lazySelNodeVal;/*{{{*/
	QString dbString;
	//if(paintTextAsDb)
	if(cl && cl->id() == AC_VOLUME)
	{
		if (vol < 0.0001f)
		{
			vol = 0.0001f;
		}
		vol = 20.0f * log10 (vol);
		if(vol < -60.0f)
			vol = -60.0f;
		 dbString += QString::number (vol, 'f', 2) + " dB";
	}
	else
	{
		dbString += QString::number(vol, 'f', 2);
	}
    if(cl->unit().isEmpty() == false)
        dbString.append(" "+cl->unit());
	if(cl->pluginName().isEmpty())
		dbString.append("  "+cl->name());
	else
		dbString.append("  "+cl->name()).append(" : ").append(cl->pluginName());/*}}}*/
	// Set the color for the dB text
	if(!useTooltip)
	{
		p.setPen(QColor(255,255,255,190));
	//p.drawText(mapx(tempomap.frame2tick(lazySelNodeFrame)) + 15, (rr.bottom()-2)-lazySelNodePrevVal*height, dbString);
		int top = (rr.bottom()-20)-lazySelNodePrevVal*height;
		if(top < 0)
			top = 0;
		p.setFont(QFont("fixed-width", 8, QFont::Bold));
		p.drawText(QRect(mapx(tempomap.frame2tick(lazySelNodeFrame)) + 10, top, 400, 60), Qt::TextWordWrap|Qt::AlignLeft, dbString);
	}
	else
	{
		QPoint cursorPos = QCursor::pos();
		QToolTip::showText(cursorPos, dbString, this, QRect(cursorPos.x(), cursorPos.y(), 2, 2));
	}
	//p.drawText(QRect(cursorPos.x() + 10, cursorPos.y(), 400, 60), Qt::TextWordWrap|Qt::AlignLeft, dbString);
}/*}}}*/

//---------------------------------------------------------
//  checkAutomation
//    compares the current mouse pointer with the automation
//    lines on the track under it.
//    if there is a controller to be moved it is marked
//    in the automation object
//    if addNewCtrl is set and a valid line is found the
//    automation object will also be set but no
//    controller added.
//---------------------------------------------------------

void ComposerCanvas::checkAutomation(Track * t, const QPoint &pointer, bool addNewCtrl)/*{{{*/
{
	//int circumference = 5;
	Track* inputTrack = 0;
	if (t->isMidiTrack())
	{
		bool found = false;
		inputTrack = t->inputTrack();
		if(inputTrack)
		{
			found = checkAutomationForTrack(inputTrack, pointer, addNewCtrl, t);
		}
    	if (!found && t->wantsAutomation())
    	{
    	    AudioTrack* atrack = ((MidiTrack*)t)->getAutomationTrack();
    	    if (!atrack)
    	        return;
			checkAutomationForTrack(atrack, pointer, addNewCtrl, t);
    	}
	}
	else
	{
		checkAutomationForTrack(t, pointer, addNewCtrl);
	}

	//printf("checkAutomation p.x()=%d p.y()=%d\n", mapx(pointer.x()), mapx(pointer.y()));

	//int currX =  mapx(pointer.x());
	//int currY =  mapy(pointer.y());

	//CtrlListList* cll;
    //if (t->isMidiTrack() && t->wantsAutomation())
   //{
       // AudioTrack* atrack = ((MidiTrack*)t)->getAutomationTrack();
       // if (!atrack)
      //      return;
		
        //cll = atrack->controller();
    //}
    //else
        //cll = ((AudioTrack*) t)->controller();

/*
	for(CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll)
	{
		//iCtrlList *icl = icll->second;
		CtrlList *cl = icll->second;
		if (cl->dontShow() || !cl->isVisible()) {
			continue;
		}
		iCtrl ic=cl->begin();

		int oldX=-1;
		int oldY=-1;
		int ypixel;
		int xpixel;

		// First check that there ARE automation, ic == cl->end means no automation
		if (ic != cl->end()) {
			for (; ic !=cl->end(); ic++)
			{
				CtrlVal &cv = ic->second;
				double y;
				if (cl->id() == AC_VOLUME ) { // use db scale for volume
					y = dbToVal(cv.val); // represent volume between 0 and 1
					if (y < 0) y = 0;
				}
				else {
					// we need to set curVal between 0 and 1
					double min, max;
					cl->range(&min,&max);
					y = ( cv.val - min)/(max-min);
				}


				int yy = track2Y(t) + t->height();

				ypixel = mapy(yy-2-y*t->height());
				xpixel = mapx(tempomap.frame2tick(cv.getFrame()));

				if (oldX==-1) oldX = xpixel;
				if (oldY==-1) oldY = ypixel;

				bool foundIt=false;
				if (addNewCtrl) {
					foundIt=true;
				}

				//printf("point at x=%d xdiff=%d y=%d ydiff=%d\n", mapx(tempomap.frame2tick(cv.frame)), x1, mapx(ypixel), y1);
				oldX = xpixel;
				oldY = ypixel;

				int x1 = abs(currX - xpixel) ;
				int y1 = abs(currY - ypixel);
				if (!addNewCtrl && x1 < circumference &&  y1 < circumference && pointer.x() > 0 && pointer.y() > 0) {
					foundIt=true;
				}

				if (foundIt) {
					automation.currentCtrlList = cl;
					automation.currentTrack = t;
					if (addNewCtrl) {
						automation.currentCtrlVal = 0;
						redraw();
						automation.controllerState = addNewController;
					}else {
						automation.currentCtrlVal=&cv;
						redraw();
						automation.controllerState = movingController;
					}
					return;
				}
				
			}
		} // if

		if (addNewCtrl)
		{
			//FIXME This is faulty logic as you can now select or add a node anywhere on the line
			// check if we are reasonably close to a line, we only need to check Y
			// as the line is straight after the last controller
			bool foundIt=false;
			if ( ypixel == oldY && abs(currY-ypixel) < circumference) {
				foundIt=true;
			}

			if (foundIt)
			{
				automation.controllerState = addNewController;
				automation.currentCtrlList = cl;
				automation.currentTrack = t;
				automation.currentCtrlVal = 0;
				return;
			}
		}
	}
	// if there are no hits we default to clearing all the data
	automation.controllerState = doNothing;
	if (automation.currentCtrlVal)
	{
		automation.currentCtrlVal = 0;
		redraw();
	}

	setCursor();
*/
}/*}}}*/

bool ComposerCanvas::checkAutomationForTrack(Track * t, const QPoint &pointer, bool addNewCtrl, Track *rt)/*{{{*/
{
	bool rv = false;
	int circumference = 5;

	//printf("checkAutomation p.x()=%d p.y()=%d\n", mapx(pointer.x()), mapx(pointer.y()));

	if(!rt)
	{
		rt = t;
	}
	int currX =  mapx(pointer.x());
	int currY =  mapy(pointer.y());

	CtrlListList* cll;
    if (t->isMidiTrack() && t->wantsAutomation())
    {
        AudioTrack* atrack = ((MidiTrack*)t)->getAutomationTrack();
        if (!atrack)
            return false;
        cll = atrack->controller();
    }
    else
        cll = ((AudioTrack*) t)->controller();

	for(CtrlListList::iterator icll =cll->begin();icll!=cll->end();++icll)
	{
		//iCtrlList *icl = icll->second;
		CtrlList *cl = icll->second;
		if (cl->dontShow() || !cl->isVisible()) {
			continue;
		}
		iCtrl ic=cl->begin();

		int oldX=-1;
		int oldY=-1;
		int ypixel;
		int xpixel;

		// First check that there ARE automation, ic == cl->end means no automation
		if (ic != cl->end()) {
			for (; ic !=cl->end(); ic++)/*{{{*/
			{
				CtrlVal &cv = ic->second;
				double y;
				if (cl->id() == AC_VOLUME ) { // use db scale for volume
					y = dbToVal(cv.val); // represent volume between 0 and 1
					if (y < 0) y = 0;
				}
				else {
					// we need to set curVal between 0 and 1
					double min, max;
					cl->range(&min,&max);
					y = ( cv.val - min)/(max-min);
				}


				int yy = track2Y(rt) + rt->height();

				ypixel = mapy(yy-2-y*rt->height());
				xpixel = mapx(tempomap.frame2tick(cv.getFrame()));

				if (oldX==-1) oldX = xpixel;
				if (oldY==-1) oldY = ypixel;

				bool foundIt=false;
				if (addNewCtrl) {
					foundIt=true;
				}

				//printf("point at x=%d xdiff=%d y=%d ydiff=%d\n", mapx(tempomap.frame2tick(cv.frame)), x1, mapx(ypixel), y1);
				oldX = xpixel;
				oldY = ypixel;

				int x1 = abs(currX - xpixel) ;
				int y1 = abs(currY - ypixel);
				if (!addNewCtrl && x1 < circumference &&  y1 < circumference && pointer.x() > 0 && pointer.y() > 0) {
					foundIt=true;
				}

				if (foundIt) {
					automation.currentCtrlList = cl;
					automation.currentTrack = rt;
					if (addNewCtrl) {
						automation.currentCtrlVal = 0;
						redraw();
						automation.controllerState = addNewController;
					}else {
						automation.currentCtrlVal=&cv;
						redraw();
						automation.controllerState = movingController;
					}
					return true;
				}
				
			}/*}}}*/
		} // if

		if (addNewCtrl)
		{
			//FIXME This is faulty logic as you can now select or add a node anywhere on the line
			// check if we are reasonably close to a line, we only need to check Y
			// as the line is straight after the last controller
			bool foundIt=false;
			if ( ypixel == oldY && abs(currY-ypixel) < circumference) {
				foundIt=true;
			}

			if (foundIt)
			{
				automation.controllerState = addNewController;
				automation.currentCtrlList = cl;
				automation.currentTrack = rt;
				automation.currentCtrlVal = 0;
				return true;
			}
		}
	}
	// if there are no hits we default to clearing all the data
	automation.controllerState = doNothing;
	if (automation.currentCtrlVal)
	{
		automation.currentCtrlVal = 0;
		redraw();
	}

	setCursor();
	return rv;
}/*}}}*/

void ComposerCanvas::selectAutomation(Track * t, const QPoint &pointer)/*{{{*/
{
	Track* inputTrack = 0;/*{{{*/
	if (t->isMidiTrack())
	{
		bool found = false;
		inputTrack = t->inputTrack();
		if(inputTrack)
		{
			found = selectAutomationForTrack(inputTrack, pointer, t);
		}
    	if (!found && t->wantsAutomation())
    	{
    	    AudioTrack* atrack = ((MidiTrack*)t)->getAutomationTrack();
    	    if (!atrack)
    	        return;
			selectAutomationForTrack(t, pointer);
    	}
	}
	else
	{
		selectAutomationForTrack(t, pointer);
	}/*}}}*/
	//printf("selectAutomation p.x()=%d p.y()=%d\n", mapx(pointer.x()), mapx(pointer.y()));

	/*int currY = mapy(pointer.y());
	int currX = mapx(pointer.x());
	QRect clickPoint(currX-5, currY-5, 10, 10);

	bool foundIt = false;
	CtrlListList* cll;
    if (t->isMidiTrack())
    {
        AudioTrack* atrack = ((MidiTrack*)t)->getAutomationTrack();
        if (!atrack)
            return;
        cll = atrack->controller();
    }
    else
        cll = ((AudioTrack*) t)->controller();

	cll->deselectAll();
	for(CtrlListList::iterator icll = cll->begin(); icll != cll->end(); ++icll)
	{
		CtrlList *cl = icll->second;
		if (cl->dontShow() || !cl->isVisible()) {
			continue;
		}
		QPainterPath cpath = cl->curvePath();
		if(cpath.isEmpty())
			continue;

		// Check if the mouse click intersecs an automation line
		// NOTE: QT uses a fill pattern to deturmin if the intersection is valid.
		// this calculation starts to break down for us when automation lines start 
		// to cross each other. This is so far the easiest selection we have had but 
		// try not to crown your automation lanes with lots of overlapping lines.
		// Maybe we can implement our own class to solve this issue one day. - Andrew
		if (cpath.intersects(clickPoint))
		{
			foundIt=true;
		}

		if(foundIt && automation.currentCtrlList && automation.currentCtrlList == cl)
		{//Do nothing
			automation.currentCtrlList->setSelected(true);
			break;
		}
		else if(foundIt)
		{
			if(automation.currentCtrlList && automation.currentCtrlList != cl)
				automation.currentCtrlList->setSelected(false);
			//printf("Selecting closest automation line\n");
			automation.controllerState = doNothing;
			automation.currentCtrlList = cl;
			automation.currentCtrlList->setSelected(true);
			automation.currentTrack = t;
			automation.currentCtrlVal = 0;
			redraw();
			break;
		}
	}
	// if there are no hits we default to clearing all the data
	if(!foundIt)
	{
		automation.controllerState = doNothing;
		if (automation.currentCtrlVal)
		{
			automation.currentCtrlVal = 0;
			redraw();
		}
	}

	setCursor();*/
}/*}}}*/

bool ComposerCanvas::selectAutomationForTrack(Track * t, const QPoint &pointer, Track* rt)/*{{{*/
{
	bool rv = false;
	if(!rt)
	{
		rt = t;
	}

	//printf("selectAutomation p.x()=%d p.y()=%d\n", mapx(pointer.x()), mapx(pointer.y()));

	int currY = mapy(pointer.y());
	int currX = mapx(pointer.x());
	QRect clickPoint(currX-5, currY-5, 10, 10);

	bool foundIt = false;
	CtrlListList* cll;
    if (t->isMidiTrack())
    {
        AudioTrack* atrack = ((MidiTrack*)t)->getAutomationTrack();
        if (!atrack)
            return false;
        cll = atrack->controller();
    }
    else
        cll = ((AudioTrack*) t)->controller();

	cll->deselectAll();
	for(CtrlListList::iterator icll = cll->begin(); icll != cll->end(); ++icll)
	{
		CtrlList *cl = icll->second;
		if (cl->dontShow() || !cl->isVisible()) {
			continue;
		}
		QPainterPath cpath = cl->curvePath();
		if(cpath.isEmpty())
			continue;

		// Check if the mouse click intersecs an automation line
		// NOTE: QT uses a fill pattern to deturmin if the intersection is valid.
		// this calculation starts to break down for us when automation lines start 
		// to cross each other. This is so far the easiest selection we have had but 
		// try not to crown your automation lanes with lots of overlapping lines.
		// Maybe we can implement our own class to solve this issue one day. - Andrew
		if (cpath.intersects(clickPoint))
		{
			foundIt=true;
		}

		if(foundIt && automation.currentCtrlList && automation.currentCtrlList == cl)
		{//Do nothing
			automation.currentCtrlList->setSelected(true);
			break;
		}
		else if(foundIt)
		{
			rv = true;
			if(automation.currentCtrlList && automation.currentCtrlList != cl)
				automation.currentCtrlList->setSelected(false);
			//printf("Selecting closest automation line\n");
			automation.controllerState = doNothing;
			automation.currentCtrlList = cl;
			automation.currentCtrlList->setSelected(true);
			automation.currentTrack = t;
			automation.currentCtrlVal = 0;
			redraw();
			break;
		}
	}
	// if there are no hits we default to clearing all the data
	if(!foundIt)
	{
		rv = false;
		automation.controllerState = doNothing;
		if (automation.currentCtrlVal)
		{
			automation.currentCtrlVal = 0;
			redraw();
		}
	}

	setCursor();
	return rv;
}/*}}}*/


void ComposerCanvas::controllerChanged(Track* /* t */)
{
	redraw();
}

void ComposerCanvas::addNewAutomation(QMouseEvent *event)
{
	//printf("ComposerCanvas::addNewAutomation(%p)\n", event);
	Track * t = y2Track(event->pos().y());
	if (t) 
	{
		checkAutomation(t, event->pos(), true);

		automation.moveController = true;
		processAutomationMovements(event);
	}
}

void ComposerCanvas::processAutomationMovements(QMouseEvent *event)
{

	if (_tool != AutomationTool)
	{
		return;
	}

	if (!automation.moveController)  // currently nothing going lets's check for some action.
	{
		Track * t = y2Track(event->pos().y());
		if (t) {
			bool addNewPoints = false;
			if (event->modifiers() & Qt::ControlModifier)
			{
				addNewPoints = true;
			}
			checkAutomation(t, event->pos(), addNewPoints);
		}
		return;
	}

	if (!automation.currentCtrlList)
	{
		return;
	}

	// automation.moveController is set, lets rock.

	int currFrame = tempomap.tick2frame(event->pos().x());
	double min, max;

	if (automation.controllerState == addNewController)
	{
		bool addNode = false;
		if(automation.currentCtrlList->selected())
		{
			addNode = true;
		}
		else
		{ //go find the right item from the current track
			Track * t = y2Track(event->pos().y());
			if (t) 
			{
				CtrlListList* cll;
				if(t->isMidiTrack())
				{
					bool found = false;
					Track* input = t->inputTrack();
					if(input)
					{
                	    cll = ((AudioTrack*) input)->controller();/*{{{*/
						for(CtrlListList::iterator icll = cll->begin(); icll != cll->end(); ++icll)
						{
							CtrlList *cl = icll->second;
							if(cl && cl->selected() && cl != automation.currentCtrlList)
							{
								found = true;
								automation.currentCtrlList = cl;
								break;
							}
						}
						if(found && automation.currentCtrlList->selected())
						{
							addNode = true;
						}/*}}}*/
					}
                	if (!found && t->wantsAutomation())
					{
                	    cll = ((MidiTrack*)t)->getAutomationTrack()->controller();
						for(CtrlListList::iterator icll = cll->begin(); icll != cll->end(); ++icll)
						{
							CtrlList *cl = icll->second;
							if(cl && cl->selected() && cl != automation.currentCtrlList)
							{
								automation.currentCtrlList = cl;
								break;
							}
						}
						if(automation.currentCtrlList->selected())
						{
							addNode = true;
						}
					}
				}
				else
				{
                	cll = ((AudioTrack*) t)->controller();
					for(CtrlListList::iterator icll = cll->begin(); icll != cll->end(); ++icll)
					{
						CtrlList *cl = icll->second;
						if(cl && cl->selected() && cl != automation.currentCtrlList)
						{
							automation.currentCtrlList = cl;
							break;
						}
					}
					if(automation.currentCtrlList->selected())
					{
						addNode = true;
					}
				}
			}
		}

		if (addNode)
		{
			automation.currentCtrlList->range(&min,&max);
			double range = max - min;
			double newValue;
			double relativeY = double(event->pos().y() - track2Y(automation.currentTrack)) / automation.currentTrack->height();

			if (automation.currentCtrlList->id() == AC_VOLUME )
			{
				newValue = dbToVal(max) - relativeY;
				newValue = valToDb(newValue);
			}
			else
			{
				newValue = max - (relativeY * range);
			}

			AddRemoveCtrlValues* cmd = new AddRemoveCtrlValues(automation.currentCtrlList, CtrlVal(currFrame, newValue), OOMCommand::ADD);
			CommandGroup* group = new CommandGroup(tr("Add Automation Node"));
			group->add_command(cmd);
			song->pushToHistoryStack(group);
		}

		QWidget::setCursor(Qt::BlankCursor);

		iCtrl ic=automation.currentCtrlList->begin();
		for (; ic !=automation.currentCtrlList->end(); ic++) {
			CtrlVal &cv = ic->second;
			if (cv.getFrame() == currFrame) {
				automation.currentCtrlVal = &cv;
				automation.controllerState = movingController;
				//printf("Adding new node to selectio----------------n\n");
				_curveNodeSelection->addNodeToSelection(automation.currentCtrlVal);
				break;
			}
		}
		controllerChanged(automation.currentTrack);
		//Add the node and return instead of processing further
		return;
	}


	int xDiff = (event->pos() - automation.mousePressPos).x();
	int frameDiff = tempomap.tick2frame(abs(xDiff));
	//qDebug("xDiff: %d, frameDiff: %d\n", xDiff, frameDiff);
	if (xDiff < 0)
	{
		frameDiff *= -1;
	}

	double mouseYDiff = (automation.mousePressPos - event->pos()).y();
	double valDiff = (mouseYDiff)/automation.currentTrack->height();
	//qDebug("mouseYDiff: %f, valDiff: %f, xDiff: %d, frameDiff: %d\n", mouseYDiff, valDiff, xDiff, frameDiff);

	automation.currentCtrlList->range(&min,&max);

	//Duplicate the _curveNodeSelection list for undo purposes
	if(!automation.movingStarted && automation.currentCtrlList)
	{//Populate a list with the current values
		m_automationMoveList.clear();
		foreach(CtrlVal* v, _curveNodeSelection->getNodes()) {
			//Add the new
			m_automationMoveList.append(CtrlVal(v->getFrame(), v->val));
		}
		if(m_automationMoveList.isEmpty() && automation.currentCtrlVal)
		{
			m_automationMoveList.append(CtrlVal(automation.currentCtrlVal->getFrame(), automation.currentCtrlVal->val));
		}
		automation.movingStarted = !m_automationMoveList.isEmpty();
	}
	if (automation.currentCtrlList->id() == AC_VOLUME )
	{
		_curveNodeSelection->move(frameDiff, valDiff, min, max, automation.currentCtrlList, automation.currentCtrlVal, true);
	}
	else
	{
		_curveNodeSelection->move(frameDiff, valDiff, min, max, automation.currentCtrlList, automation.currentCtrlVal, false);
	}

	automation.mousePressPos = event->pos();

	//printf("mouseY=%d\n", mouseY);
	controllerChanged(automation.currentTrack);
}

void ComposerCanvas::trackViewChanged()
{
	//printf("ComposerCanvas::trackViewChanged()\n");
    for (iCItem i = _items.begin(); i != _items.end(); ++i)
	{
		NPart* part = (NPart*) (i->second);
		QRect r = part->bbox();
		part->part()->setSelected(i->second->isSelected());
		Track* track = part->part()->track();
		int y = track->y();
		int th = track->height();

		part->setPos(QPoint(part->part()->tick(), y));

		part->setBBox(QRect(part->part()->tick(), y + 1, part->part()->lenTick(), th));
	}
	emit selectionChanged();
	redraw();
}

Part* ComposerCanvas::currentCanvasPart()
{
	if(_curPart)
		return _curPart;
	return 0;
}
