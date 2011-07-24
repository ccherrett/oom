#ifndef _LOOPTOOLS_H_
#define _LOOPTOOLS_H_
#include <QFrame>

class QHBoxLayout;
class QToolButton;
class QAction;

class LoopToolbar : public QFrame
{
	Q_OBJECT
	QHBoxLayout* m_layout;
	QToolButton* m_btnLoop;
	QToolButton* m_btnPunchin;
	QToolButton* m_btnPunchout;

private slots:
	void songChanged(int);
public slots:
public:
	LoopToolbar(QWidget* parent = 0);
	virtual ~LoopToolbar(){}
};

#endif
