#ifndef _EPICTOOLS_H_
#define _EPICTOOLS_H_
#include <QFrame>
#include <QList>

class QHBoxLayout;
class QToolButton;
class QAction;

class EpicToolbar : public QFrame
{
	Q_OBJECT
	QHBoxLayout* m_layout;
	QToolButton* m_btnSelect;
	QToolButton* m_btnDraw;
	QToolButton* m_btnRecord;
	QToolButton* m_btnParts;

private slots:
	void songChanged(int);
public slots:
public:
	EpicToolbar(QList<QAction*> actions, QWidget* parent = 0);
	virtual ~EpicToolbar(){}
	//void setSoloAction(QAction*);
	//void setMuteAction(QAction*);
};

#endif
