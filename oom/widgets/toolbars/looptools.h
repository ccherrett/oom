#ifndef _LOOPTOOLS_H_
#define _LOOPTOOLS_H_
#include <QFrame>

class QBoxLayout;
class QToolButton;
class QAction;

class LoopToolbar : public QFrame
{
	Q_OBJECT
	QBoxLayout* m_layout;
	QToolButton* m_btnLoop;
	QToolButton* m_btnPunchin;
	QToolButton* m_btnPunchout;
	Qt::Orientation m_orient;

private slots:
	void songChanged(int);
	void setLoopSilent(bool);
	void setPunchinSilent(bool);
	void setPunchoutSilent(bool);
public slots:
public:
	LoopToolbar(Qt::Orientation orient = Qt::Horizontal, QWidget* parent = 0);
	virtual ~LoopToolbar(){}
};

#endif
