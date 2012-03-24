#include <QtGui>
#include "CanvasNavigator.h"
#include "ComposerCanvas.h"
#include "config.h"
#include "globals.h"
#include "gconfig.h"
#include "song.h"
#include "track.h"
#include "part.h"
#include "app.h"
#include "marker/marker.h"

static const double MIN_PART_HEIGHT = 1.0;
static const double TICK_PER_PIXEL = 81.37;

//---------------------------------------------------------------
// PartItem
//---------------------------------------------------------------
PartItem::PartItem(qreal x, qreal y, qreal w, qreal h, QGraphicsItem* parent)
: QGraphicsRectItem(x, y, w, h, parent)
{
}

PartItem::PartItem(QRectF r, QGraphicsItem* parent)
: QGraphicsRectItem(r, parent)
{
}

//-----------------------------------------------------------------
// NavigatorScene
//----------------------------------------------------------------
NavigatorScene::NavigatorScene(QObject* parent)
: QGraphicsScene(parent)
{
}

NavigatorScene::NavigatorScene(const QRectF &r, QObject* parent)
: QGraphicsScene(r, parent)
{
}

NavigatorScene::NavigatorScene(qreal x, qreal y, qreal w, qreal h, QObject* parent)
: QGraphicsScene(x, y, w, h, parent)
{
}

void NavigatorScene::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
	if(ev->modifiers() & Qt::ShiftModifier)
		emit updatePlayhead(ev->scenePos().x());
	emit centerCanvas(ev->scenePos().x(), ev->scenePos().y());
}

void NavigatorScene::mouseMoveEvent(QGraphicsSceneMouseEvent* ev)
{
	if(ev->buttons() & (Qt::LeftButton | Qt::RightButton))
	{
		if(ev->modifiers() & Qt::ShiftModifier)
			emit updatePlayhead(ev->scenePos().x());
		emit centerCanvas(ev->scenePos().x(), ev->scenePos().y());
	}
}


void NavigatorScene::wheelEvent(QGraphicsSceneWheelEvent* ev)
{
	ev->ignore();
}

//-------------------------------------------------------------
// Main navigator widget
//------------------------------------------------------------

CanvasNavigator::CanvasNavigator(QWidget* parent)
: QWidget(parent)
{
	m_editing = false;
	m_updateMarkers = false;
	m_partGroup = 0;
	m_start = 0;
	m_playhead = 0;
	m_canvasBox =  0;
	m_punchIn = 0;
	m_punchOut = 0;
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setFixedHeight(60);

	m_scene = new NavigatorScene(QRectF());
	m_scene->setBackgroundBrush(QColor(63, 63, 63));
	
	m_view = new QGraphicsView(m_scene);
	m_view->setRenderHints(QPainter::Antialiasing);
	m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_view->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
	m_view->setFixedHeight(60);
	layout->addWidget(m_view);
	setFocusPolicy(Qt::NoFocus);
	m_view->setFocusPolicy(Qt::NoFocus);
	//m_scene->setFocusPolicy(Qt::NoFocus);
	connect(m_scene, SIGNAL(centerCanvas(int, int)), this, SLOT(updateCanvasPosition(int, int)));
	connect(m_scene, SIGNAL(updatePlayhead(int)), this, SLOT(updatePlayheadPosition(int)));
}

void CanvasNavigator::setCanvas(ComposerCanvas* c)
{
	m_canvas = c;
	createCanvasBox();
}

void CanvasNavigator::updatePlayheadPosition(int pos)
{
	Pos p(sceneToTick(pos), true);
	song->setPos(0, p);
}

void CanvasNavigator::updateCanvasPosition(int x, int y)
{
	if(m_canvas)
	{
		int pos = ((m_canvas->mapx(sceneToTick(x))+m_canvas->xOffsetDev())-(m_canvas->width()/2));
		int ypos = sceneToCanvas(y)-200;
		//int ypos = sceneToCanvas(y);
		emit updateScroll(pos, ypos);
	}
}

void CanvasNavigator::createCanvasBox()
{
	QRect crect = m_canvas->contentsRect();
	QRect mapped = m_canvas->mapDev(crect);
	QRectF real(calcSize(mapped.x()), 0, calcSize(mapped.width()), 80);
	m_canvasBox = m_scene->addRect(real);
	m_canvasBox->setZValue(124000.0f);
	updateCanvasBoxColor();
}

void CanvasNavigator::updateCanvasBoxColor()
{
	QColor outlineColor = QColor(config.partColors[1]);
	QColor fillColor = QColor(config.partWaveColors[1]);
	if(m_canvasBox)
	{
		if(m_canvas)
		{
			Part* _curPart = m_canvas->currentCanvasPart();
			if(_curPart)
			{
				outlineColor = QColor(config.partColors[_curPart->colorIndex()]);
				fillColor = QColor(config.partWaveColors[_curPart->colorIndex()]);
			}
		}
		fillColor.setAlpha(80);
		outlineColor.setAlpha(100);
		QPen pen(outlineColor, 2, Qt::SolidLine);
		pen.setCosmetic(true);
		m_canvasBox->setPen(pen);
		m_canvasBox->setBrush(QBrush(fillColor));
	}
}

