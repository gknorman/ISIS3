#include "MosaicGraphicsView.h"

#include <iostream>

#include <QResizeEvent>

namespace Isis {
  MosaicGraphicsView::MosaicGraphicsView(QGraphicsScene *scene,
      QWidget *parent) : QGraphicsView(scene, parent) {
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    p_resizeZooming = true;
  }

  MosaicGraphicsView::~MosaicGraphicsView() {
  }


  void MosaicGraphicsView::contextMenuEvent(QContextMenuEvent *event) {
    QGraphicsView::contextMenuEvent(event);
  }


  void MosaicGraphicsView::resizeEvent(QResizeEvent *event) {
    if(event->oldSize().isEmpty() || p_resizeZooming) {
      QRectF sceneRect(scene()->itemsBoundingRect());
      fitInView(sceneRect, Qt::KeepAspectRatio);
    }

    QGraphicsView::resizeEvent(event);
  }
}

