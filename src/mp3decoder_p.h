#ifndef MP3DECODER_P_H
#define MP3DECODER_P_H

#include <QObject>

#include "mp3decoder.h"

class MP3DecoderPrivate : public QObject {
  Q_OBJECT

  friend class MP3Decoder;

private:
  MP3Decoder &d;

  MP3DecoderPrivate(MP3Decoder &decoder);

  ~MP3DecoderPrivate();

  QAudioSink *out = nullptr;

  mpg123_handle *handle;

  //ensure buffer size is divisible by 3 (in case input is 24bit)
  static constexpr unsigned int bufferSize = 9 * 44100; //2channels/16bit: 2.25s (-> output buffer = 6.75s)
  unsigned char inputBuffer[bufferSize];
  unsigned int bufferPos = 0;

  //output buffer size >> input buffer size, to prevent underrun
  unsigned int outputBufferSize = bufferSize * 3;

  QIODevice *dev = nullptr;
  QAudioFormat format;
  int bytesPerSample;

  MP3Decoder::State state = MP3Decoder::Idle;
  bool finished = false;
  bool metaDataObtained = false;
  bool feedNeeded = false;

  unsigned int offset; //offset to convert unsigned PCM value to signed range
  int maxValue;
  int minValue;
  qint32 currentSample = 0; //current samples being constructed of input bytes
  quint32 totalBlocks = 0; //total blocks sent to output device
  quint64 total = 0; //total bytes read
  quint64 inputIndex = 0; //# of currently read byte
  quint64 sampleIndex = 0; //# of currently read sample
  QVarLengthArray<areal, 2> channelData; //contains samples of all channels

  //performance testing values
  qint64 idleStart = 0, idleTime = 0, idleTimeTotal = 0;
  qint64 busyStart = 0, busyTime = 0, busyTimeTotal = 0;

  qint64 startTime = 0;

  QPointer<AudioStream> inputStream;

  QTimer *idleTimer;
  QMetaObject::Connection timerConnection;

signals:
  void stopped();

public slots:
  void writeBuffer();
  void decodeNextFrame();
  void readMetaData();
  void bufferAvailable();
  void play(QString path);
  void play(AudioStream *stream);
  void stop();
protected slots:
  void finishNow();
private:
  inline void processInput(unsigned char *value);
  inline void writeSamplesToBuffer(unsigned char *);
  static QString toString(mpg123_string *text);
  static QString toString(char *text);

  inline bool running() { return state == MP3Decoder::Playing; }
  inline bool idle() { return state == MP3Decoder::Idle; }
  inline bool stopping() { return state == MP3Decoder::Stopping; }
};

#endif // MP3DECODER_P_H
