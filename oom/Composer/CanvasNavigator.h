#ifndef __OOM_CANVAS_NAVIGATOR__
#define __OOM_CANVAS_NAVIGATOR__

#include <QWidget>
#include <QList>
#include <QHash>
#include <QGraphicsRectItem>
#include <QGraphicsScene>

class QGraphicsView;
class QGraphicsItem;
class QGraphicsLineItem;
class QGraphicsItemGroup;
class QGraphicsScene;
class QResizeEvent;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneWheelEvent;
class ComposerCanvas;
class Part;

class PartItem : public QGraphicsRectItem
{
	Part* m_part;
public:
	PartItem(qreal x, qreal y, qreal w, qreal h, QGraphicsItem* parent = 0);
	PartItem(QRectF r, QGraphicsItem* parent = 0);
	void setPart(Part* p) {m_part = p;}
	Part* part(){return  m_part;}
};

class NavigatorScene : public QGraphicsScene
{
	Q_OBJECT
protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent*);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent*);
	virtual void wheelEvent(QGraphicsSceneWheelEvent*);
signals:
	void centerCanvas(int, int);
	void updatePlayhead(int);
public:
	NavigatorScene(QObject* parent = 0);
	NavigatorScene(const QRectF&, QObject* parent = 0);
	NavigatorScene(qreal x, qreal y, qreal w, qreal h, QObject* parent = 0);
};

class CanvasNavigator : public QWidget
{
	Q_OBJECT

	ComposerCanvas* m_canvas;
	QGraphicsView* m_view;
	NavigatorScene* m_scene;
	//QGraphicsLineItem* m_playhead;
	QGraphicsRectItem* m_playhead;
	QGraphicsRectItem* m_start;
	QGraphicsRectItem* m_canvasBox;
	QGraphicsRectItem* m_punchIn;
	QGraphicsRectItem* m_punchOut;
	QGraphicsItemGroup* m_partGroup;

	QList<PartItem*> m_parts;
	QList<int> m_heightList;
	QHash<qint64, QGraphicsRectItem*> m_markers;
	bool m_editing;
	bool m_updateMarkers;

	void createCanvasBox();

protected:
	virtual void resizeEvent(QResizeEvent*);

private slots:
	void updatePlayheadPosition(int);
	void updateCanvasPosition(int, int);

public slots:
	void updateParts();
	void updateSpacing();
	void updateSelections(int = -1);
	void updateCanvasBox();
	void advancePlayhead();
	void updateCanvasBoxColor();
	void updateMarkers();
	void updateLoopRange();

signals:
	void updateScroll(int, int);
public:
	CanvasNavigator(QWidget* parent = 0);
	void setCanvas(ComposerCanvas* c);
	static double calcSize(int);
	static int sceneToTick(int);
	static int sceneToCanvas(int);
};

#endif
