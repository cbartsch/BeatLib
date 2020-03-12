#include "utils.h"

Utils::Utils()
{
}

QVariantMap Utils::calculateVertices(int numEdges, areal size, areal baseAngle) {
  QSizeF bounds;
  QVariantList verts;
  areal edgeAngle = 2 * M_PI / numEdges;
  areal sz = size / 2 / qSin(edgeAngle / 2);

  // areal maxY = 0;
  for(int i = 0; i < numEdges; i++) {
    areal angle = i * edgeAngle + baseAngle;
    areal x = sz * (1 + qSin(angle));
    areal y = sz * (1 - qCos(angle));
    bounds.setWidth(qMax(areal(bounds.width()), x));
    bounds.setHeight(qMax(areal(bounds.height()), y));
    // maxY = qMax(maxY, y);
    verts.append(QVariant(QPointF(x, y)));
  }

  return QVariantMap {
    { "bounds", QVariant(bounds) },
    { "vertices", verts }
  };
}
