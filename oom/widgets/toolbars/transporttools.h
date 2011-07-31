#ifndef _TRANSPORTTOOLS_H_
#define _TRANSPORTTOOLS_H_
#include <QFrame>

class QHBoxLayout;
class QToolButton;
class QAction;

class TransportToolbar : public QFrame
{
	Q_OBJECT
	QHBoxLayout* m_layout;
	QToolButton* m_btnAudition;
	QToolButton* m_btnRewindEnd;
	QToolButton* m_btnRewind;
	QToolButton* m_btnFFwd;
	QToolButton* m_btnStop;
	QToolButton* m_btnPlay;
	QToolButton* m_btnRecord;
	QToolButton* m_btnMute;
	QToolButton* m_btnSolo;
	QToolButton* m_btnPanic;
	QToolButton* m_btnClick;

private slots:
	void songChanged(int);
	void updateClick(bool);
public slots:
signals:
	void recordTriggered(bool);
public:
	TransportToolbar(QWidget* parent = 0, bool showMuteSolo = false, bool showPanic = false);
	virtual ~TransportToolbar(){}
	void setSoloAction(QAction*);
	void setMuteAction(QAction*);
};

#endif
