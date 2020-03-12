#ifndef MP3DECODER_H
#define MP3DECODER_H

#include <QObject>
#include <QtQml>
#include <QAudioOutput>

#include "mpg123.h"

#include "audioeffect.h"
#include "audiofileselector.h"
#include "mp3metadata.h"

Q_DECLARE_LOGGING_CATEGORY(logBl)

class MP3DecoderPrivate;
class MP3Decoder : public QObject
{
  Q_OBJECT

  Q_PROPERTY(AudioEffect *effect READ effect WRITE setEffect DESIGNABLE true)
  Q_PROPERTY(int sampleRate READ sampleRate NOTIFY sampleRateChanged)
  Q_PROPERTY(qint64 startTime READ startTime NOTIFY started)

  Q_PROPERTY(bool running READ running NOTIFY runningChanged)
  Q_PROPERTY(bool idle READ idle NOTIFY idleChanged)
  Q_PROPERTY(bool stopping READ stopping NOTIFY stoppingChanged)
  Q_PROPERTY(State state READ state NOTIFY stateChanged)

  Q_PROPERTY(qreal busyRate MEMBER m_busyRate NOTIFY busyRateChanged)
  Q_PROPERTY(qreal busyRateAverage MEMBER m_busyRateAverage NOTIFY busyRateChanged)
  Q_PROPERTY(MP3MetaData *metaData READ metaData NOTIFY metaDataChanged)
public:
  enum State {
    Idle, Playing, Stopping
  };

  Q_ENUM(State)

  MP3Decoder();
  ~MP3Decoder();
  Q_INVOKABLE void play(QString path);
  Q_INVOKABLE void play(AudioStream *stream);
  Q_INVOKABLE void stop();

  void writeSamples(unsigned int i, int sample, int bytesPerSample);

  AudioEffect* effect() const;
  void setEffect(AudioEffect* value);

  int sampleRate() const;

  qint64 startTime();

  State state();
  bool idle();
  bool running();
  bool stopping();

  MP3MetaData *metaData() const;

signals:
  void sampleRateChanged();
  void busyRateChanged();

  void stateChanged();
  void runningChanged();
  void stoppingChanged();
  void idleChanged();

  void started();
  void finished();
  void error(QString message);
  void metaDataChanged(MP3MetaData * metaData);

private slots:
  void onStopped();

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

