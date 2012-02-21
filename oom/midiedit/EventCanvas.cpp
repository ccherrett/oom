//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id:$
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//=========================================================

#include <errno.h>
#include <values.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <QKeyEvent>
#include <QDropEvent>
#include <QEvent>
#include <QMimeData>
#include <QByteArray>
#include <QDrag>
#include <QMenu>
#include <QAction>
#include <QTimer>

#include "xml.h"
#include "AbstractMidiEditor.h"
#include "EventCanvas.h"
#include "app.h"
#include "Composer.h"
#include "song.h"
#include "event.h"
#include "shortcuts.h"
#include "audio.h"
#include "gconfig.h"
#include "globals.h"
#include "traverso_shared/TConfig.h"

static int NOTE_PLAY_TIME = 20;
//---------------------------------------------------------
//   EventCanvas
//---------------------------------------------------------

EventCanvas::EventCanvas(AbstractMidiEditor* pr, QWidget* parent, int sx, int sy, const char* name)
: Canvas(parent, sx, sy, name)
{
	editor = pr;
	_steprec = false;
	_midiin = false;
	_playEvents = false;
	m_showcomments = false;
	curVelo = 70;

    //setBg(QColor(226, 229, 229));//this was the old ligh color we are moving to dark now
    setBg(QColor(63,63,63));
	setAcceptDrops(true);
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	_curPart = (MidiPart*) (editor->parts()->begin()->second);
	_curPartId = _curPart->sn();
	bool pl = tconfig().get_property("PerformerEdit", "partLines", true).toBool();
	_drawPartLines = true;
	_drawPartEndLine = pl;
    //setDrawPartLines(pl);
}

//---------------------------------------------------------
//   getCaption
//---------------------------------------------------------

QString EventCanvas::getCaption() const
{
	int bar1, bar2, xx;
	unsigned x;
	///sigmap.tickValues(curPart->tick(), &bar1, &xx, &x);
	AL::sigmap.tickValues(_curPart->tick(), &bar1, &xx, &x);
	///sigmap.tickValues(curPart->tick() + curPart->lenTick(), &bar2, &xx, &x);
	AL::sigmap.tickValues(_curPart->tick() + _curPart->lenTick(), &bar2, &xx, &x);

	return QString("") + _curPart->track()->name() + QString("  :  ") + _curPart->name() + QString("     Bar: %1 to %2     ").arg(bar1 + 1).arg(bar2 + 1);
}

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void EventCanvas::leaveEvent(QEvent*)
{
	emit pitchChanged(-1);
    emit timeChanged(MAXINT);
}

//---------------------------------------------------------
//   enterEvent
//---------------------------------------------------------

void EventCanvas::enterEvent(QEvent*)
{
    emit enterCanvas();
}

//---------------------------------------------------------
//   raster
//---------------------------------------------------------

QPoint EventCanvas::raster(const QPoint& p) const
{
	int x = p.x();
	if (x < 0)
		x = 0;
	x = editor->rasterVal(x);
	int pitch = y2pitch(p.y());
	int y = pitch2y(pitch);
	return QPoint(x, y);
}

//---------------------------------------------------------
//   startUndo
//---------------------------------------------------------

void EventCanvas::startUndo(DragType)
{
	song->startUndo();
}

//---------------------------------------------------------
//   endUndo
//---------------------------------------------------------

void EventCanvas::endUndo(DragType dtype, int flags)
{
	song->endUndo(flags | ((dtype == MOVE_COPY || dtype == MOVE_CLONE)
						   ? SC_EVENT_INSERTED : SC_EVENT_MODIFIED));
}

//---------------------------------------------------------
//   mouseMove
//---------------------------------------------------------

void EventCanvas::mouseMove(QMouseEvent* event)
{
	emit pitchChanged(y2pitch(event->pos().y()));
	int x = event->pos().x();
    emit timeChanged(editor->rasterVal(x));
}

//---------------------------------------------------------
//   updateSelection
//---------------------------------------------------------

void EventCanvas::updateSelection()
{
    song->update(SC_SELECTION);
}

//---------------------------------------------------------
//   songChanged(type)
//---------------------------------------------------------

