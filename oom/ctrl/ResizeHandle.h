#ifndef __OOM_RESIZE_HANDLE__
#define __OOM_RESIZE_HANDLE__

#include <QFrame>

class QEvent;
class QMouseEvent;

class ResizeHandle : public QFrame
{
	Q_OBJECT
	int m_height;
	int m_minheight;
	int m_maxheight;
	bool resizeFlag;
	int startY;
    int curY;
	QPoint m_startPos;
	bool m_collapsed;

protected:
	//virtual QSize minimumSizeHint(){return QSize(400, 2);}
    enum
    {
        NORMAL, START_DRAG, DRAG, RESIZE
    } mode;
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void enterEvent(QEvent*);
    virtual void leaveEvent(QEvent*);

signals:
	void handleMoved(int);
	void deltaChanged(int);
	void resizeEnded();
	void resizeStarted();

public slots:
	void setCollapsed(bool c);
	void setMinHeight(int v){m_minheight = v;}
	void setMaxHeight(int v)
	{
		m_maxheight = m_height+v;
	}

public:
	ResizeHandle(QWidget* parent = 0);
	void setStartHeight(int h){m_height = h;}
	int startHeight() {return m_height;}
};

#endif
