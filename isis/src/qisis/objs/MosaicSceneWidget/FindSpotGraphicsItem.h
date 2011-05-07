#ifndef FindSpotGraphicsItem_h
#define FindSpotGraphicsItem_h

#include <QGraphicsEllipseItem>

class QPointF;

namespace Isis {
  class MosaicSceneWidget;

  /**
   * @brief The visual display of the find point
   *
   * @author 2011-05-07 Steven Lambright
   */
  class FindSpotGraphicsItem : public QGraphicsEllipseItem {
    public:
      FindSpotGraphicsItem(QPointF center,
                           MosaicSceneWidget *boundingRectSrc);
      virtual ~FindSpotGraphicsItem();

      QRectF boundingRect() const;
      void paint(QPainter *, const QStyleOptionGraphicsItem *,
                 QWidget * widget = 0);
    private:
      QRectF calcRect() const;
      QPointF *p_centerPoint;
      MosaicSceneWidget *p_mosaicScene;
  };
}

#endif

