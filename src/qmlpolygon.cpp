#include "qmlpolygon.h"
#include <QBrush>
#include <QPen>

QmlPolygon::QmlPolygon(QQuickItem *parent) :
  QQuickItem(parent)
{
}

QVariantList QmlPolygon::vertices() const {
  return m_vertices;
}

void QmlPolygon::createPolygon()
{
  m_polygonData.clear();
  for(QVariant vert : m_vertices) {
    QPointF point = vert.value<QPointF>();
    m_polygonData << point;
  }
  QRectF bounds = m_polygonData.boundingRect();
  setSize(QSizeF(bounds.width(), bounds.height()));
  setPosition(QPointF(bounds.left(), bounds.top()));
}

areal QmlPolygon::width() const
{
  return m_polygonData.boundingRect().width();
}

areal QmlPolygon::height() const
{
  return m_polygonData.boundingRect().height();
}

void QmlPolygon::setVertices(const QVariantList &vertices) {
  m_vertices = vertices;

  createPolygon();

  emit widthChanged(width());
  emit heightChanged(height());
}

bool QmlPolygon::intersects(QmlPolygon *other)
{
  if(other) {
    QRectF pos = mapRectToScene(this->boundingRect());
    QRectF otherPos = other->mapRectToScene(other->boundingRect());
    if(pos.intersects(otherPos)) {
      auto trans = m_polygonData.translated(pos.x(), pos.y());
      auto otherTrans = other->m_polygonData.translated(otherPos.x(), otherPos.y());

      //point in polygon comparison
      for(QPointF p : other->m_polygonData) {
        if(m_polygonData.containsPoint(p, Qt::FillRule::OddEvenFill)) {
          return true;
        }
      }
      for(QPointF p : m_polygonData) {
        if(other->m_polygonData.containsPoint(p, Qt::FillRule::OddEvenFill)) {
          return true;
        }
      }

      //line intersection comparison
      QPolygonF intersects = trans.intersected(otherTrans);
      return intersects.size() > 0;
    }
  }
  return false;
}
