#include "mp3decoder.h"

#include "mpg123.h"

#include <QIODevice>
#include <QEventLoop>
#include <QAudioDecoder>
#include <QAudioOutput>
#include <QDateTime>
#include <QDebug>
#include <QVarLengthArray>

#include <mp3decoder_p.h>

Q_LOGGING_CATEGORY(logBl, "at.cb.beatlib")

MP3Decoder::MP3Decoder()  :
  d(new MP3DecoderPrivate(*this)),
  m_metaData(new MP3MetaData(this))
{
  QObject::connect(this, &MP3Decoder::stateChanged, this, &MP3Decoder::runningChanged);
  QObject::connect(this, &MP3Decoder::stateChanged, this, &MP3Decoder::idleChanged);
  QObject::connect(this, &MP3Decoder::stateChanged, this, &MP3Decoder::stoppingChanged);
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

  // cleanup properly before returning
  // otherwise a crash can happen with "QThread: Destroyed while thread is still running"
  // this can stall the app for until the buffer is written though
  m_thread.wait();
}

void MP3Decoder::stop()
{
  if(d && d->running()) {
    d->state = Stopping;
    emit stateChanged();

    if(d->inputStream) {
      delete d->inputStream;
      d->inputStream = nullptr;
    }

    QMetaObject::invokeMethod(d, "stop");
  }
}

void MP3DecoderPrivate::stop()
{
  if(d.d) {
    d.m_thread.quit();
    d.d = nullptr;

    delete this;
  }
}

void MP3Decoder::onStopped()
{
  d = new MP3DecoderPrivate(*this);

  emit stateChanged();
}

QDebug MP3Decoder::log()
{
  return qDebug(logBl) << QDateTime::currentDateTime().toString("hh:mm:ss:zzz");
}

void MP3DecoderPrivate::writeBuffer() {
  if(idleTimer) {
    idleTimer->stop();
  }
  if(busyStart) { //performance testing
    busyTime += QDateTime::currentMSecsSinceEpoch() - busyStart;
    busyStart = 0;
  }
  if(state != MP3Decoder::Playing) {
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
  } else if(out->state() == QAudio::ActiveState && out->bytesFree() < int(bufferPos)) {
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
    dev->write(reinterpret_cast<char *>(inputBuffer), bufferPos); //cast from unsigned to signed (used as bytes so signedness is irrelevant)
    if(totalBlocks % 2 == 0) { //performance testing
      busyTimeTotal += busyTime;
      idleTimeTotal += idleTime;
      d.m_busyRate = qreal(busyTime) / (busyTime + idleTime);
      d.m_busyRateAverage = qreal(busyTimeTotal) / (busyTimeTotal + idleTimeTotal);

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
  if(!running() || finished) {
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
      auto ret = mpg123_feed(handle, reinterpret_cast<unsigned char*>(data.data()), size_t(data.size()));
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
    maxValue = int(offset - 1);
    minValue = -maxValue - 1;
    channelData.resize(channels);
    out = new QAudioOutput(format);
    out->setBufferSize(int(outputBufferSize));
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

  state = MP3Decoder::Playing;
  emit d.stateChanged();
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

  state = MP3Decoder::Playing;
  emit d.stateChanged();
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
  for(unsigned int i = 0; i < bufferPos && running(); i++, inputIndex++) {
    processInput(&inputBuffer[i]);

    //this loop can take up to multiple seconds
    //be sure to execute events like timers
    auto time = QDateTime::currentMSecsSinceEpoch();
    if(time - lastUpdateTime > 100) {
      if(!running()) {
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
    //reverse order of channel samples
    areal value = channelData[format.channelCount() - 1 - j];

    //convert float value to int to write to output buffer:
    int outSample = int(value);

    if(format.sampleType() == QAudioFormat::UnSignedInt) {
      outSample += offset;
    }
    //else -> signed: do nothing, float: not supported yet

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
  qint32 &sample = currentSample;
  int byteIndex = int(inputIndex % quint64(bytesPerSample));
  if(format.byteOrder() != QAudioFormat::LittleEndian) {
    sample = *value + (sample << 8); //big endian
  } else {
    sample += *value << (byteIndex * 8); //little endian
  }
  if(byteIndex == bytesPerSample - 1) {
    //full sample received

    if(format.sampleType() == QAudioFormat::SignedInt) {
      bool sign = sample & int(offset);
      if(sign) {
        if(bytesPerSample == 3) {
          sample = sample | int(0xff000000);
        } else if(bytesPerSample == 2) {
          sample = sample | int(0xffff0000);
        } else if(bytesPerSample == 1) {
          sample = sample | int(0xffffff00);
        }
      }
    } else if(format.sampleType() == QAudioFormat::UnSignedInt) {
      sample -= offset;
    } //else -> float: not supported yet

    int channelIndex = int(inputIndex / quint64(bytesPerSample)) % format.channelCount();
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
  return d ? d->format.sampleRate() : 0;
}

qint64 MP3Decoder::startTime()
{
  return d ? d->startTime : 0;
}

MP3Decoder::State MP3Decoder::state()
{
  return d ? d->state : Stopping;
}

bool MP3Decoder::idle()
{
  return d && d->idle();
}

bool MP3Decoder::running()
{
  return d && d->running();
}

bool MP3Decoder::stopping()
{
  return d && d->stopping();
}

MP3MetaData *MP3Decoder::metaData() const
{
  return m_metaData;
}
