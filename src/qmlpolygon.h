#ifndef QMLPOLYGON_H
#define QMLPOLYGON_H

#include <QObject>
#include <QPolygon>
#include <QGraphicsPolygonItem>
#include <QQuickPaintedItem>
#include <QStyleOptionGraphicsItem>

#include "utils.h"

class QmlPolygon : public QQuickItem
{
  Q_OBJECT
  Q_PROPERTY(QVariantList vertices READ vertices WRITE setVertices)
  Q_PROPERTY(areal width READ width NOTIFY widthChanged)
  Q_PROPERTY(areal height READ height NOTIFY heightChanged)
  QML_ELEMENT

public:
  explicit QmlPolygon(QQuickItem *parent = nullptr);

  QVariantList vertices() const;
  void setVertices(const QVariantList &vertices);

  Q_INVOKABLE bool intersects(QmlPolygon *other);
  void createPolygon();

  areal width() const;
  areal height() const;

signals:
  void widthChanged(areal width);
  void heightChanged(areal height);

public slots:

private:
  QVariantList m_vertices;
  QPolygonF m_polygonData;
};

#endif // QMLPOLYGON_H
