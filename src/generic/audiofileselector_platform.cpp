#include "audiofileselector.h"


struct AudioFileSelectorPrivate {
};

struct AudioStreamPrivate {
  AudioStreamPrivate(QString name) : file(name) {
    file.open(QFile::OpenModeFlag::ReadOnly);
  }

  ~AudioStreamPrivate() {
    file.close();
  }

  QFile file;
};

AudioStream::~AudioStream()
{
  delete d;
}

QByteArray AudioStream::read(int numBytes) {
  return d->file.read(numBytes);
}

void AudioFileSelector::platform_init() {
  d = new AudioFileSelectorPrivate;
}

void AudioFileSelector::selectAudioFile()
{
  auto name = QFileDialog::getOpenFileName(nullptr, "Open media file",
                                           QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
                                           "Audio files (*.mp3)");
  qDebug() << "name" << name;
  if(!name.isEmpty()) {
    emit audioFileSelected(name);
  }
}


void AudioFileSelector::selectAudioStream()
{
  auto name = QFileDialog::getOpenFileName(nullptr, "Open media file",
                                           QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
                                           "Audio files (*.mp3)");
  qDebug() << "name" << name;
  if(!name.isEmpty()) {
    emit audioStreamSelected(new AudioStream(this, new AudioStreamPrivate(name)));
  }
}

AudioStream *AudioFileSelector::openAudioStream(QString fileName)
{
  return new AudioStream(this, new AudioStreamPrivate(fileName));
}
