#include "mp3decoder.h"

#include "mpg123.h"

#include <QIODevice>
#include <QEventLoop>
#include <QAudioDecoder>
#include <QAudioOutput>
#include <QDateTime>
#include <QDebug>
#include <QVarLengthArray>



class MP3DecoderPrivate : public QObject {
  Q_OBJECT

  friend class MP3Decoder;

private:
  MP3Decoder &d;

  MP3DecoderPrivate(MP3Decoder &decoder);

  ~MP3DecoderPrivate();

  QAudioOutput *out = nullptr;

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

  bool running = false;
  bool finished = false;
  bool metaDataObtained = false;

  unsigned int offset;
  int maxValue;
  int minValue;
  int currentSample = 0; //current samples being constructed of input bytes
  QVarLengthArray<areal, 2> channelData; //contains samples of all channels
  quint64 inputIndex = 0; //# of currently read byte
  quint64 sampleIndex = 0; //# of currently read sample
  quint64 total = 0; //total bytes read
  quint32 totalBlocks = 0; //total blocks sent to output device

  //performance testing values
  qint64 idleStart = 0, idleTime = 0, idleTimeTotal = 0;
  qint64 busyStart = 0, busyTime = 0, busyTimeTotal = 0;

  quint64 startTime = 0;

  bool feedNeeded = false;
  AudioStream *inputStream = nullptr;

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
};
#include "mp3decoder.moc" //include vtable for private class

MP3Decoder::MP3Decoder()  :
  d(new MP3DecoderPrivate(*this)),
  m_metaData(new MP3MetaData(this))
{
}

MP3DecoderPrivate::MP3DecoderPrivate(MP3Decoder &decoder) : d(decoder) {
  d.m_thread.setObjectName("MP3DecoderThread");
  moveToThread(&d.m_thread);

  QObject::connect(this, &MP3DecoderPrivate::stopped,
                   &d, &MP3Decoder::onStopped);
}

MP3DecoderPrivate::~MP3DecoderPrivate() {
  if(idleTimer) {
    disconnect(timerConnection);
    idleTimer->stop();
    idleTimer = nullptr;
  }
  if(handle) {
    mpg123_close(handle);
  }
  if(dev) {
    dev->close();
  }
  if(out) {
    out->stop();
    delete out;
  }

  emit stopped();
}

MP3Decoder::~MP3Decoder()
{
  stop();
}

void MP3Decoder::stop()
{
  if(d->running) {
    if(d->inputStream) {
      delete d->inputStream;
    }

    QMetaObject::invokeMethod(d, "stop");
  }
}

void MP3DecoderPrivate::stop()
{
  if(running) {
    running = false;

    d.d = nullptr;

    delete this;
  }
}

void MP3Decoder::onStopped()
{
  m_thread.quit();

  d = new MP3DecoderPrivate(*this);

  emit runningChanged();
}

QDebug MP3Decoder::log()
{
  return qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss:zzz");
}

