#ifndef VOLUMEDETECTOR_H
#define VOLUMEDETECTOR_H

#include "directform2filter.h"
#include "peakfinder.h"
#include <memory>

class VolumeDetector : public DirectForm2Filter
{
  Q_OBJECT
  Q_PROPERTY(int filterDelayMs READ filterDelayMs WRITE setFilterDelayMs)
  Q_PROPERTY(int updateIntervalMs READ updateIntervalMs WRITE setUpdateIntervalMs NOTIFY updateIntervalChanged)
  Q_PROPERTY(areal volume READ volume NOTIFY volumeChanged)
  QML_ELEMENT

public:
  explicit VolumeDetector(QObject *parent = nullptr);

  virtual void start(const QAudioFormat &format);
  virtual bool processSample(QVarLengthArray<areal, 2> &channels, quint64 sampleIndex, areal maxValue, areal minValue);

  int updateIntervalMs() const;
  void setUpdateIntervalMs(int updateIntervalMs);

  areal volume() const;
  areal currentVolume() const;

  int filterDelayMs() const;
  void setFilterDelayMs(int filterDelayMs);

  Q_INVOKABLE virtual void resetState();

signals:
  void volumeChanged();
  void updateIntervalChanged();

private:
  void setVolume(areal volume);

  int m_updateIntervalMs = 1000;
  int m_filterDelayMs = 0;
  //currentVolume = lastly detected volume in processSample()
  //volume = actual volume at current point in time of playback
  areal m_volume = 0, m_currentVolume = 0;

  PeakFinder m_peakFinder;
};

#endif // VOLUMEDETECTOR_H
