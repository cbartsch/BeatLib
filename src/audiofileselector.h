#ifndef AUDIOFILESELECTOR_H
#define AUDIOFILESELECTOR_H

#include <QObject>
#include <QFileDialog>
#include <QtQml>

struct AudioStreamPrivate;
class AudioStream : public QObject {
  Q_OBJECT

public:
  AudioStream(QObject *parent, AudioStreamPrivate *d) : QObject(parent), d(d) { }
  ~AudioStream();

  QByteArray read(int numBytes);

private:
  AudioStreamPrivate *d;
};

class AudioFileSelector : public QObject {
  Q_OBJECT
public:
  static void registerQml() {
    qmlRegisterType<AudioFileSelector>("beats", 1, 0, "AudioFileSelector");
    qmlRegisterUncreatableType<AudioStream>("beats", 1, 0, "AudioStream", "Only returned by AudioFileSelector.selectAudioStream()");
  }

  AudioFileSelector();
  Q_INVOKABLE void selectAudioFile();
  Q_INVOKABLE void selectAudioStream();
  Q_INVOKABLE AudioStream *openAudioStream(QString fileName);

signals:
  void audioFileSelected(QString fileName);
  void audioStreamSelected(AudioStream *stream);

private:
  struct AudioFileSelectorPrivate *d;
  void platform_init();
};

#endif // AUDIOFILESELECTOR_H
