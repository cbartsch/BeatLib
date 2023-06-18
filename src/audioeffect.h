#ifndef AUDIOEFFECT_H
#define AUDIOEFFECT_H

#include <QObject>
#include <QVarLengthArray>
#include <QAudioFormat>

#include <QtQml>

#include "utils.h"

class AudioEffect : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled)
  Q_PROPERTY(qint64 startTime READ startTime WRITE setStartTime)
  QML_ELEMENT
  QML_UNCREATABLE("AudioEffect is abstract, only implementations can be instantiated.")

public:
  AudioEffect(QObject *parent = nullptr);

  virtual void start(const QAudioFormat &);

  //return true if any of the values in channels have changed
  virtual bool processSample(QVarLengthArray<areal, 2> &channels, quint64 sampleIndex, areal maxValue, areal minValue) = 0;

  Q_INVOKABLE virtual void resetState();

  bool enabled() const;
  void setEnabled(bool enabled);

  qint64 startTime() const;
  void setStartTime(const qint64 &startTime);

protected:
  void notifyAtTime(quint64 sampleIndex, int delayMs, int maxTimeDiffMs, std::function<void()> func);

  int m_sampleRate = 0;
  qint64 m_startTime = 0;
  quint64 m_lastNotifySampleIndex = 0;

private:
  bool m_enabled = true;

  QList<std::shared_ptr<QTimer>> m_timers;
};

#endif // AUDIOEFFECT_H
