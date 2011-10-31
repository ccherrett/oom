#include "canvasfadeanimation.h"

#include <QGraphicsItem>

START_NAMESPACE_PATCHCANVAS

CanvasFadeAnimation::CanvasFadeAnimation(QGraphicsItem* item, bool show, QObject* parent) :
    QAbstractAnimation(parent)
{
    _item = item;
    _show = show;
    _duration = 0;
}

void CanvasFadeAnimation::setDuration(int time)
{
    if (!_show && !_item->opacity() == 0)
        _duration = 0;
    else
    {
        _item->show();
        _duration = time;
    }
}

int CanvasFadeAnimation::duration() const
{
    return _duration;
}

int CanvasFadeAnimation::totalDuration() const
{
    return _duration;
}

void CanvasFadeAnimation::updateCurrentTime(int time)
{
    if (_duration == 0)
      return;

    float value;

    if (_show)
      value = float(time)/_duration;
    else
      value = 1.0-(float(time)/_duration);

    _item->setOpacity(value);
}

void CanvasFadeAnimation::updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
{
    Q_UNUSED(newState);
    Q_UNUSED(oldState);
}

void CanvasFadeAnimation::updateDirection(QAbstractAnimation::Direction direction)
{
    Q_UNUSED(direction);
}

END_NAMESPACE_PATCHCANVAS
