#ifndef _MISCTOOLS_H_
#define _MISCTOOLS_H_
#include <QFrame>

class QHBoxLayout;
class QToolButton;
class QAction;

class MiscToolbar : public QFrame
{
	Q_OBJECT
	QHBoxLayout* m_layout;
	QToolButton* m_btnStep;
	QToolButton* m_btnSpeaker;
	QToolButton* m_btnPanic;

private slots:
	void songChanged(int);
public slots:
public:
	MiscToolbar(QList<QAction*> actions, QWidget* parent = 0);
	virtual ~MiscToolbar(){}
};

#endif
