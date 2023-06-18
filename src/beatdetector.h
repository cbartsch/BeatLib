#ifndef BEATDETECTOR_H
#define BEATDETECTOR_H

#include "audioeffect.h"
#include "directform2filter.h"
#include "volumedetector.h"
#include "spectrumdetector.h"
#include "utils.h"

class BeatDetector : public AudioEffect
{
  Q_OBJECT
  Q_PROPERTY(AudioEffect* preEffect READ preEffect WRITE setPreEffect)
  Q_PROPERTY(AudioEffect* envelopeDetector READ envDetector WRITE setEnvDetector)
  Q_PROPERTY(areal minTimeDistanceMs READ minTimeDistanceMs WRITE setMinTimeDistanceMs)
  Q_PROPERTY(int beatHalfLifeTimeMs READ beatHalfLifeTimeMs WRITE setBeatHalfLifeTimeMs)
  Q_PROPERTY(VolumeDetector *volumeDetector MEMBER m_volumeDetector)
  Q_PROPERTY(SpectrumDetector *spectrumDetector MEMBER m_spectrumDetector)
  QML_ELEMENT

public:
  BeatDetector();

  virtual void start(const QAudioFormat &);
  virtual bool processSample(QVarLengthArray<areal, 2> &channels, quint64 sampleIndex, areal maxValue, areal minValue);

  AudioEffect *preEffect() const;
  void setPreEffect(AudioEffect *preEffect);

  areal minTimeDistanceMs() const;
  void setMinTimeDistanceMs(areal minTimeDistanceMs);

  void setBeatHalfLifeTimeMs(int time);
  areal beatHalfLifeTimeMs() const;
  void updateMinTimeSamples();

  Q_INVOKABLE void resetState();
  AudioEffect *envDetector() const;
  void setEnvDetector(AudioEffect *envDetector);

private:
  AudioEffect *m_preEffect = nullptr;
  AudioEffect *m_envDetector = nullptr;
  VolumeDetector *m_volumeDetector = nullptr;
  SpectrumDetector *m_spectrumDetector = nullptr;

  areal m_lastValue = 0, m_lastDiff = 0, m_maxDiff = 0;
  bool m_rising = true;

  quint64 m_lastIndex = 0;
  areal m_minTimeDistanceMs = 0;
  quint64 m_minTimeDistanceSamples = 0;

  // damps maxDiff instantly, 1 == unused right now
  areal m_maxGap = 1;//0.95;

  // this will make maxDiff halve in value after 1s
  int m_beatHalfLifeTimeMs = 1000;
  areal m_fadeFactor = 0;

  void updateFadeFactor();
signals:
  void beatDetected(quint64 currentTime, quint64 sampleIndex,
                    qreal envValue, qreal diffValue, qreal maxValue,
                    qreal volume, SpectrumData spectrumData);
};

#endif // BEATDETECTOR_H
