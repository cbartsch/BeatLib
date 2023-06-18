#include "volumedetector.h"
#include "utils.h"

VolumeDetector::VolumeDetector(QObject *parent) :
  DirectForm2Filter(parent), m_peakFinder(parent)
{
}

void VolumeDetector::start(const QAudioFormat &format)
{
  DirectForm2Filter::start(format);
  setVolume(0);
}

void VolumeDetector::setVolume(areal volume)
{
  m_volume = volume;
  emit volumeChanged();
}

bool VolumeDetector::processSample(QVarLengthArray<areal, 2> &channels, quint64 sampleIndex, areal maxValue, areal minValue)
{
  auto mono = Utils::toMono<areal, 2>(channels);
  areal &volume = mono[0];
  volume = qAbs(volume / maxValue);

  m_peakFinder.processSample(mono, sampleIndex, maxValue, minValue);
  DirectForm2Filter::processSample(mono, sampleIndex, maxValue, minValue);

  if(m_startTime > 0 && (sampleIndex - m_lastNotifySampleIndex) / areal(m_sampleRate) >= m_updateIntervalMs / 1000.0) {
    notifyAtTime(sampleIndex, m_filterDelayMs, 10000, [this, volume]() {
      setVolume(volume);
    });
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

void VolumeDetector::resetState()
{
  DirectForm2Filter::resetState();
  setVolume(0);
}


