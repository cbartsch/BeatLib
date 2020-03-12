 #include "audioeffect.h"

AudioEffect::AudioEffect(QObject *parent) : QObject(parent)
{
}

void AudioEffect::start(const QAudioFormat &)
{
  // does nothing, can be overridden
}

bool AudioEffect::enabled() const
{
  return m_enabled;
}

void AudioEffect::setEnabled(bool enabled)
{
  m_enabled = enabled;
}

