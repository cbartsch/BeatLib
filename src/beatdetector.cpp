#include "beatdetector.h"
#include "utils.h"

BeatDetector::BeatDetector()
{
}

void BeatDetector::start(const QAudioFormat &format)
{
  m_sampleRate = format.sampleRate();
  updateFadeFactor();
  updateMinTimeSamples();
  QAudioFormat mono(format); //copy format
  mono.setChannelCount(1); //only need to analyze mono channel of audio signal

  if(m_preEffect) { m_preEffect->start(mono); }
  if(m_envDetector) { m_envDetector->start(mono); }
}


bool BeatDetector::processSample(QVarLengthArray<areal, 2> &channels, quint64 sampleIndex, areal maxValue, areal minValue)
{
  QVarLengthArray<areal, 2> mono = Utils::toMono(channels);   //compute mono signal by arithmetic mean
  areal &value = mono[0];

  if(m_preEffect) { //apply pre-effect, if set
    m_preEffect->processSample(mono, sampleIndex, maxValue, minValue);
  }

  value = qAbs(value);  //take absolute of value

  //compute envelope, if detector is set
  if(m_envDetector) {
    m_envDetector->processSample(mono, sampleIndex, maxValue, minValue);
  }

  areal diff = value - m_lastValue; //the differential of the envelope = change in amplitude
  areal diff2 = diff - m_lastDiff;  //the change of the differential
  bool rising = diff2 > 0;

  if(sampleIndex - m_lastIndex > m_minTimeDistanceSamples) { //min time distance kept?)
    if(diff > m_maxDiff //diff big enough compared to largest diff up to now?
       && !rising && m_rising //extreme value of diff found?
       ) { //if all true -> beat detected!
      m_lastIndex = sampleIndex;
      areal volume = m_volumeDetector ? m_volumeDetector->currentVolume() : 0;
      emit beatDetected(QDateTime::currentMSecsSinceEpoch(), sampleIndex, value, diff, maxValue, volume);
    } else {
    }
  }
  m_maxDiff = qMax(m_maxDiff, diff);
  m_maxDiff *= m_fadeFactor;

  m_rising = rising;
  m_lastValue = value;
  m_lastDiff = diff;

  return false;
}
AudioEffect *BeatDetector::preEffect() const
{
  return m_preEffect;
}

void BeatDetector::setPreEffect(AudioEffect *preEffect)
{
  m_preEffect = preEffect;
}
areal BeatDetector::minTimeDistanceMs() const
{
  return m_minTimeDistanceSamples;
}

void BeatDetector::updateMinTimeSamples()
{
  if(m_sampleRate > 0) {
    m_minTimeDistanceSamples = m_minTimeDistanceMs * m_sampleRate / 1000;
  }
}

void BeatDetector::resetState()
{
  m_lastValue = 0;
  m_lastDiff = 0;
  m_lastIndex = 0;
}

AudioEffect *BeatDetector::envDetector() const
{
  return m_envDetector;
}

void BeatDetector::setEnvDetector(AudioEffect *envDetector)
{
  m_envDetector = envDetector;
}


void BeatDetector::setMinTimeDistanceMs(areal minTimeDistance)
{
  m_minTimeDistanceMs = minTimeDistance;

  updateMinTimeSamples();
}

void BeatDetector::updateFadeFactor()
{
  if(m_sampleRate > 0) {
    areal samples = m_beatHalfLifeTimeMs / 1000.0 * m_sampleRate;

    //this will make maxDiff halve in value after the number of samples
    m_fadeFactor = qPow(0.5, 1/samples);
  }
}

void BeatDetector::setBeatHalfLifeTimeMs(int time)
{
  m_beatHalfLifeTimeMs = time;
  updateFadeFactor();
}

areal BeatDetector::beatHalfLifeTimeMs() const
{
  return m_beatHalfLifeTimeMs;
}


