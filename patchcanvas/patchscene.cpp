#include "patchscene.h"

#include <QGraphicsView>
#include <cmath>

PatchScene::PatchScene()
{
    ctrl_down = false;
    mouse_down = false;
    fake_selection = false;
    fake_rubberband = addRect(QRectF(0, 0, 0, 0));
    fake_rubberband->setZValue(-1);
    fake_rubberband->hide();
    orig_point = QPointF(0, 0);
}

void PatchScene::rubberbandByTheme(Theme* theme)
{
    fake_rubberband->setPen(theme->rubberband_pen);
    fake_rubberband->setBrush(theme->rubberband_brush);
}

void PatchScene::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Control)
      ctrl_down = true;
    QGraphicsScene::keyPressEvent(event);
}

void PatchScene::keyReleaseEvent(QKeyEvent* event)
{
    ctrl_down = false;
    QGraphicsScene::keyReleaseEvent(event);
}

void PatchScene::wheelEvent(QGraphicsSceneWheelEvent* event)
{
    if (ctrl_down)
    {
      double factor = pow(1.41, (event->delta()/240.0));
      QGraphicsView* const view = views().at(0);
      view->scale(factor, factor);
      event->accept();
    }
    else
      QGraphicsScene::wheelEvent(event);
}

void PatchScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
      mouse_down = true;
    else
      mouse_down = false;

    QGraphicsScene::mousePressEvent(event);
}

void PatchScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (selectedItems().count() > 0)
      return QGraphicsScene::mouseMoveEvent(event);
    else if (mouse_down)
    {
      if (!fake_selection)
      {
        orig_point = event->scenePos();
        fake_rubberband->show();
        fake_selection = true;
      }

      QPointF pos = event->scenePos();
      int x, y;

      if (pos.x() > orig_point.x()) x = orig_point.x();
      else x = pos.x();

      if (pos.y() > orig_point.y()) y = orig_point.y();
      else y = pos.y();

      fake_rubberband->setRect(x, y, abs(pos.x()-orig_point.x()), abs(pos.y()-orig_point.y()));
      event->accept();
    }
}

void PatchScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (fake_selection)
    {
//      QList<QGraphicsItem*> items = items();
//      if (len(items) > 0):
//        for i in range(len(items)):
//          if (items[i].isVisible() and type(items[i]) == CanvasBox):
//            item_rect = items[i].sceneBoundingRect();
//            if ( self.fake_rubberband.contains(QPointF(item_rect.x(), item_rect.y())) and
//                 self.fake_rubberband.contains(QPointF(item_rect.x()+item_rect.width(), item_rect.y()+item_rect.height())) )
//              items[i].setSelected(true);

        fake_rubberband->hide();
        fake_rubberband->setRect(0, 0, 0, 0);
        fake_selection = false;
    }
    else
    {
//        bool update = false;
//      ret_items = [];
        QList<QGraphicsItem*> items = selectedItems();

        if (items.count() > 0)
        {
//        for i in range(len(items)):
//          if (items[i].isVisible() and type(items[i]) == CanvasBox):
//            items[i].checkItemPos()
//            use_xy2 = True if (items[i].splitted and items[i].splitted_mode == PORT_MODE_INPUT) else False
//            ret_items.append((items[i].group_id, items[i].scenePos(), use_xy2))
//            update = True
        }

//      if (update)
//        emit(SIGNAL("sceneItemsMoved"), ret_items);
    }

    mouse_down = false;
    QGraphicsScene::mouseReleaseEvent(event);
}
