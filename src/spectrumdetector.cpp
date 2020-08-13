#include "spectrumdetector.h"

SpectrumDetector::SpectrumDetector(QObject *parent) : AudioEffect(parent)
{

}

SpectrumDetector::~SpectrumDetector()
{
  fftw_destroy_plan(m_plan);
  fftw_free(m_input);
  fftw_free(m_output);
}

void SpectrumDetector::start(const QAudioFormat &format)
{
  qDebug() << "Initialize FFT with n =" << m_spectrumSize;

  AudioEffect::start(format);

  m_currentIndex = 0;

  m_input = (double*) fftw_malloc(sizeof(double) * m_spectrumSize);
  m_output = (std::complex<double>*) fftw_malloc(sizeof(std::complex<double>) * m_spectrumSize);
  m_plan = fftw_plan_dft_r2c_1d(m_spectrumSize, m_input, reinterpret_cast<fftw_complex*>(m_output), FFTW_ESTIMATE);
}

bool SpectrumDetector::processSample(QVarLengthArray<areal, 2> &channels, quint64 sampleIndex, areal maxValue, areal minValue)
{
  auto mono = Utils::toMono(channels);

  if(m_preEffect) { //apply pre-effect, if set
    m_preEffect->processSample(mono, sampleIndex, maxValue, minValue);
  }

  double monoValue = mono[0];

  // normalize sample to range [-1, 1]:
  m_input[m_currentIndex] = (monoValue - minValue) * 2.0 / (maxValue - minValue) - 1;

  m_currentIndex++;

  if(m_currentIndex == m_spectrumSize) {
    // buffer is full -> execute FFT now
    fftw_execute(m_plan);

    std::complex<double> dc = m_output[0];

    int nSmooth = 1;
    double max = 0, maxIndex = 0, mean = 0, sum = 0;

    int startIndex = m_spectrumSize * m_startFrequency / m_sampleRate;

    for(int i = 1; i < m_spectrumSize / 2 - (nSmooth - 1); i++) {
      double smoothed = 0;

      for(int j = 0; j < nSmooth; j++) {
        auto value = m_output[i + j];
        auto abs = std::abs(value);

        smoothed += abs;
      }

      smoothed /= nSmooth;

      if(i >= startIndex && smoothed > max) {
        max = smoothed;
        maxIndex = i;
      }

      mean += i * smoothed * smoothed;
      sum += smoothed * smoothed;
    }

    double maxFrequency = (double)m_sampleRate * maxIndex / m_spectrumSize;
    double meanFrequency = (double)m_sampleRate * mean / sum / m_spectrumSize;

   // qDebug() << "mean and sum" << mean << sum << meanFrequency;
   // qDebug() << "max index" << max << maxIndex;

    m_currentIndex = 0;

    notifyAtTime(sampleIndex, 0, 10000, [=]() {
      SpectrumData s;
      s.maxFrequency = maxFrequency;
      s.meanFrequency = meanFrequency;
      s.dcValue = std::abs(dc);
      setSpectrumData(s);
    });
  }

  return false;
}

int SpectrumDetector::spectrumSize() const
{
  return m_spectrumSize;
}

SpectrumData SpectrumDetector::spectrumData() const
{
  return m_spectrumData;
}

void SpectrumDetector::setSpectrumSize(int spectrumSize)
{
  if (m_spectrumSize == spectrumSize)
    return;

  if(m_currentIndex != -1) {
    qWarning() << "SpectrumDetector: Cannot change spectrumSize.";
    return;
  }

  m_spectrumSize = spectrumSize;
  emit spectrumSizeChanged(m_spectrumSize);
}

void SpectrumDetector::setSpectrumData(SpectrumData spectrumData)
{
  if (m_spectrumData == spectrumData)
    return;

  m_spectrumData = spectrumData;
  emit spectrumDataChanged(m_spectrumData);
}
