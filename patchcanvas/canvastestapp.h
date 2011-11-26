#ifndef CANVASTESTAPP_H
#define CANVASTESTAPP_H

#include <QMainWindow>

#include "patchcanvas.h"
#include "patchscene.h"

namespace Ui {
    class CanvasTestApp;
}

class QSettings;

class CanvasTestApp : public QMainWindow
{
    Q_OBJECT

public:
    explicit CanvasTestApp(QWidget *parent = 0);
    ~CanvasTestApp();

private:
    Ui::CanvasTestApp* ui;
    PatchCanvas::PatchScene* scene;
    QSettings* settings;

    void closeEvent(QCloseEvent* event);
};

#endif // CANVASTESTAPP_H
