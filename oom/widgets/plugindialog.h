//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: plugin.h,v 1.9.2.13 2009/12/06 01:25:21 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _PLUGIN_DIALOG_H_
#define _PLUGIN_DIALOG_H_

#include "plugin.h"

#include <QDialog>

class QAbstractButton;
class QComboBox;
class QRadioButton;
class QScrollArea;
class QToolButton;
class QTreeWidget;
class QShowEvent;
class QHideEvent;
class QCloseEvent;

class AudioTrack;
class PluginGui;

//---------------------------------------------------------
//   PluginDialog
//---------------------------------------------------------

enum
{
    SEL_SM, SEL_S, SEL_M, SEL_ALL
};

class PluginDialog : public QDialog
{
    Q_OBJECT

    QTreeWidget* pList;
    QRadioButton* allPlug;
    QRadioButton* onlyM;
    QRadioButton* onlyS;
    QRadioButton* onlySM;
    QPushButton *okB;
    QComboBox* m_cmbType;
    PluginType m_display_type;
	int m_trackType;

public:
    PluginDialog(int type, QWidget* parent = 0);
    static PluginI* getPlugin(int type, QWidget* parent);
    PluginI* value();
    void accept();

public slots:
    void fillPlugs(QAbstractButton*);
    void fillPlugs(int i);
    void fillPlugs(const QString& sortValue);
	void typeChanged(int);

private slots:
    void enableOkB();

private:
    QComboBox *sortBox;
    static int selectedPlugType;
    static QStringList sortItems;

protected:
	virtual void showEvent(QShowEvent*);
	virtual void closeEvent(QCloseEvent*);
	virtual void hideEvent(QHideEvent*);
};

#endif