void MP3DecoderPrivate::writeBuffer() {
  if(idleTimer) {
    idleTimer->stop();
  }
  if(busyStart) { //performance testing
    busyTime += QDateTime::currentMSecsSinceEpoch() - busyStart;
    busyStart = 0;
  }
  if(!running) {
    //not running anymore
    //d.log() << "writeBuffer(), but stopped";
    return;
  }
  if(bufferPos == 0) {
    //no data available in input buffer
    //d.log() << "writeBuffer(), but nothing in input buffer";
    if(!finished) {
      decodeNextFrame();
    }
  } else if(out->state() == QAudio::ActiveState && out->bytesFree() < (int)bufferPos) {
    //not enough space available in audio device output buffer
    //d.log() << "writeBuffer(), but output device buffer is full (" << out->bytesFree() << "/" << bufferPos << ")";
    //try again after some time
    idleTimer->start();
    busyStart = 0;
    if(idleStart == 0) {
      idleStart = QDateTime::currentMSecsSinceEpoch(); //performance testing
    }
  } else {
    if(!dev) {
      dev = out->start();
    }
    if(!dev->isOpen()) {
      return;
    }
    //d.log() << "writing to output device, buffer: " << out->bytesFree() << "/" << out->bufferSize();
    if(idleStart) { //performance testing
      idleTime += QDateTime::currentMSecsSinceEpoch() - idleStart;
      idleStart = 0;
    }
    if(startTime == 0) {
      startTime = QDateTime::currentMSecsSinceEpoch();
      emit d.started();
    }
    dev->write((char*)inputBuffer, bufferPos);
    if(totalBlocks % 2 == 0) { //performance testing
      busyTimeTotal += busyTime;
      idleTimeTotal += idleTime;
      d.m_busyRate = (qreal)busyTime / (busyTime + idleTime);
      d.m_busyRateAverage = (qreal)busyTimeTotal / (busyTimeTotal + idleTimeTotal);

      emit d.busyRateChanged();

      d.log() << "busy / idle time: " << busyTime << "/" << (busyTime + idleTime) << "ms (" << int(d.m_busyRate * 100) << "%)";
      busyTime = 0;
      idleTime = 0;
    }
    totalBlocks++;
    bufferPos = 0;
    decodeNextFrame();
  }
}

void MP3DecoderPrivate::decodeNextFrame()
{
  if(!running || finished) {
    return;
  }
  busyStart = QDateTime::currentMSecsSinceEpoch(); //performance testing
  size_t bytes = 0;

  bool endOfStream = false;
  if(feedNeeded) {
    //reading from stream and feeding data to MPEG123
    //as opposed to letting the library read the file itself
    auto data = inputStream->read(bufferSize);
    if(!data.isEmpty()) {
      auto ret = mpg123_feed(handle, (unsigned char*)data.data(), data.size());
      feedNeeded = false;
      if(ret != MPG123_OK) {
        d.log() << "feed return value" << ret;
      }
    } else {
      endOfStream = true;
    }
  }

  int mc = mpg123_read(handle, inputBuffer, bufferSize, &bytes);
  readMetaData();

  if(mc == MPG123_NEED_MORE) {
    feedNeeded = true;
    if(bytes == 0 && out == nullptr && !endOfStream) {
      //no data to read yet, lib needs feed -> start from top of function
      decodeNextFrame();
      return;
    }
  }
  if(mc == MPG123_NEW_FORMAT) {
    long rate = 0;
    int channels = 0, encoding = 0;
    mpg123_getformat(handle, &rate, &channels, &encoding);

    format.setSampleRate(rate);
    format.setChannelCount(channels);
    bytesPerSample = mpg123_encsize(encoding);
    format.setSampleSize(bytesPerSample * 8); //set in bits
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(encoding & MPG123_ENC_SIGNED ? QAudioFormat::SignedInt : QAudioFormat::UnSignedInt);
    bytesPerSample = format.sampleSize() / 8;
    offset = 1 << (bytesPerSample * 8 - 1);
    maxValue = offset - 1;
    minValue = -maxValue - 1;
    channelData.resize(channels);
    out = new QAudioOutput(format);
    out->setBufferSize(outputBufferSize);
    d.log() << "received format: Fs = " << rate << ", channels = " << channels << ", bytes per sample = " << bytesPerSample;
    d.log() << "expected/actual QAudioOutput.bufferSize():" << outputBufferSize << out->bufferSize();

    idleTimer->setInterval(100); //check every 100ms if output needs to be written
    timerConnection = connect(idleTimer, &QTimer::timeout, this, &MP3DecoderPrivate::writeBuffer);

    emit d.sampleRateChanged();

    if(d.m_effect) {
      d.m_effect->start(format);
    }

    decodeNextFrame();
  }
  if(bytes) {
    total += bytes;
    bufferPos = bytes;

    bufferAvailable();

    writeBuffer();
  }
  if(mc == MPG123_DONE || (mc == MPG123_NEED_MORE && endOfStream)) {
    d.log() << "done, bytes read: " << total;
    finished = true;

    if(out) {
      //finish after output buffer is empty
      QTimer::singleShot(1000 * out->bufferSize() / bytesPerSample / format.channelCount() / format.sampleRate(),
                         Qt::PreciseTimer, this, SLOT(finishNow()));
    } else {
      finishNow();
    }
  } else if(mc == MPG123_ERR) {
    QString errorStr = mpg123_strerror(handle);
    d.log() << "MP3Decoder error: " << errorStr;
    emit d.error(errorStr);
    d.stop();
  } else if(mc != MPG123_OK && mc != MPG123_NEW_FORMAT && mc != MPG123_NEED_MORE) {
    //TODO wat do?
    d.log() << "nothing received, mc = " << mc;
    //stop();
  }
}

