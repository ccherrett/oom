#include "canvasicon.h"

#include <cstdio>

CanvasIcon::CanvasIcon(PatchCanvas::Icon icon, QString name, QGraphicsItem* parent, PatchCanvas::Canvas* canvas_) :
    QGraphicsSvgItem(parent),
    canvas(canvas_)
{
    renderer = 0;

    setIcon(icon, name);

    colorFX = new QGraphicsColorizeEffect(this);
    colorFX->setColor(canvas->theme->box_text.color());
    setGraphicsEffect(colorFX);
}

CanvasIcon::~CanvasIcon()
{
    if (renderer)
        delete renderer;
}

void CanvasIcon::setIcon(PatchCanvas::Icon icon, QString name)
{
    name = name.toLower();
    QString icon_path;

    if (icon == PatchCanvas::ICON_APPLICATION)
    {
      size = QRectF(3, 2, 19, 18);

      if (name.contains("audacious"))
      {
        size = QRectF(5, 4, 16, 16);
        icon_path = ":/svg/pb_audacious.svg";
      }
      else if (name.contains("clementine"))
      {
        size = QRectF(5, 4, 16, 16);
        icon_path = ":/svg/pb_clementine.svg";
      }
      else if (name.contains("jamin"))
      {
        size = QRectF(5, 3, 16, 16);
        icon_path = ":/svg/pb_jamin.svg";
      }
      else if (name.contains("mplayer"))
      {
        size = QRectF(5, 4, 16, 16);
        icon_path = ":/svg/pb_mplayer.svg";
      }
      else if (name.contains("vlc"))
      {
        size = QRectF(5, 3, 16, 16);
        icon_path = ":/svg/pb_vlc.svg";
      }
      else
      {
        size = QRectF(5, 3, 16, 16);
        icon_path = ":/svg/pb_generic.svg";
      }
    }
    else if (icon == PatchCanvas::ICON_HARDWARE)
    {
        size = QRectF(5, 2, 16, 16);
        icon_path = ":/svg/pb_hardware.svg";

    }
    else if (icon == PatchCanvas::ICON_LADISH_ROOM)
    {
        size = QRectF(5, 2, 16, 16);
        icon_path = ":/svg/pb_hardware.svg";
    }
    else
    {
      size = QRectF(0, 0, 0, 0);
      printf("PatchCanvas::CanvasIcon->setIcon() - Unsupported Icon requested\n");
      return;
    }

    if (renderer)
        delete renderer;

    renderer = new QSvgRenderer(icon_path, canvas->scene);
    setSharedRenderer(renderer);
    update();
}

QRectF CanvasIcon::boundingRect() const
{
    return QRectF(size);
}

void CanvasIcon::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    if (renderer)
    {
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->setRenderHint(QPainter::TextAntialiasing, false);
        renderer->render(painter, QRectF(size));
    }
    else
        QGraphicsSvgItem::paint(painter, option, widget);
}
