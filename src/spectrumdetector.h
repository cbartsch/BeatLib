#ifndef SPECTRUMDETECTOR_H
#define SPECTRUMDETECTOR_H

#include <QObject>

#include "audioeffect.h"

#include <complex>
#include "fftw3.h"

struct SpectrumData {
  Q_GADGET
  Q_PROPERTY(double maxFrequency MEMBER maxFrequency CONSTANT)
  Q_PROPERTY(double meanFrequency MEMBER meanFrequency CONSTANT)
  Q_PROPERTY(double dcValue MEMBER dcValue CONSTANT)
  QML_VALUE_TYPE(spectrumData)

public:
  double maxFrequency, meanFrequency, dcValue;

  auto operator==(const SpectrumData& o) const {
    return maxFrequency == o.maxFrequency && dcValue == o.dcValue;
  };
  auto operator!=(const SpectrumData& o) const {
    return !(*this == o);
  }
};

Q_DECLARE_METATYPE(SpectrumData);

class SpectrumDetector : public AudioEffect
{
  Q_OBJECT
  Q_PROPERTY(int spectrumSize READ spectrumSize WRITE setSpectrumSize NOTIFY spectrumSizeChanged)
  Q_PROPERTY(double startFrequency MEMBER m_startFrequency NOTIFY startFrequencyChanged)
  Q_PROPERTY(AudioEffect* preEffect MEMBER m_preEffect NOTIFY preEffectChanged)
  Q_PROPERTY(SpectrumData spectrumData READ spectrumData WRITE setSpectrumData NOTIFY spectrumDataChanged)
  QML_ELEMENT

public:
  explicit SpectrumDetector(QObject *parent = nullptr);
  ~SpectrumDetector();

  virtual void start(const QAudioFormat &);

  virtual bool processSample(QVarLengthArray<areal, 2> &channels, quint64 sampleIndex, areal maxValue, areal minValue);

  int spectrumSize() const;

  SpectrumData spectrumData() const;
  SpectrumData currentSpectrumData();

public slots:
  void setSpectrumSize(int spectrumSize);
  void setSpectrumData(SpectrumData spectrumData);

signals:
  void spectrumSizeChanged(int spectrumSize);
  void preEffectChanged(AudioEffect* preEffect);
  void spectrumDataChanged(SpectrumData spectrumData);
  void startFrequencyChanged(double startFrequency);

private:
  void destroyFftPlan();

  SpectrumData m_currentSpectrumData { 0, 0, 0 };
  SpectrumData m_spectrumData { 0, 0, 0 };

  AudioEffect *m_preEffect = nullptr;

  int m_spectrumSize = 4096;
  double m_startFrequency = 250;

  int m_currentIndex = -1;
  fftw_plan m_plan = nullptr;
  double *m_input = nullptr;
  std::complex<double> *m_output = nullptr;
};

#endif // SPECTRUMDETECTOR_H
