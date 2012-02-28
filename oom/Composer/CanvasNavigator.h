#ifndef __OOM_CANVAS_NAVIGATOR__
#define __OOM_CANVAS_NAVIGATOR__

#include <QWidget>
#include <QList>
#include <QGraphicsRectItem>

class QGraphicsView;
class QGraphicsItem;
class QGraphicsScene;
class QResizeEvent;
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

class CanvasNavigator : public QWidget
{
	Q_OBJECT

	ComposerCanvas* m_canvas;
	QGraphicsView* m_view;
	QGraphicsScene* m_scene;
	QGraphicsRectItem* m_playhead;

	QList<PartItem*> m_parts;
	bool m_editing;

	double calcSize(int);

protected:
	virtual void resizeEvent(QResizeEvent*);
	//virtual QSize minimumSizeHint() const;
	//virtual QSize sizeHint() const;

public slots:
	void updateParts();
	void updateSpacing();
	void updateSelections(int = -1);
	void advancePlayhead();

public:
	CanvasNavigator(QWidget* parent = 0);
	void setCanvas(ComposerCanvas* c);
};

#endif
