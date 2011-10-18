#ifndef CANVASTESTAPP_H
#define CANVASTESTAPP_H

#include <QMainWindow>

#include "patchcanvas.h"
#include "canvasbox.h"

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
    PatchCanvas::Canvas* canvas;
    PatchScene* scene;
    CanvasBox *box1, *box2, *box3;
};

#endif // CANVASTESTAPP_H
