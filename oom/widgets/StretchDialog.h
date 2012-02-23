//=========================================================
//  OOStudio
//  OpenOctave Midi and Audio Editor

//  (C) Copyright 2012 Filipe Coelho and the OOMidi team
//=========================================================

#ifndef __STRETCH_DIALOG_H__
#define __STRETCH_DIALOG_H__

#include <QDialog>

namespace Ui {
class StretchDialog;
}

class StretchDialog : public QDialog
{
    Q_OBJECT
    
public:
    StretchDialog(QWidget* parent = 0);
    ~StretchDialog();
    
protected:
    Ui::StretchDialog* ui;
    
};

#endif // __STRETCH_DIALOG_H__
