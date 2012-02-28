//=========================================================
//  OOStudio
//  OpenOctave Midi and Audio Editor

//  (C) Copyright 2012 Filipe Coelho and the OOMidi team
//=========================================================

#ifndef __STRETCH_DIALOG_H__
#define __STRETCH_DIALOG_H__

#include <QDialog>
#include <QProcess>

namespace Ui {
class StretchDialog;
}

class StretchDialog : public QDialog
{
    Q_OBJECT
    
public:
    StretchDialog(QWidget* parent = 0);
    ~StretchDialog();
    
    void setFile(QString filename, QString dirName);
    //QString getNewFilename();

public Q_SLOTS:
    virtual void accept();

protected:
    Ui::StretchDialog* ui;
    
private:
    QString m_filename;
    QString m_dirName;
    QProcess m_proc;
};

#endif // __STRETCH_DIALOG_H__
