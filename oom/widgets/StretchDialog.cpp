//=========================================================
//  OOStudio
//  OpenOctave Midi and Audio Editor

//  (C) Copyright 2012 Filipe Coelho and the OOMidi team
//=========================================================

#include "StretchDialog.h"
#include "ui_stretchdialog.h"

#include <QDebug>

StretchDialog::StretchDialog(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::StretchDialog),
      m_proc(this)
{
    ui->setupUi(this);

    connect(&m_proc, SIGNAL(finished(int)), SLOT(accept()));
}

StretchDialog::~StretchDialog()
{
}

void StretchDialog::setFile(QString filename, QString dirName)
{
    m_filename = filename;
    m_dirName = dirName;
}

void StretchDialog::accept()
{
    if (ui->b_apply->isEnabled())
    {
        qWarning("Run rubberband");

        // Run rubberband
        QString program = "rubberband";
        QStringList arguments;
        arguments << "-c" << QString::number(ui->comboBox->currentIndex());
        arguments << m_filename;
        arguments << "/tmp/oom-rubberband.wav";
        qWarning() << program;
        qWarning() << arguments;
        m_proc.start(program, arguments);
        //m_proc.waitForFinished();
        ui->b_apply->setEnabled(false);
    }
    else
    {
        qWarning("Finished");
        
        // Finished
        QDialog::accept();
    }
}
