#ifndef UTILS_H
#define UTILS_H

#include <QVarLengthArray>
#include <QObject>
#include <QtQml>
#include <QtMath>
#include <math.h>

#ifdef USE_FLOAT
typedef float areal;
#else
typedef qreal areal;
#endif

class Utils : public QObject
{
  Q_OBJECT
public:
  Utils();

  template<typename T, int N>
  static QVarLengthArray<T, 2> toMono(QVarLengthArray<T, N> &channels);

  Q_INVOKABLE QVariantMap calculateVertices(int numEdges, areal size, areal baseAngle);

private:
};

template<typename T, int N>
QVarLengthArray<T, 2> Utils::toMono(QVarLengthArray<T, N> &channels)
{
  QVarLengthArray<T, 2> mono(1);
  auto &value = mono[0];
  value = 0;
  for(auto &i : channels) {
    value += i;
  }
  value /= channels.size();

  return mono;
}

#endif // UTILS_H
