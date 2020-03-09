#ifndef MULTIEFFECT_H
#define MULTIEFFECT_H

#include "audioeffect.h"

class MultiEffect : public AudioEffect
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> effects READ effects WRITE setEffects DESIGNABLE true)
public:
  static void registerQml() {
    qmlRegisterType<MultiEffect>("beats", 1, 0, "MultiEffect");
  }

  explicit MultiEffect();

  QList<QVariant> effects() const;
  void setEffects(const QList<QVariant> &effects);

  virtual void start(const QAudioFormat &);
  virtual bool processSample(QVarLengthArray<areal, 1> &channels, quint64 sampleIndex, areal maxValue, areal minValue);

private:
  QList<QVariant> m_effects;

};

#endif // MULTIEFFECT_H
