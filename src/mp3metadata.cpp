#include "mp3metadata.h"

MP3MetaData::MP3MetaData(QObject *parent) : QObject(parent)
{

}

void MP3MetaData::setTitle(QString title)
{
  if (m_title == title)
    return;

  m_title = title;
  emit titleChanged(m_title);
}

void MP3MetaData::setArtist(QString artist)
{
  if (m_artist == artist)
    return;

  m_artist = artist;
  emit artistChanged(m_artist);
}

void MP3MetaData::setAlbum(QString album)
{
  if (m_album == album)
    return;

  m_album = album;
  emit albumChanged(m_album);
}
