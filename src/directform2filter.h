#ifndef DIRECTFORM2FILTER_H
#define DIRECTFORM2FILTER_H

#include <audioeffect.h>
#include <QList>

class DirectForm2Filter : public AudioEffect
{
  Q_OBJECT
  Q_PROPERTY(const QVariantMap & data WRITE setData DESIGNABLE true)

public:
  DirectForm2Filter(QObject *parent = nullptr);

  virtual void start(const QAudioFormat &format) Q_DECL_OVERRIDE;
  virtual bool processSample(QVarLengthArray<areal, 2> &channels, quint64 sampleIndex, areal maxValue, areal minValue) Q_DECL_OVERRIDE;

  Q_INVOKABLE void set(int order, QList<QVariant> sosMatrix, QList<QVariant> scaleValues);
  void set(int order, const double sosMatrix[][6], const double scaleValues[]);

  Q_INVOKABLE virtual void resetState() Q_DECL_OVERRIDE;

  void setData(const QVariantMap &data);

private:
  struct sosEntry { //second order sections entry
    double values[6];
  };

  void setOrder(int order);

  inline void incrementMemoryIndex(int value);
  inline int memoryIndex(int offset = 0);
  inline double &memory(int channel, int section, int offset = 0);

  int m_order = 0, m_sections = 0;
  QVarLengthArray<sosEntry> m_sosMatrix;
  QVarLengthArray<double> m_scaleValues;

  QVarLengthArray<double> m_memory;
  int m_memoryIndex = 0;
};

#endif // DIRECTFORM2FILTER_H