void CanvasNavigator::updateMarkers()
{
	MarkerList* markers = song->marker();
	for (iMarker m = markers->begin(); m != markers->end(); ++m)
	{
		QPointF point(calcSize(m->second.tick()), 0.0);
		QGraphicsRectItem* marker = m_markers.value(m->second.id());
		if(marker)
			marker->setPos(point);
	}
}

void CanvasNavigator::updateLoopRange()
{
	if(m_punchIn)
	{
		double lpos = calcSize(song->lpos());
		m_punchIn->setPos(QPointF(lpos, 0.0));
		m_punchIn->setVisible(song->loop() || song->punchin());
	}
	if(m_punchOut)
	{
		double rpos = calcSize(song->rpos());
		m_punchOut->setPos(QPointF(rpos, 0.0));
		m_punchOut->setVisible(song->loop() || song->punchout());
	}
}

void CanvasNavigator::updateCanvasBox()/*{{{*/
{
	if(m_canvasBox && m_canvas)
	{
		QRect crect = m_canvas->contentsRect();
		QRect mapped = m_canvas->mapDev(crect);
		double h = mapped.height();
		h = (h * 8)/100;
		double y = mapped.y();
		y = (y * 8)/100;
		int w = mapped.width();
		int len = song->len();
		int x = mapped.x();
		int add = x+w;
		if(add > len)
		{
			w = (len - x);
		}
		QRectF real(calcSize(mapped.x()), y, calcSize(w), h);
		m_canvasBox->setRect(real);
		//m_view->ensureVisible(m_canvasBox, 0, 0);
	}
}/*}}}*/

void CanvasNavigator::advancePlayhead()/*{{{*/
{
	if(m_editing)
		return;
	if(m_playhead)
	{
		QPoint point(calcSize(song->cpos()), 0);
		m_playhead->setPos(point);
	}
	if(m_start)
	{
		Track* master = song->findTrackByIdAndType(song->masterId(), Track::AUDIO_OUTPUT);
		int mh = MIN_TRACKHEIGHT;
		if(master)
		{
			mh = master->height();
		}
		double partHeight = (mh * 8)/100;
		QRectF rect(0.0, 0.0, calcSize(song->len()), partHeight);
		//QLineF line(0.0, 0.0, calcSize(song->len()), 0.0);
		m_start->setRect(rect);
	}
	updateCanvasBox();
	updateLoopRange();
	if(m_updateMarkers)
		updateMarkers();
	m_updateMarkers = true;
}/*}}}*/

