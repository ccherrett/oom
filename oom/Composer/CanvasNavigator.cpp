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
	m_partGroup = 0;
	m_start = 0;
	m_playhead = 0;
	m_canvasBox =  0;
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setFixedHeight(80);

	m_scene = new NavigatorScene(QRectF());
	m_scene->setBackgroundBrush(QColor(63, 63, 63));
	
	m_view = new QGraphicsView(m_scene);
	m_view->setRenderHints(QPainter::Antialiasing);
	m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_view->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
	m_view->setFixedHeight(80);
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
		//Pos p(sceneToTick(x), true);
		//song->setPos(0, p);
		int pos = ((m_canvas->mapx(sceneToTick(x))+m_canvas->xOffsetDev())-(m_canvas->width()/2));
		//m_canvas->setXPos(sceneToCanvas(x) - m_canvas->width());
		int ypos = sceneToCanvas(y)-200;
		emit updateScroll(pos, ypos);
	}
}

void CanvasNavigator::createCanvasBox()
{
	QRect crect = m_canvas->contentsRect();
	QRect mapped = m_canvas->mapDev(crect);
	QRectF real(calcSize(mapped.x()), 0, calcSize(mapped.width()), 80);
	m_canvasBox = m_scene->addRect(real);
	//m_canvasBox->setFlag(QGraphicsItem::ItemIsMovable, true);
	m_canvasBox->setZValue(124000.0f);
	QPen pen(QColor(229, 233, 234));
	pen.setWidth(2);
	pen.setCosmetic(true);
	m_canvasBox->setPen(pen);
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
		QRectF real(calcSize(mapped.x()), y, calcSize(mapped.width()), h);
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
}/*}}}*/

void CanvasNavigator::updateParts()/*{{{*/
{
	m_editing = true;
	m_playhead = 0;
	m_start = 0;
	m_canvasBox =  0;
	m_parts.clear();
	m_scene->clear();
	/*foreach(PartItem* i, m_parts)
		m_scene->removeItem(i);
	while(m_parts.size())
		delete m_parts.takeFirst();*/
	m_heightList.clear();
	m_scene->setSceneRect(QRectF());
	int index = 1;
	QList<QGraphicsItem*> group;
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
		//if((*ci)->type() == Track::WAVE || (*ci)->type() == Track::MIDI)
		//{
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
						double w = calcSize(len);//m_canvas->mapx(len)+m_canvas->xOffsetDev();
						double pos = calcSize(tick);//m_canvas->mapx(tick)+m_canvas->xOffsetDev();
					//	qDebug("CanvasNavigator::updateParts: tick: %d, len: %d , pos: %d, w: %d", tick, len, pos, w);
						PartItem* item = new PartItem(pos, ypos, w, partHeight);
						item->setPart(part);
						m_parts.append(item);
						group.append((QGraphicsItem*)item);
						int i = part->colorIndex();
						QColor partWaveColor(config.partWaveColors[i]);
						QColor partColor(config.partColors[i]);
						//partWaveColor.setAlpha(150);
						partColor.setAlpha(150);
						item->setBrush(part->selected() ? partWaveColor : partColor);
						item->setPen(part->selected() ? partColor : partWaveColor);
					}
				}
				++index;
			}
		//}
	}
	QColor colTimeLine = QColor(0, 186, 255);
	int kpos = 0;
	foreach(int i, m_heightList)
		kpos += i;
	kpos = ((kpos + 400) * 8)/100;
	//double val = calcSize(song->cpos());
	//m_playhead = new QGraphicsLineItem(val, 0, val, kpos);
	m_playhead = new QGraphicsRectItem(0, 0, 1, kpos);
	m_playhead->setBrush(colTimeLine);
	m_playhead->setPen(Qt::NoPen);
	m_playhead->setZValue(124001.0f);
	m_scene->addItem(m_playhead);
	createCanvasBox();
	//m_playhead->setBrush(colTimeLine);
	group.append((QGraphicsItem*)m_start);
	//group.append((QGraphicsItem*)m_playhead);
	if(group.size())
	{
		m_partGroup = m_scene->createItemGroup(group);
	}
	else
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
