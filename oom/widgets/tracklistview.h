//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include <QFrame>
#include <QList>
#define PartRole Qt::UserRole+2
#define TrackRole Qt::UserRole+3
#define TrackNameRole Qt::UserRole+4
#define TickRole Qt::UserRole+5

class QTableView;
class QStandardItem;
class QStandardItemModel;
class QItemSelectionModel;
class QString;
class QVBoxLayout;

class Part;
class Track;
class MidiEditor;

class TrackListView : public QFrame
{
	Q_OBJECT

	QTableView* m_table;
	QStandardItemModel* m_model;
	QVBoxLayout* m_layout;
	MidiEditor* m_editor;
	QList<QString> m_selected;

private slots:
	void songChanged(int);
public slots:
	void toggleTrackPart(QStandardItem*);
public:
	TrackListView(MidiEditor* e, QWidget* parent = 0);
	~TrackListView();
};