void CanvasNavigator::updateParts()/*{{{*/
{
	m_editing = true;
	m_playhead = 0;
	m_start = 0;
	m_canvasBox =  0;
	m_punchIn = 0;
	m_punchOut = 0;
	m_parts.clear();
	m_markers.clear();
	m_scene->clear();
	/*foreach(PartItem* i, m_parts)
		m_scene->removeItem(i);
	while(m_parts.size())
		delete m_parts.takeFirst();*/
	m_heightList.clear();
	m_scene->setSceneRect(QRectF());
	int index = 1;
	//QList<QGraphicsItem*> group;
	TrackList* tl = song->visibletracks();
	ciTrack ci = tl->begin();
	for(; ci != tl->end(); ci++)
	{
		m_heightList.append((*ci)->height());
	}
	ci = tl->begin();
	if(ci != tl->end())
	{
		int mh = (*ci)->height();
		double partHeight = (mh * 8)/100;
		m_start = m_scene->addRect(0.0, 0.0, calcSize(song->len()), partHeight);
		m_start->setBrush(QColor(63, 63, 63));
		m_start->setPen(QPen(QColor(63, 63, 63)));
		m_start->setZValue(-50);
		ci++;//Special case for master
	}
	for(; ci != tl->end(); ci++)
	{
		Track* track = *ci;
		if(track)
		{
			QList<int> list = m_heightList.mid(0, index);
			int ypos = 0;
			foreach(int i, list)
				ypos += i;
			ypos = (ypos * 8)/100;
			int ih = m_heightList.at(index);
			double partHeight = (ih * 8)/100;
				
			//qDebug("CanvasNavigator::updateParts: partHeight: %2f, ypos: %d", partHeight, ypos);
			PartList* parts = track->parts();
			if(parts && !parts->empty())
			{
				for(iPart p = parts->begin(); p != parts->end(); p++)
				{
					Part *part =  p->second;

					int tick, len;
					if(part && track->isMidiTrack())
					{
						tick = part->tick();
						len = part->lenTick();
					}
					else
					{
						tick = tempomap.frame2tick(part->frame());
						len = tempomap.frame2tick(part->lenFrame());
					}
					double w = calcSize(len);
					double pos = calcSize(tick);
				
					PartItem* item = new PartItem(pos, ypos, w, partHeight);
					item->setPart(part);
					m_parts.append(item);
					//group.append((QGraphicsItem*)item);
					int i = part->colorIndex();
					QColor partWaveColor(config.partWaveColors[i]);
					QColor partColor(config.partColors[i]);
					//partWaveColor.setAlpha(150);
					partColor.setAlpha(150);
					item->setBrush(part->selected() ? partWaveColor : partColor);
					item->setPen(part->selected() ? partColor : partWaveColor);
					m_scene->addItem(item);
				}
			}
			++index;
		}
	}
	QColor colTimeLine = QColor(0, 186, 255);
	int kpos = 0;
	foreach(int i, m_heightList)
		kpos += i;
	//kpos = ((kpos + 400) * 8)/100;
	kpos = ((kpos + 400) * 8)/100;
	
	m_playhead = new QGraphicsRectItem(0, 0, 1, kpos);
	m_playhead->setBrush(colTimeLine);
	m_playhead->setPen(Qt::NoPen);
	m_playhead->setZValue(124001.0f);
	m_playhead->setFlags(QGraphicsItem::ItemIgnoresTransformations);
	m_scene->addItem(m_playhead);
	QColor loopColor(139, 225, 69);
	QColor markerColor(243,191,124);
	MarkerList* markers = song->marker();
	for (iMarker m = markers->begin(); m != markers->end(); ++m)
	{
		//double xp = calcSize(m->second.tick());
		QGraphicsRectItem *marker = m_scene->addRect(0, 0, 1, kpos);
		marker->setData(Qt::UserRole, m->second.id());
		m_markers[m->second.id()] = marker;
		marker->setPen(Qt::NoPen);
		marker->setBrush(markerColor);
		marker->setZValue(124002.0f);
		marker->setFlags(QGraphicsItem::ItemIgnoresTransformations);
		m_updateMarkers = true;
	}

	m_punchIn = new QGraphicsRectItem(0, 0, 1, kpos);
	m_punchIn->setBrush(loopColor);
	m_punchIn->setPen(Qt::NoPen);
	m_punchIn->setZValue(124003.0f);
	m_punchIn->setFlags(QGraphicsItem::ItemIgnoresTransformations);
	m_scene->addItem(m_punchIn);
	m_punchIn->setVisible(song->loop() || song->punchin());

	m_punchOut = new QGraphicsRectItem(0, 0, 1, kpos);
	m_punchOut->setBrush(loopColor);
	m_punchOut->setPen(Qt::NoPen);
	m_punchOut->setZValue(124003.0f);
	m_punchOut->setFlags(QGraphicsItem::ItemIgnoresTransformations);
	m_scene->addItem(m_punchOut);
	m_punchOut->setVisible(song->loop() || song->punchout());

	createCanvasBox();
	
	//group.append((QGraphicsItem*)m_start);
	//group.append((QGraphicsItem*)m_playhead);
	//if(group.size())
	//{
	//	m_partGroup = m_scene->createItemGroup(group);
	//}
	//else
		m_partGroup = 0;
	updateSpacing();
	m_editing = false;
}/*}}}*/

double CanvasNavigator::calcSize(int val)
{
	double rv = 0.0;
	rv = ((val / TICK_PER_PIXEL) * 8)/100;
	return rv;
}

int CanvasNavigator::sceneToTick(int size)
{
	int rv = 0;
	rv = ((size * TICK_PER_PIXEL) * 100)/8;
	return rv;
}

int CanvasNavigator::sceneToCanvas(int size)
{
	int rv = 0;
	rv = (size * 100)/8;
	return rv;
}

void CanvasNavigator::updateSelections(int)/*{{{*/
{
//	qDebug("CanvasNavigator::updateSelections");
	foreach(PartItem* p, m_parts)
	{
		Part* part = p->part();
		if(part)
		{
			int i = part->colorIndex();
			QColor partWaveColor(config.partWaveColors[i]);
			QColor partColor(config.partColors[i]);
			//partWaveColor.setAlpha(150);
			partColor.setAlpha(150);
			p->setBrush(part->selected() ? partWaveColor : partColor);
			p->setPen(part->selected() ? partColor : partWaveColor);
		}
	}
	updateCanvasBoxColor();
	//updateSpacing();
}/*}}}*/

void CanvasNavigator::resizeEvent(QResizeEvent*)
{
	updateParts();
}

void CanvasNavigator::updateSpacing()
{
	QRectF bounds;
/*	if(m_partGroup)
	{
		bounds = m_partGroup->childrenBoundingRect();
	}
	else
	{*/
		bounds = m_scene->itemsBoundingRect();
	//}
	m_view->fitInView(bounds, Qt::IgnoreAspectRatio);
	//m_view->fitInView(bounds, Qt::KeepAspectRatio);
	//m_view->fitInView(bounds, Qt::KeepAspectRatioByExpanding);
	//m_start->setScale(1.0);
}

/*QSize CanvasNavigator::sizeHint() const
{
	return QSize(400, 80);
}

QSize CanvasNavigator::minimumSizeHint() const
{
	return sizeHint();
}*/