void EventCanvas::songChanged(int flags)/*{{{*/
{
	// Is it simply a midi controller value adjustment? Forget it.
	if (flags == SC_MIDI_CONTROLLER)
		return;

	if (flags & ~SC_SELECTION)
	{
		_items.clear();
		start_tick = MAXINT;
		end_tick = 0;
	_curPart = 0;

		for (iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p)
		{
			MidiPart* part = (MidiPart*) (p->second);
	    if (part->sn() == _curPartId)
		_curPart = part;
			unsigned stick = part->tick();
			unsigned len = part->lenTick();
			unsigned etick = stick + len;
			if (stick < start_tick)
				start_tick = stick;
			if (etick > end_tick)
				end_tick = etick;

			EventList* el = part->events();
			for (iEvent i = el->begin(); i != el->end(); ++i)
			{
				Event e = i->second;
				if (e.isNote())
				{
					addItem(part, e);
				}
			}
		}
	}

	Event event;
	MidiPart* part = 0;
	int x = 0;
	CItem* nevent = 0;

	int n = 0; // count selections
    for (iCItem k = _items.begin(); k != _items.end(); ++k)
	{
		Event ev = k->second->event();
		bool selected = ev.selected();
		if (selected)
		{
			k->second->setSelected(true);
			++n;
			if (!nevent)
			{
				nevent = k->second;
				Event mi = nevent->event();
				curVelo = mi.velo();
			}
		}
	}
	start_tick = song->roundDownBar(start_tick);
	end_tick = song->roundUpBar(end_tick);

	if (n == 1)
	{
		x = nevent->x();
		event = nevent->event();
		part = (MidiPart*) nevent->part();
	if (_curPart != part)
		{
	    _curPart = part;
	    _curPartId = _curPart->sn();
			curPartChanged();
		}
	}
	emit selectionChanged(x, event, part);
    if (_curPart == 0)
	{
		_curPart = (MidiPart*) (editor->parts()->begin()->second);
		if(_curPart)
		{
			editor->setCurCanvasPart(_curPart);
		}
	}	

	updateCItemsZValues();

	redraw();
}/*}}}*/

//---------------------------------------------------------
//   selectAtTick
//---------------------------------------------------------

void EventCanvas::selectAtTick(unsigned int tick)/*{{{*/
{
	CItemList list = _items;
	if(multiPartSelectionAction && !multiPartSelectionAction->isChecked())
		list = getItemlistForCurrentPart();
	//CItemList list = getItemlistForCurrentPart();

	//Select note nearest tick, if none selected and there are any
	if (!list.empty() && selectionSize() == 0)
	{
		iCItem i = list.begin();
		CItem* nearest = i->second;

		while (i != list.end())
		{
			CItem* cur = i->second;
			unsigned int curtk = abs(cur->x() + cur->part()->tick() - tick);
			unsigned int neartk = abs(nearest->x() + nearest->part()->tick() - tick);

			if (curtk < neartk)
			{
				nearest = cur;
			}

			i++;
		}

		if (!nearest->isSelected())
		{
			selectItem(nearest, true);
			if(editor->isGlobalEdit())
				populateMultiSelect(nearest);
			songChanged(SC_SELECTION);
		}
		itemPressed(nearest);
		m_tempPlayItems.append(nearest);
		QTimer::singleShot(NOTE_PLAY_TIME, this, SLOT(playReleaseForItem()));
	}
}/*}}}*/

void EventCanvas::playReleaseForItem()
{
	if(!m_tempPlayItems.isEmpty())
		itemReleased(m_tempPlayItems.takeFirst(), QPoint(1,1));
}

//---------------------------------------------------------
//   track
//---------------------------------------------------------

MidiTrack* EventCanvas::track() const
{
	return ((MidiPart*) _curPart)->track();
}

