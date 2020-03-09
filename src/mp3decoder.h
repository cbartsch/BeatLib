#ifndef MP3DECODER_H
#define MP3DECODER_H

#include <QObject>
#include <QtQml>
#include <QAudioOutput>

#include "mpg123.h"

#include "audioeffect.h"
#include "audiofileselector.h"
#include "mp3metadata.h"

class MP3DecoderPrivate;
class MP3Decoder : public QObject
{
  Q_OBJECT
  Q_PROPERTY(AudioEffect* effect READ effect WRITE setEffect DESIGNABLE true)
  Q_PROPERTY(int sampleRate READ sampleRate NOTIFY sampleRateChanged)
  Q_PROPERTY(quint64 startTime READ startTime NOTIFY started)
  Q_PROPERTY(bool running READ running NOTIFY runningChanged)
  Q_PROPERTY(qreal busyRate MEMBER m_busyRate NOTIFY busyRateChanged)
  Q_PROPERTY(qreal busyRateAverage MEMBER m_busyRateAverage NOTIFY busyRateChanged)
  Q_PROPERTY(MP3MetaData *metaData READ metaData NOTIFY metaDataChanged)
public:
  static void registerQml() {
    qmlRegisterType<MP3Decoder>("beats", 1, 0, "MP3Decoder");
  }

  MP3Decoder();
  ~MP3Decoder();
  Q_INVOKABLE void play(QString path);
  Q_INVOKABLE void play(AudioStream *stream);
  Q_INVOKABLE void stop();

  void writeSamples(unsigned int i, int sample, int bytesPerSample);

  AudioEffect* effect() const;
  void setEffect(AudioEffect* value);

  int sampleRate() const;

  quint64 startTime();

  bool running();

  MP3MetaData *metaData() const;

signals:
  void sampleRateChanged();
  void runningChanged();
  void busyRateChanged();

  void started();
  void finished();
  void error(QString message);
  void metaDataChanged(MP3MetaData * metaData);

private:
  static QDebug log();

  AudioEffect *m_effect = nullptr;

  QThread m_thread;
  bool m_threadStarted = false;

  friend class MP3DecoderPrivate;
  MP3DecoderPrivate *d;

  qreal m_busyRate = 0, m_busyRateAverage = 0;
  MP3MetaData * const m_metaData;
};


#endif // MP3DECODER_H

