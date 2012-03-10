//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: noteinfo.h,v 1.3 2004/01/09 17:12:54 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __NOTE_INFO_H__
#define __NOTE_INFO_H__

#include <QToolBar>

namespace Awl
{
    class PosEdit;
    //class PitchEdit;
};

class QTableWidget;
class GridCombo;
class QSpinBox;
class QCheckBox;
///class PosEdit;
class PitchEdit;
class Pos;
class QVBoxLayout;

//---------------------------------------------------------
//   NoteInfo
//---------------------------------------------------------

class NoteInfo : public QWidget
{
    Q_OBJECT

	//ToolBar1 merge
    GridCombo* quantLabel;
    QTableWidget* qlist;
    GridCombo* rasterLabel;
    QTableWidget* rlist;
	//end merge

    Awl::PosEdit* selTime;
	QVBoxLayout* m_layout;
    QSpinBox* selLen;
    PitchEdit* selPitch;
    QSpinBox* selVelOn;
    QSpinBox* selVelOff;
    QSpinBox* m_renderAlpha;
	QCheckBox* m_partLines;
    bool deltaMode;
	
	void addTool(QString label, QWidget* tool);

public:
    enum ValType
    {
        VAL_TIME, VAL_LEN, VAL_VELON, VAL_VELOFF, VAL_PITCH
    };
    NoteInfo(QWidget* parent = 0);
    void setValues(unsigned, int, int, int, int);
    void setDeltaMode(bool);
	void enableTools(bool);

private slots:
    void lenChanged(int);
    void velOnChanged(int);
    void velOffChanged(int);
    void pitchChanged(int);
    void timeChanged(const Pos&);
	void alphaChanged(int);
	void partLinesChanged(bool);
    //tb1
    void _rasterChanged(int);
    void _quantChanged(int);
	//end

public slots:
    void setValue(ValType, int);
    //tb1
    void setRaster(int);
    void setQuant(int);
    //end

signals:
    void valueChanged(NoteInfo::ValType, int);
	void alphaChanged();
	void enablePartLines(bool);
	//tb1
    void rasterChanged(int);
    void quantChanged(int);
    void soloChanged(bool);
    void toChanged(int);
	//end
};
#endif

