#ifndef MP3METADATA_H
#define MP3METADATA_H

#include <QObject>
#include <QtQml>

class MP3MetaData : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString title MEMBER m_title WRITE setTitle NOTIFY titleChanged)
  Q_PROPERTY(QString artist MEMBER m_artist WRITE setArtist NOTIFY artistChanged)
  Q_PROPERTY(QString album MEMBER m_album WRITE setAlbum NOTIFY albumChanged)

public:
  MP3MetaData(QObject *parent = nullptr);

  static void registerQml() {
    qmlRegisterUncreatableType<MP3MetaData>("beats", 1, 0, "MP3MetaData",
                                            "MP3MetaData is only used for MP3Decoder.metaData");
  }

public slots:
  void setTitle(QString title);
  void setArtist(QString artist);
  void setAlbum(QString album);

signals:
  void titleChanged(QString title);
  void artistChanged(QString artist);
  void albumChanged(QString album);

private:
  QString m_title, m_artist, m_album;
};

#endif // MP3METADATA_H
