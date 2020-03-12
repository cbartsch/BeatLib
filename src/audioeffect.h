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
public:
  AudioEffect(QObject *parent = nullptr);

  virtual void start(const QAudioFormat &);

  //return true if any of the values in channels have changed
  virtual bool processSample(QVarLengthArray<areal, 2> &channels, quint64 sampleIndex, areal maxValue, areal minValue) = 0;

  bool enabled() const;
  void setEnabled(bool enabled);

private:
  bool m_enabled = true;
};

#endif // AUDIOEFFECT_H
