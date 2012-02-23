//=========================================================
//  OOStudio
//  OpenOctave Midi and Audio Editor

//  (C) Copyright 2012 Filipe Coelho and the OOMidi team
//=========================================================

#include "StretchDialog.h"
#include "ui_stretchdialog.h"

StretchDialog::StretchDialog(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::StretchDialog)
{
    ui->setupUi(this);
}

StretchDialog::~StretchDialog()
{
}