void MP3DecoderPrivate::readMetaData()
{
  int metaFlags = mpg123_meta_check(handle);
  if(metaFlags == 0) {
    return;
  }
  if(metaFlags & MPG123_ID3) {
    bool newMeta = metaFlags & MPG123_NEW_ID3;
    if(!newMeta && metaDataObtained) {
      return;
    }

    mpg123_id3v1 *id3v1;
    mpg123_id3v2 *id3v2;
    int ret = mpg123_id3(handle, &id3v1, &id3v2);

    if(ret != MPG123_OK) {
      d.log() << "Could not get ID3 meta data:" << ret;
      return;
    }

    metaDataObtained = true;

    if(id3v2) {
      d.log() << "Obtained ID3v2 meta data.";
      d.m_metaData->setTitle(toString(id3v2->title));
      d.m_metaData->setAlbum(toString(id3v2->album));
      d.m_metaData->setArtist(toString(id3v2->artist));
    }
    else if(id3v1) {
      d.log() << "Obtained ID3v1 meta data.";
      d.m_metaData->setTitle(toString(id3v1->title));
      d.m_metaData->setArtist(toString(id3v1->artist));
      d.m_metaData->setAlbum(toString(id3v1->album));
    }

    mpg123_meta_free(handle);

    emit d.metaDataChanged(d.m_metaData);
  }
  else if(metaFlags & MPG123_ICY) {
    bool newMeta = metaFlags & MPG123_NEW_ICY;
    if(!newMeta && metaDataObtained) {
      return;
    }

    char *icy;
    int ret = mpg123_icy(handle, &icy);

    if(ret != MPG123_OK) {
      d.log() << "Could not get ID3 meta data:" << ret;
      return;
    }

    metaDataObtained = true;

    d.log() << "Obtained ICY meta data:" << icy;

    // TODO use icy data

    mpg123_meta_free(handle);

    emit d.metaDataChanged(d.m_metaData);
  }
}

void MP3Decoder::play(QString path)
{
  //invoke in background thread
  m_thread.start();
  m_thread.setPriority(QThread::Priority::TimeCriticalPriority);
  QMetaObject::invokeMethod(d, "play", Q_ARG(QString, path));
}

void MP3Decoder::play(AudioStream *stream)
{
  //invoke in background thread
  m_thread.start();
  m_thread.setPriority(QThread::Priority::TimeCriticalPriority);
  QMetaObject::invokeMethod(d, "play", Q_ARG(AudioStream*, stream));
}

void MP3DecoderPrivate::play(QString path)
{
  finished = false;
  d.stop();

  idleTimer = new QTimer(this);

  handle = mpg123_new(nullptr, nullptr);
  if(mpg123_open(handle, path.toLocal8Bit().data()) != MPG123_OK) {
    QString errorStr = mpg123_strerror(handle);
    qWarning() << "Cannot open " << path << ": " << errorStr;
    emit d.error(errorStr);
    return;
  }

  running = true;
  emit d.runningChanged();
  decodeNextFrame();
}

void MP3DecoderPrivate::play(AudioStream *stream)
{
  finished = false;
  d.stop();

  idleTimer = new QTimer(this);

  handle = mpg123_new(nullptr, nullptr);
  if(mpg123_open_feed(handle) != MPG123_OK) {
    QString errorStr = mpg123_strerror(handle);
    qWarning() << "Cannot open audio feed" << errorStr;
    emit d.error(errorStr);
    return;
  }

  inputStream = stream;
  feedNeeded = true;

  running = true;
  emit d.runningChanged();
  decodeNextFrame();
}
void MP3DecoderPrivate::finishNow()
{
  emit d.finished();
  d.stop();
}

