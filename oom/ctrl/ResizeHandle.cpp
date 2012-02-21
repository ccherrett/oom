#include "ResizeHandle.h"
#include <QtGui>

ResizeHandle::ResizeHandle(QWidget* parent)
: QFrame(parent)
{
	setFrameShape(QFrame::NoFrame);
	setFrameShadow(QFrame::Plain);
	setFixedHeight(2);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_height = 80;
	m_minheight = 80;
	m_maxheight = 400;
	m_collapsed = false;
	resizeFlag = false;
	startY = 0;
	curY = 0;
	mode = NORMAL;
}

void ResizeHandle::enterEvent(QEvent*)/*{{{*/
{
	if(m_collapsed)
		return;
	if (!resizeFlag)
	{
		resizeFlag = true;
		setCursor(QCursor(Qt::SplitVCursor));
	}
}/*}}}*/

void ResizeHandle::mousePressEvent(QMouseEvent* ev) //{{{
{
	if(m_collapsed)
		return;
	int button = ev->button();

	if (button == Qt::LeftButton)
	{
		if (resizeFlag)
		{
			startY = ev->y();
			mode = RESIZE;
			emit resizeStarted();
		}
	}
}/*}}}*/

void ResizeHandle::mouseMoveEvent(QMouseEvent* ev)/*{{{*/
{
	if(m_collapsed)
		return;

	if(resizeFlag)
	{
		curY = ev->y();
		int delta = startY - curY;
		switch (mode)
		{
			case RESIZE:
			{
				int h = m_height + delta;
//				qDebug("ResizeHandle::mouseMoveEvent: h: %d, max: %d, delta: %d, startY: %d, curY: %d", h, m_maxheight, delta, startY, curY);
				startY = curY;
				if (h < m_minheight || h > m_maxheight)
					return;
				/*	h = m_minheight;
				if(h > m_maxheight)
					h = m_maxheight;
				if(m_height > h)
				{//we are shrinking
					for(int i = m_height; i >= h; --i)
						emit handleMoved(i);
				}
				else
				{
					for(int i = m_height; i <= h; ++i)
						emit handleMoved(i);
				}*/
				emit handleMoved(h);
				emit deltaChanged(delta);
				m_height = h;
			}
			break;
			default:
			break;
		}
	}
}/*}}}*/

void ResizeHandle::mouseReleaseEvent(QMouseEvent*)/*{{{*/
{
	if(m_collapsed)
		return;
	if(mode == RESIZE)
		emit resizeEnded();
	mode = NORMAL;
	setCursor(QCursor(Qt::ArrowCursor));
	resizeFlag = false;
}/*}}}*/

void ResizeHandle::leaveEvent(QEvent*)/*{{{*/
{
	if(m_collapsed)
		return;
	//setCursor(QCursor(Qt::ArrowCursor));
}/*}}}*/

void ResizeHandle::setCollapsed(bool c)
{
	m_collapsed = c;
}
