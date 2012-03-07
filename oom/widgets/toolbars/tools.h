//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: tools.h,v 1.1.1.1 2003/10/27 18:54:49 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <QFrame>
#include <QList>

class Action;

class QHBoxLayout;
class QToolButton;
class QAction;
class QPixmap;
class QIcon;
class QWidget;
class QActionGroup;

enum Tool
{
    PointerTool = 1, PencilTool = 2, RubberTool = 4, CutTool = 8,
    ScoreTool=16, GlueTool=32, QuantTool=64, DrawTool=128, StretchTool=256, MuteTool=512, AutomationTool=1024, MasterTool=2048
};

//const int composerTools = PointerTool | PencilTool | RubberTool | CutTool | GlueTool | StretchTool | MuteTool | AutomationTool;
const int composerTools = PointerTool | PencilTool | RubberTool | CutTool | GlueTool | MuteTool | AutomationTool;
const int masterTools = PointerTool | PencilTool | RubberTool | MasterTool;

struct ToolB
{
    //QPixmap** icon;
	QIcon** icon;
	const char* tip;
    const char* ltip;
};

extern ToolB toolList[];

//---------------------------------------------------------
//   EditToolBar
//---------------------------------------------------------

class EditToolBar : public QFrame
{
    Q_OBJECT
	QHBoxLayout* m_layout;
    Action** actions;
	QActionGroup* action;
    int nactions;

private slots:
    void toolChanged(QAction* action);

signals:
    void toolChanged(int);

public slots:
    void set(int id);
    void setNoUpdate(int id);

public:
    //EditToolBar(QMainWindow*, int, const char* name = 0);
    EditToolBar(QWidget* /*parent*/, int /*tools*/, bool addSpacer = false, const char* name = 0); // Needs a parent !
    ~EditToolBar();
    int curTool();
	QList<QAction*> getActions();
};

#endif

