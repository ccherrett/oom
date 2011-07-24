#ifndef _EDITTOOLS_H_
#define _EDITTOOLS_H_
#include <QFrame>
#include <QList>

class QHBoxLayout;
class QToolButton;
class QAction;

class EditTools : public QFrame
{
	Q_OBJECT
	QHBoxLayout* m_layout;
	void addButton(QAction*);

private slots:
	void songChanged(int);
public slots:
public:
	EditTools(QList<QAction*> actions, QWidget* parent = 0);
	virtual ~EditTools(){}
};

#endif
