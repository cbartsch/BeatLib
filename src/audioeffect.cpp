 #include "audioeffect.h"

AudioEffect::AudioEffect(QObject *parent) : QObject(parent)
{
}

AudioEffect::~AudioEffect()
{

}

bool AudioEffect::enabled() const
{
  return m_enabled;
}

void AudioEffect::setEnabled(bool enabled)
{
  m_enabled = enabled;
}

