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

class Utils: public QObject
{
  Q_OBJECT
public:
  static void registerQml() {
    qmlRegisterSingletonType<Utils>("beats.utils", 1, 0, "BeatUtils",
                                    [](QQmlEngine*, QJSEngine *) -> QObject * {
      static Utils *s_qmlInstance = nullptr;
      if(!s_qmlInstance) { s_qmlInstance = new Utils(); }
      return s_qmlInstance;
    });
  }

  Utils();

  template<typename T, int N>
  static QVarLengthArray<T, 1> toMono(QVarLengthArray<T, N> &channels)
  {
    QVarLengthArray<T, 1> mono(1);
    auto &value = mono[0];
    value = 0;
    for(auto &i : channels) {
      value += i;
    }
    value /= channels.size();

    return mono;
  }

  Q_INVOKABLE QVariantMap calculateVertices(int numEdges, areal size, areal baseAngle);

private:
};

#endif // UTILS_H
