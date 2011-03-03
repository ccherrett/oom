//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: $
//  (C) Copyright 2011 Andrew Williams and the OOMidi team
//=========================================================

#include <QObject>
#include <QWidgetAction>

class QListWidget;
class QListWidgetItem;
class Track;

class MenuList : public QWidgetAction
{
	Q_OBJECT
	QListWidget *list;
	Track* _track;

	public:
		MenuList(QWidget* parent = 0, Track* t = 0);
		virtual QWidget* createWidget(QWidget* parent = 0);

	private slots:
		void updateData(QListWidgetItem*);
	
	signals:
		void triggered();
};