void EventCanvas::populateMultiSelect(CItem* baseItem)/*{{{*/
{
	if(editor->isGlobalEdit() && baseItem)
	{
		PartList* pl = editor->parts();
		int curTranspose = ((MidiTrack*)baseItem->part()->track())->getTransposition();
		Event curEvent = baseItem->event();
		int curPitch = curEvent.pitch();
		int curRawPitch = curPitch - curTranspose;
		//int curLen = curEvent.lenTick();
		m_multiSelect.clear();
		for(iPart p = pl->begin(); p != pl->end(); ++p)
		{
			if(p->second == _curPart)
				continue;
			CItemList pitems = getItemlistForPart(p->second);
			for (iCItem i = pitems.begin(); i != pitems.end(); ++i)
			{
				MidiTrack* mtrack = (MidiTrack*)i->second->part()->track();
				int transp = mtrack->getTransposition();
				Event e = i->second->event();
				if(e.empty())
					continue;
				int pitch = e.pitch();
				int rpitch = pitch - transp;
				//int len = e.lenTick();
				//printf("Current pitch: %d, rawpitch: %d - note pitch: %d, raw: %d\n", curPitch, curRawPitch, pitch, rpitch);
				if(e.tick() == curEvent.tick() && rpitch == curRawPitch/*, len == curLen*/)
				{
					m_multiSelect.add(i->second);
						break;
				}
			}
		}
		//printf("MultiSelect list size: %d \n", (int)m_multiSelect.size());
	}
}/*}}}*/

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void EventCanvas::viewMousePressEvent(QMouseEvent* event)/*{{{*/
{
	///keyState = event->state();
	_keyState = ((QInputEvent*) event)->modifiers();
	_button = event->button();

	//printf("viewMousePressEvent buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());

	// special events if right button is clicked while operations
	// like moving or drawing lasso is performed.
	if (event->buttons() & Qt::RightButton & ~(event->button()))
	{
		//printf("viewMousePressEvent special buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
		switch (_drag)
		{
		case DRAG_LASSO:
			_drag = DRAG_OFF;
			redraw();
			return;
		case DRAG_MOVE:
			_drag = DRAG_OFF;
			endMoveItems(_start, MOVE_MOVE, 0);
			return;
		default:
			break;
		}
	}

	// ignore event if (another) button is already active:
	if (event->buttons() & (Qt::LeftButton | Qt::RightButton | Qt::MidButton) & ~(event->button()))
	{
		//printf("viewMousePressEvent ignoring buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
		return;
	}
	bool shift = _keyState & Qt::ShiftModifier;
	bool alt = _keyState & Qt::AltModifier;
	bool ctrl = _keyState & Qt::ControlModifier;
	_start = event->pos();

	//---------------------------------------------------
	//    set curItem to item mouse is pointing
	//    (if any)
	//---------------------------------------------------

	CItemList list = _items;
	if(multiPartSelectionAction && !multiPartSelectionAction->isChecked())
		list = getItemlistForCurrentPart();
	if (virt())
	{
		_curItem = list.find(_start);//_items.find(_start);
	}
	else
	{
		_curItem = 0; //selectAtTick(_start.x());
		iCItem ius;
		bool usfound = false;
		for (iCItem i = list.begin(); i != list.end(); ++i)
		{
			MidiTrack* mtrack = (MidiTrack*)i->second->part()->track();
			int sy = _start.y();
			int p = y2pitch(sy);
			if(editor->isGlobalEdit())
				p += mtrack->getTransposition();
			int p2 = pitch2y(p);
			QPoint lpos(_start.x(), p2);
			QRect box = i->second->bbox();
			int x = rmapxDev(box.x());
			int y = rmapyDev(box.y());
			int w = rmapxDev(box.width());
			int h = rmapyDev(box.height());
			QRect r(x, y, w, h);
			r.translate(i->second->pos().x(), i->second->pos().y());
			if(r.contains(lpos))
			{
				if (i->second->isSelected())
				{
					_curItem = i->second;
					break;
				}
				else if (!usfound)
				{
					ius = i;
					usfound = true;
				}
			}
		}
		if (!_curItem && usfound)
			_curItem = ius->second;
		
	}

	if(editor->isGlobalEdit() && _curItem)
	{
		populateMultiSelect(_curItem);
	}

	if (_curItem && (event->button() == Qt::MidButton))
	{
		if (!_curItem->isSelected())
		{
			selectItem(_curItem, true);
			updateSelection();
			redraw();
		}
		startDrag(_curItem, shift);
	}
	else if (event->button() == Qt::RightButton)
	{
		if (_curItem)
		{
			if (shift)
			{
				_drag = DRAG_RESIZE;
				setCursor();
				int dx = _start.x() - _curItem->x();
				_curItem->setWidth(dx);
				_start.setX(_curItem->x());
				deselectAll();
				selectItem(_curItem, true);
				updateSelection();
				redraw();
			}
			else
			{
				_itemPopupMenu = genItemPopup(_curItem);
				if (_itemPopupMenu)
				{
					QAction *act = _itemPopupMenu->exec(QCursor::pos());
					if (act)
						itemPopup(_curItem, act->data().toInt(), _start);
					delete _itemPopupMenu;
				}
			}
		}
		else
		{
			_canvasPopupMenu = genCanvasPopup(true);
			if (_canvasPopupMenu)
			{
				QAction *act = _canvasPopupMenu->exec(QCursor::pos(), 0);
				if (act)
				{
					int actnum = act->data().toInt();
					canvasPopup(actnum);
					if(actnum >= 20) //Nome of the tools have a higher number than 9
					{
						editor->updateCanvas();
						oom->composer->updateCanvas();
					}	
				}
				delete _canvasPopupMenu;
			}
		}
	}
	else if (event->button() == Qt::LeftButton)
	{
		switch (_tool)
		{
		case PointerTool:
			if (_curItem)
			{
				/*if (_curItem->part() != _curPart)
				{
					_curPart = _curItem->part();
					_curPartId = _curPart->sn();
					curPartChanged();
				}*/
				itemPressed(_curItem);
				if (shift)
					_drag = DRAG_COPY_START;
				else if (alt)
				{
					_drag = DRAG_CLONE_START;
				}
				else if (ctrl)
				{ //Select all on the same pitch (e.g. same y-value)
					deselectAll();
					//printf("Yes, ctrl and press\n");
					for (iCItem i = _items.begin(); i != _items.end(); ++i)
					{
						if (i->second->y() == _curItem->y())
							selectItem(i->second, true);
					}
					updateSelection();
					redraw();
				}
				else
					_drag = DRAG_MOVE_START;
			}
			else
				_drag = DRAG_LASSO_START;
			setCursor();
			break;

			case RubberTool:
			deleteItem(_start);
			_drag = DRAG_DELETE;
			setCursor();
			break;

			case PencilTool:
			if (_curItem)
			{
				_drag = DRAG_RESIZE;
				setCursor();
				int dx = _start.x() - _curItem->x();
				_curItem->setWidth(dx);
				_start.setX(_curItem->x());
			}
			else
			{
				_drag = DRAG_NEW;
				setCursor();
				_curItem = newItem(_start, event->modifiers());
				if (_curItem)
					_items.add(_curItem);
				else
				{
					_drag = DRAG_OFF;
					setCursor();
				}
			}
			deselectAll();
			if (_curItem)
			{
				selectItem(_curItem, true);
				// Play the note
				itemPressed(_curItem);
			}
			updateSelection();
			redraw();
			break;

			default:
			break;
		}
	}
	mousePress(event);
}/*}}}*/

