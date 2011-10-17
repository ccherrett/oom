#ifndef CANVASTESTAPP_H
#define CANVASTESTAPP_H

#include <QMainWindow>

#include "patchscene.h"
#include "theme.h"

namespace Ui {
    class CanvasTestApp;
}

class CanvasTestApp : public QMainWindow
{
    Q_OBJECT

public:
    explicit CanvasTestApp(QWidget *parent = 0);
    ~CanvasTestApp();

private:
    Ui::CanvasTestApp *ui;
    PatchScene* scene;
    Theme* theme;
};

#endif // CANVASTESTAPP_H
