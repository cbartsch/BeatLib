#include "peakfinder.h"
#include "utils.h"

PeakFinder::PeakFinder(QObject *parent) :
  AudioEffect(parent)
{
}

bool PeakFinder::processSample(QVarLengthArray<areal, 2> &channels, quint64 /*sampleIndex*/, areal /*maxValue*/, areal /*minValue*/)
{
  auto mono = Utils::toMono<areal, 2>(channels);   //compute mono signal by arithmetic mean
  areal &value = mono[0];

  areal diff = value - m_lastValue; //the differential of the signal

  if((diff > 0 && m_lastDiff < 0) || (diff < 0 && m_lastDiff > 0)) {//peak of signal found?
    m_peakValue = value;
  } else {
    value = m_peakValue;
  }

  m_lastValue = value;
  m_lastDiff = diff;

  return true;
}
