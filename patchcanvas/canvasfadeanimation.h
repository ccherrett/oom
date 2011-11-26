#ifndef CANVASFADEANIMATION_H
#define CANVASFADEANIMATION_H

#include "patchcanvas.h"

#include <QAbstractAnimation>

class QGraphicsItem;

START_NAMESPACE_PATCHCANVAS

class CanvasFadeAnimation : public QAbstractAnimation
{
public:
    CanvasFadeAnimation(QGraphicsItem* item, bool show, QObject* parent=0);

    void setDuration(int time);
    int duration() const;
    int totalDuration() const;

protected:
    void updateCurrentTime(int time);
    void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);
    void updateDirection(QAbstractAnimation::Direction direction);

private:
    QGraphicsItem* _item;
    bool _show;
    int _duration;
};

END_NAMESPACE_PATCHCANVAS

#endif // CANVASFADEANIMATION_H
