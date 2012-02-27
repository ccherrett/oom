#ifndef __OOM_CANVAS_NAVIGATOR__
#define __OOM_CANVAS_NAVIGATOR__

#include <QWidget>
#include <QList>

class QGraphicsView;
class QGraphicsScene;
class QResizeEvent;
class ComposerCanvas;

class CanvasNavigator : public QWidget
{
	Q_OBJECT

	ComposerCanvas* m_canvas;
	QGraphicsView* m_view;
	QGraphicsScene* m_scene;

	QList<qint64> m_tracks;

	double calcSize(int);

protected:
	virtual void resizeEvent(QResizeEvent*);

public slots:
	void updateParts();
	void updateSpacing();

public:
	CanvasNavigator(QWidget* parent = 0);
	void setCanvas(ComposerCanvas* c);
};

#endif
