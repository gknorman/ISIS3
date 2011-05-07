#ifndef ControlPointGraphicsItem_h
#define ControlPointGraphicsItem_h

#include <QGraphicsRectItem>

class QPointF;

namespace Isis {
  class ControlPoint;
  class MosaicSceneWidget;

  /**
   * @brief The visual display of a single control point
   *
   * @author 2011-05-07 Steven Lambright
   */
  class ControlPointGraphicsItem : public QGraphicsRectItem {
    public:
      ControlPointGraphicsItem(QPointF center, ControlPoint *cp,
          MosaicSceneWidget *scene, QGraphicsItem *parent);
      virtual ~ControlPointGraphicsItem();

      QRectF boundingRect() const;
      void paint(QPainter *, const QStyleOptionGraphicsItem *,
                 QWidget * widget = 0);

    protected:

    private:
      QRectF calcRect() const;
      QPointF *p_centerPoint;
      MosaicSceneWidget *p_mosaicScene;
      ControlPoint *p_controlPoint;
  };
}

#endif

