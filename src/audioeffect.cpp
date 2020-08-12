 #include "audioeffect.h"

AudioEffect::AudioEffect(QObject *parent) : QObject(parent)
{
}

void AudioEffect::start(const QAudioFormat &format)
{
  m_sampleRate = format.sampleRate();

  // can be overridden
}

void AudioEffect::resetState()
{
  for(auto timer : m_timers) {
    QMetaObject::invokeMethod(timer.get(), "stop");
  }
  m_timers.clear();
}

bool AudioEffect::enabled() const
{
  return m_enabled;
}

void AudioEffect::setEnabled(bool enabled)
{
  m_enabled = enabled;
}

qint64 AudioEffect::startTime() const
{
  return m_startTime;
}

void AudioEffect::setStartTime(const qint64 &startTime)
{
  m_startTime = startTime;
}

void AudioEffect::notifyAtTime(quint64 sampleIndex, int delayMs, int maxTimeDiffMs, std::function<void ()> func)
{
  qint64 elapsedTime = QDateTime::currentMSecsSinceEpoch() - m_startTime;
  qint64 sampleTime = qint64(sampleIndex) * 1000L / m_sampleRate;
  qint64 timeDiff = qMax(0ll, sampleTime - (elapsedTime + delayMs)); //calculate at which point exactly this sample will be played

  //qDebug() << "volume:" << volume << "in" << timeDiff << "ms" << sampleTime << elapsedTime << m_filterDelayMs;
  if(timeDiff >= 0 && timeDiff < maxTimeDiffMs) {
    std::shared_ptr<QTimer> timer(new QTimer());
    m_timers.append(timer);
    timer->setSingleShot(true);

    connect(timer.get(), &QTimer::timeout, [=]() {
      //qDebug() << "set volume to" << volume << "now";
      func();
      m_timers.removeOne(timer);
    });
    timer->start(int(timeDiff));

    m_lastNotifySampleIndex = sampleIndex;
  }
}
