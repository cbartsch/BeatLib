#include "multieffect.h"

MultiEffect::MultiEffect()
{
}

QList<QVariant> MultiEffect::effects() const
{
  return m_effects;
}

void MultiEffect::setEffects(const QList<QVariant> &effects)
{
  m_effects = effects;
}

void MultiEffect::start(const QAudioFormat &format) {
  AudioEffect::start(format);

  for(QVariant effect : m_effects) {
    effect.value<AudioEffect *>()->start(format);
  }
}

bool MultiEffect::processSample(QVarLengthArray<areal, 2> &channels, quint64 sampleIndex, areal maxValue, areal minValue)
{
  bool res = false;
  for(auto effect : m_effects) {
    AudioEffect * eff = effect.value<AudioEffect *>();
    res |= eff->enabled() &&
        eff->processSample(channels, sampleIndex, maxValue, minValue);
  }
  return res;
}