void EventCanvas::mouseRelease(const QPoint& pos)
{
	if(_tool == PencilTool && _curItem)
	{
		itemReleased(_curItem, pos);
	}
}

QMenu* EventCanvas::genItemPopup(CItem* item)/*{{{*/
{
	QMenu* notePopup = new QMenu(this);
	QMenu* colorPopup = notePopup->addMenu(tr("Part Color"));

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
			if(item->part()->colorIndex() == i)
			{
				colorname = QString(config.partColorNames[i]);
				colorPopup->setIcon(partColorIcons.at(i));
				colorPopup->setTitle(colorSub->title()+": "+colorname);

				colorname = QString("* "+config.partColorNames[i]);
				QAction *act_color = colorSub->addAction(partColorIcons.at(i), colorname);
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

	notePopup->addSeparator();
	for (unsigned i = 0; i < 9; ++i)
	{
		if ((_canvasTools & (1 << i)) == 0)
			continue;
		QAction* act = notePopup->addAction(QIcon(**toolList[i].icon), tr(toolList[i].tip));
		act->setData(1 << i); 
	}
	
	return notePopup;
}/*}}}*/


void EventCanvas::itemPopup(CItem* item, int n, const QPoint&)/*{{{*/
{
	switch (n)
	{
		case 20 ... NUM_PARTCOLORS + 20:
		{
			int curColorIndex = n - 20;
			if (item)
				item->part()->setColorIndex(curColorIndex);

			editor->updateCanvas();
			oom->composer->updateCanvas();
			song->update(SC_PART_COLOR_MODIFIED);
			redraw();
			break;
		}
		default:
			canvasPopup(n);
			break;
	}
}/*}}}*/

//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------

void EventCanvas::keyPress(QKeyEvent* event)/*{{{*/
{
	event->ignore();
}/*}}}*/

//---------------------------------------------------------
// actionCommands
//---------------------------------------------------------

void EventCanvas::actionCommand(int action)/*{{{*/
{
	switch(action)
	{
		case LOCATORS_TO_SELECTION:
		{
			int tick_max = 0;
			int tick_min = INT_MAX;
			bool found = false;

			for (iCItem i = _items.begin(); i != _items.end(); i++)
			{
				if (!i->second->isSelected())
					continue;

				int tick = i->second->x();
				int len = i->second->event().lenTick();
				found = true;
				if (tick + len > tick_max)
					tick_max = tick + len;
				if (tick < tick_min)
					tick_min = tick;
			}
			if (found)
			{
				Pos p1(tick_min, true);
				Pos p2(tick_max, true);
				song->setPos(1, p1);
				song->setPos(2, p2);
			}
		}
		break;
		case  SEL_RIGHT ... SEL_RIGHT_ADD:
		{
			if (action == SEL_RIGHT && allItemsAreSelected())
			{
				deselectAll();
				selectAtTick(song->cpos());
				return;
			}

			iCItem i, iRightmost;
			CItem* rightmost = NULL;

			// get a list of items that belong to the current part
			// since multiple parts have populated the _items list
			// we need to filter on the actual current Part!
			CItemList list = _items;
			if(multiPartSelectionAction && !multiPartSelectionAction->isChecked())
				list = getItemlistForCurrentPart();

			//Get the rightmost selected note (if any)
			i = list.begin();
			while (i != list.end())
			{
				if (i->second->isSelected())
				{
					iRightmost = i;
					rightmost = i->second;
				}

				++i;
			}
			if (rightmost)
			{
				iCItem temp = iRightmost;
				temp++;
				//If so, deselect current note and select the one to the right
				if (temp != list.end())
				{
					if (action != SEL_RIGHT_ADD)
						deselectAll();

					iRightmost++;
					iRightmost->second->setSelected(true);
					itemPressed(iRightmost->second);
					m_tempPlayItems.append(iRightmost->second);
					QTimer::singleShot(NOTE_PLAY_TIME, this, SLOT(playReleaseForItem()));
					if(editor->isGlobalEdit())
						populateMultiSelect(iRightmost->second);
					updateSelection();
				}
			}
			else // there was no item selected at all? Then select nearest to tick if there is any
			{
				selectAtTick(song->cpos());
				updateSelection();
			}
		}
		break;
		case SEL_LEFT ... SEL_LEFT_ADD:
		{
			if (action == SEL_LEFT && allItemsAreSelected())
			{
				deselectAll();
				selectAtTick(song->cpos());
				return;
			}

			iCItem i, iLeftmost;
			CItem* leftmost = NULL;

			// get a list of items that belong to the current part
			// since multiple parts have populated the _items list
			// we need to filter on the actual current Part!
			CItemList list = _items;
			if(multiPartSelectionAction && !multiPartSelectionAction->isChecked())
				list = getItemlistForCurrentPart();

			if (list.size() > 0)
			{
				i = list.end();
				while (i != list.begin())
				{
					--i;

					if (i->second->isSelected())
					{
						iLeftmost = i;
						leftmost = i->second;
					}
				}
				if (leftmost)
				{
					if (iLeftmost != list.begin())
					{
						//Add item
						if (action != SEL_LEFT_ADD)
							deselectAll();

						iLeftmost--;
						iLeftmost->second->setSelected(true);
						itemPressed(iLeftmost->second);
						m_tempPlayItems.append(iLeftmost->second);
						QTimer::singleShot(NOTE_PLAY_TIME, this, SLOT(playReleaseForItem()));
						if(editor->isGlobalEdit())
							populateMultiSelect(iLeftmost->second);
						updateSelection();
					} else {
						leftmost->setSelected(true);
						itemPressed(leftmost);
						m_tempPlayItems.append(leftmost);
						QTimer::singleShot(NOTE_PLAY_TIME, this, SLOT(playReleaseForItem()));
						if(editor->isGlobalEdit())
							populateMultiSelect(leftmost);
						updateSelection();
					}
				} else // there was no item selected at all? Then select nearest to tick if there is any
				{
					selectAtTick(song->cpos());
					updateSelection();
				}
			}
		}
		break;
		case INC_PITCH_OCTAVE:
		{
			modifySelected(NoteInfo::VAL_PITCH, 12);
		}
		break;
		case DEC_PITCH_OCTAVE:
		{
			modifySelected(NoteInfo::VAL_PITCH, -12);
		}
		break;
		case INC_PITCH:
		{
			modifySelected(NoteInfo::VAL_PITCH, 1);
		}
		break;
		case DEC_PITCH:
		{
			modifySelected(NoteInfo::VAL_PITCH, -1);
		}
		break;
		case INC_POS:
		{
			// TODO: Check boundaries
			modifySelected(NoteInfo::VAL_TIME, editor->raster());
		}
		break;
		case DEC_POS:
		{
			// TODO: Check boundaries
			modifySelected(NoteInfo::VAL_TIME, 0 - editor->raster());
		}
		break;
		case INCREASE_LEN:
		{
			// TODO: Check boundaries
			modifySelected(NoteInfo::VAL_LEN, editor->raster());
		}
		break;
		case DECREASE_LEN:
		{
			// TODO: Check boundaries
			modifySelected(NoteInfo::VAL_LEN, 0 - editor->raster());
		}
		break;
		case GOTO_SEL_NOTE:
		{
			CItem* leftmost = getLeftMostSelected();
			if (leftmost)
			{
				unsigned newtick = leftmost->event().tick() + leftmost->part()->tick();
				Pos p1(newtick, true);
				song->setPos(0, p1, true, true, false);
			}
		}
		break;
		case MIDI_PANIC:
		{
			song->panic();
		}
		break;
	}
}/*}}}*/

//---------------------------------------------------------
//   getTextDrag
//---------------------------------------------------------

//QDrag* EventCanvas::getTextDrag(QWidget* parent)

QMimeData* EventCanvas::getTextDrag()
{
	//---------------------------------------------------
	//   generate event list from selected events
	//---------------------------------------------------

	EventList el;
	unsigned startTick = MAXINT;
	for (iCItem i = _items.begin(); i != _items.end(); ++i)
	{
		if (!i->second->isSelected())
			continue;
		///NEvent* ne = (NEvent*)(i->second);
		CItem* ne = i->second;
		Event e = ne->event();
		if (startTick == MAXINT)
			startTick = e.tick();
		el.add(e);
	}

	//---------------------------------------------------
	//    write events as XML into tmp file
	//---------------------------------------------------

	FILE* tmp = tmpfile();
	if (tmp == 0)
	{
		fprintf(stderr, "EventCanvas::getTextDrag() fopen failed: %s\n",
				strerror(errno));
		return 0;
	}
	Xml xml(tmp);

	int level = 0;
	xml.tag(level++, "eventlist");
	for (ciEvent e = el.begin(); e != el.end(); ++e)
		e->second.write(level, xml, -startTick);
	xml.etag(--level, "eventlist");

	//---------------------------------------------------
	//    read tmp file into drag Object
	//---------------------------------------------------

	fflush(tmp);
	struct stat f_stat;
	if (fstat(fileno(tmp), &f_stat) == -1)
	{
		fprintf(stderr, "PerformerCanvas::copy() fstat failes:<%s>\n",
				strerror(errno));
		fclose(tmp);
		return 0;
	}
	int n = f_stat.st_size;
	char* fbuf = (char*) mmap(0, n + 1, PROT_READ | PROT_WRITE,
							  MAP_PRIVATE, fileno(tmp), 0);
	fbuf[n] = 0;

	QByteArray data(fbuf);
	QMimeData* md = new QMimeData();
	//QDrag* drag = new QDrag(parent);

	md->setData("text/x-oom-eventlist", data);
	//drag->setMimeData(md);

	munmap(fbuf, n);
	fclose(tmp);

	//return drag;
	return md;
}

//---------------------------------------------------------
//   pasteAt
//---------------------------------------------------------

void EventCanvas::pasteAt(const QString& pt, int pos)
{
	QByteArray ba = pt.toLatin1();
	const char* p = ba.constData();
	Xml xml(p);
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
		case Xml::Error:
		case Xml::End:
			return;
		case Xml::TagStart:
			if (tag == "eventlist")
			{
				song->startUndo();
				EventList* el = new EventList();
				el->read(xml, "eventlist", true);
				int modified = SC_EVENT_INSERTED;
				for (iEvent i = el->begin(); i != el->end(); ++i)
				{
					Event e = i->second;
					int tick = e.tick() + pos - _curPart->tick();
					if (tick < 0)
					{
						printf("ERROR: trying to add event before current part!\n");
						song->endUndo(SC_EVENT_INSERTED);
						delete el;
						return;
					}

					e.setTick(tick);
					int diff = e.endTick() - _curPart->lenTick();
					if (diff > 0)
					{// too short part? extend it
						Part* newPart = _curPart->clone();
						newPart->setLenTick(newPart->lenTick() + diff);
						// Indicate no undo, and do port controller values but not clone parts.
						audio->msgChangePart(_curPart, newPart, false, true, false);
						modified = modified | SC_PART_MODIFIED;
						_curPart = newPart; // reassign
					}
					// Indicate no undo, and do not do port controller values and clone parts.
					audio->msgAddEvent(e, _curPart, false, false, false);
				}
				song->endUndo(modified);
				delete el;
				return;
			}
			else
				xml.unknown("pasteAt");
			break;
			case Xml::Attribut:
			case Xml::TagEnd:
			default:
			break;
		}
	}
}

