#include "volumedetector.h"
#include "utils.h"

VolumeDetector::VolumeDetector(QObject *parent) :
  DirectForm2Filter(parent), m_peakFinder(parent)
{
}

void VolumeDetector::start(const QAudioFormat &format)
{
  m_sampleRate = format.sampleRate();
  setVolume(0);
  DirectForm2Filter::start(format);
}

void VolumeDetector::setVolume(areal volume)
{
  m_volume = volume;
  emit volumeChanged();
}

bool VolumeDetector::processSample(QVarLengthArray<areal, 1> &channels, quint64 sampleIndex, areal maxValue, areal minValue)
{
  auto mono = Utils::toMono(channels);
  areal &volume = mono[0];
  volume = qAbs(volume / maxValue);

  m_peakFinder.processSample(mono, sampleIndex, maxValue, minValue);
  DirectForm2Filter::processSample(mono, sampleIndex, maxValue, minValue);

  if(m_startTime > 0 && (sampleIndex - m_lastNotifySampleIndex) / (areal)m_sampleRate >= m_updateIntervalMs / 1000.0) {

    qint64 elapsedTime = QDateTime::currentMSecsSinceEpoch() - m_startTime;
    qint64 sampleTime = (qint64)sampleIndex * 1000L / m_sampleRate;
    qint64 timeDiff = qMax((qint64)0, sampleTime - (elapsedTime + m_filterDelayMs)); //calculate at which point exactly this sample will be played

    //qDebug() << "volume:" << volume << "in" << timeDiff << "ms" << sampleTime << elapsedTime << m_filterDelayMs;
    if(timeDiff >= 0 && timeDiff < 10000) {
      std::shared_ptr<QTimer> timer(new QTimer());
      m_timers.append(timer);
      timer->setSingleShot(true);

      connect(timer.get(), &QTimer::timeout, [=]() {
        //qDebug() << "set volume to" << volume << "now";
        setVolume(volume);
        m_timers.removeOne(timer);
      });
      timer->start(timeDiff);

      m_lastNotifySampleIndex = sampleIndex;
    }

  }

  m_currentVolume = volume;

  return false;
}
int VolumeDetector::updateIntervalMs() const
{
  return m_updateIntervalMs;
}

void VolumeDetector::setUpdateIntervalMs(int updateIntervalMs)
{
  if(updateIntervalMs != m_updateIntervalMs) {
    m_updateIntervalMs = updateIntervalMs;
    emit updateIntervalChanged();
  }
}

areal VolumeDetector::volume() const
{
  return m_volume;
}

areal VolumeDetector::currentVolume() const
{
  return m_currentVolume;
}

int VolumeDetector::filterDelayMs() const
{
  return m_filterDelayMs;
}

void VolumeDetector::setFilterDelayMs(int filterDelayMs)
{
  m_filterDelayMs = filterDelayMs;
}
qint64 VolumeDetector::startTime() const
{
  return m_startTime;
}

void VolumeDetector::setStartTime(const qint64 &startTime)
{
  m_startTime = startTime;
}

void VolumeDetector::resetState()
{
  for(auto timer : m_timers) {
    QMetaObject::invokeMethod(timer.get(), "stop");
  }
  m_timers.clear();
  DirectForm2Filter::resetState();
  setVolume(0);
}


