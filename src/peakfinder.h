#ifndef PEAKFINDER_H
#define PEAKFINDER_H

#include "audioeffect.h"

class PeakFinder : public AudioEffect
{
  Q_OBJECT
  QML_ELEMENT

public:
  explicit PeakFinder(QObject *parent = nullptr);

  virtual bool processSample(QVarLengthArray<areal, 2> &channels, quint64 sampleIndex, areal maxValue, areal minValue);

signals:

public slots:

private:
  areal m_lastValue = 0, m_lastDiff = 0;
  areal m_peakValue = 0;

};

#endif // PEAKFINDER_H