//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void EventCanvas::viewDropEvent(QDropEvent* event)
{
	QString text;
	if (event->source() == this)
	{
		printf("local DROP\n"); // REMOVE Tim
		//event->acceptProposedAction();
		//event->ignore();                     // TODO CHECK Tim.
		return;
	}
	if (event->mimeData()->hasFormat("text/x-oom-eventlist"))
	{
		text = QString(event->mimeData()->data("text/x-oom-eventlist"));

		int x = editor->rasterVal(event->pos().x());
		if (x < 0)
			x = 0;
		pasteAt(text, x);
		//event->accept();  // TODO
	}
	else
	{
		printf("cannot decode drop\n");
		//event->acceptProposedAction();
		//event->ignore();                     // TODO CHECK Tim.
	}
}

CItem* EventCanvas::getRightMostSelected()
{
	iCItem i, iRightmost;
	CItem* rightmost = NULL;

	// get a list of items that belong to the current part
	// since multiple parts have populated the _items list
	// we need to filter on the actual current Part!
	CItemList list = _items;
	if(multiPartSelectionAction && !multiPartSelectionAction->isChecked())
		list = getItemlistForCurrentPart();
	//CItemList list = getItemlistForCurrentPart();

	//Get the rightmost selected note (if any)
	i = list.begin();
	while (i != list.end())
	{
		if (i->second->isSelected())
		{
			iRightmost = i;
			rightmost = i->second;
		}

		++i;
	}

	return rightmost;
}

CItem* EventCanvas::getLeftMostSelected()
{
	iCItem i, iLeftmost;
	CItem* leftmost = NULL;

	// get a list of items that belong to the current part
	// since multiple parts have populated the _items list
	// we need to filter on the actual current Part!
	//CItemList list = getItemlistForCurrentPart();
	CItemList list = _items;
	if(multiPartSelectionAction && !multiPartSelectionAction->isChecked())
		list = getItemlistForCurrentPart();

	if (list.size() > 0)
	{
		i = list.end();
		while (i != list.begin())
		{
			--i;

			if (i->second->isSelected())
			{
				iLeftmost = i;
				leftmost = i->second;
			}
		}
	}

	return leftmost;
}