void MP3DecoderPrivate::bufferAvailable()
{
  auto lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
  auto updateStart = QDateTime::currentMSecsSinceEpoch();
  for(unsigned int i = 0; i < bufferPos && running; i++, inputIndex++) {
    processInput(&inputBuffer[i]);

    //this loop can take up to multiple seconds
    //be sure to execute events like timers
    auto time = QDateTime::currentMSecsSinceEpoch();
    if(time - lastUpdateTime > 100) {
      if(!running) {
        break;
      }

      QCoreApplication::processEvents();
      lastUpdateTime = time;
    }
  }
  d.log() << "applying effects took" << (QDateTime::currentMSecsSinceEpoch() - updateStart) << "ms";
}

void MP3DecoderPrivate::writeSamplesToBuffer(unsigned char *buffer)
{
  for(int j = 0; j < format.channelCount(); j++) {
    int outSample = channelData[format.channelCount() - 1 - j]; //reverse order of channel samples

    if(format.sampleType() == QAudioFormat::UnSignedInt) {
      outSample += offset;
    } //else -> signed: do nothing, float: not supported yet

    int index = - j * bytesPerSample;
    for(int k = 0; k < bytesPerSample; k++) {
      if(buffer + index - k >= inputBuffer) {
        if(format.byteOrder() != QAudioFormat::LittleEndian) {
          buffer[index - k] = (outSample >> (8 * k)) & 0xff; //big endian
        } else {
          buffer[index - k] = (outSample >> (8 * (bytesPerSample - k - 1))) & 0xff; //little endian
        }
      }
    }
  }
}

QString MP3DecoderPrivate::toString(mpg123_string *text)
{
  return text && text->p ? QString(text->p) : "";
}

QString MP3DecoderPrivate::toString(char *text)
{
  return text ? QString(text) : "";
}

void MP3DecoderPrivate::processInput(unsigned char *value)
{
  int &sample = currentSample;
  int byteIndex = inputIndex % bytesPerSample;
  if(format.byteOrder() != QAudioFormat::LittleEndian) {
    sample = *value + (sample << 8); //big endian
  } else {
    sample += *value << (byteIndex * 8); //little endian
  }
  if(byteIndex == bytesPerSample - 1) {
    //full sample received

    if(format.sampleType() == QAudioFormat::SignedInt) {
      bool sign = sample & offset;
      if(sign) {
        if(bytesPerSample == 3) {
          sample = sample | 0xff000000;
        } else if(bytesPerSample == 2) {
          sample = sample | 0xffff0000;
        } else if(bytesPerSample == 1) {
          sample = sample | 0xffffff00;
        }
      }
    } else if(format.sampleType() == QAudioFormat::UnSignedInt) {
      sample -= offset;
    } //else -> float: not supported yet

    int channelIndex = (inputIndex / bytesPerSample) % format.channelCount();
    channelData[channelIndex] = sample;

    if(channelIndex == format.channelCount() - 1) {
      //all channels received

      //true if any effect changed any value
      bool changed = d.m_effect && d.m_effect->enabled() &&
          d.m_effect->processSample(channelData, sampleIndex, maxValue, minValue);

      sampleIndex++;

      if(changed) {
        //write sample back in buffer to send to output device
        writeSamplesToBuffer(value);
      }
    }
    sample = 0;
  }
}

AudioEffect* MP3Decoder::effect() const
{
  return m_effect;
}

void MP3Decoder::setEffect(AudioEffect* value)
{
  m_effect = value;
}

int MP3Decoder::sampleRate() const
{
  return d->format.sampleRate();
}

quint64 MP3Decoder::startTime()
{
  return d->startTime;
}

bool MP3Decoder::running()
{
  return d && d->running;
}

MP3MetaData *MP3Decoder::metaData() const
{
  return m_metaData;
}
