#include "directform2filter.h"

constexpr int B0 = 0;
constexpr int B1 = 1;
constexpr int B2 = 2;
constexpr int A1 = 4;
constexpr int A2 = 5;

DirectForm2Filter::DirectForm2Filter(QObject *parent) : AudioEffect(parent)
{
}

void DirectForm2Filter::set(int order, QList<QVariant> sosMatrix, QList<QVariant> scaleValues)
{
  setOrder(order);
  for(int i = 0; i < m_sections; i++) {
    sosEntry entry;
    for(int j = 0; j < 6; j++) {
      entry.values[j] = sosMatrix[i].toList()[j].toDouble();
    }
    m_sosMatrix[i] = entry;
    m_scaleValues[i] = scaleValues[i].toDouble();
  }
}
void DirectForm2Filter::set(int order, const double sosMatrix[][6], const double scaleValues[])
{
  setOrder(order);
  for(int i = 0; i < m_sections; i++) {
    for(int j = 0; j < 6; j++) {
      m_sosMatrix[i].values[j] = sosMatrix[i][j];
    }
    m_scaleValues[i] = scaleValues[i];
  }
}

void DirectForm2Filter::resetState()
{
  for(auto &val : m_memory) {
    val = 0;
  }
  m_memoryIndex = 0;
}

void DirectForm2Filter::setData(const QVariantMap &data)
{
  set(data["order"].toInt(), data["sosMatrix"].toList(), data["scaleValues"].toList());
}

void DirectForm2Filter::setOrder(int order)
{
  m_order = order;
  m_sections = order / 2;
  m_sosMatrix.resize(m_sections);
  m_scaleValues.resize(m_sections);
}


void DirectForm2Filter::start(const QAudioFormat &format)
{
  //for each section and channel, 3 values need to be stored in memory (v[n], v[n-1] and v[n-2])
  m_memory.resize(3 * m_sections * format.channelCount());
  for(double &x : m_memory) { x = 0; }
}


bool DirectForm2Filter::processSample(QVarLengthArray<areal, 1> &channels, quint64 /*sampleIndex*/, areal maxValue, areal minValue)
{
  for(int c = 0; c < channels.size(); c++) { //apply to each channel
    areal &val = channels[c];

    for(int i = 0; i < m_sections; i++) { //apply DF2 for each section
      const sosEntry &sos = m_sosMatrix[i];
      memory(c, i, 0) = val - sos.values[A1] * memory(c, i, -1) - sos.values[A2] * memory(c, i, -2);
      val = sos.values[B0] * memory(c, i, 0) + sos.values[B1] * memory(c, i, -1) + sos.values[B2] * memory(c, i, -2);
      val *= m_scaleValues[i];
    }

    val = qBound(minValue, val, maxValue);
  }
  incrementMemoryIndex(1);

  return true;
}

void DirectForm2Filter::incrementMemoryIndex(int value)
{
  m_memoryIndex = (m_memoryIndex + value) % 3;
}

int DirectForm2Filter::memoryIndex(int offset)
{
  int index = m_memoryIndex + offset;
  return index < 0 ? index + 3 : index % 3;
}

double &DirectForm2Filter::memory(int channel, int section, int offset)
{
  return m_memory[memoryIndex(offset) + 3 * section + 3 * m_sections * channel];
}
