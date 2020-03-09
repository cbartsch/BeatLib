#include "beatlib_plugin.h"

#include <qqml.h>

#include "qmlpolygon.h"
#include "mp3decoder.h"
#include "mp3metadata.h"
#include "audiofileselector.h"
#include "directform2filter.h"
#include "beatdetector.h"
#include "multieffect.h"
#include "volumedetector.h"
#include "peakfinder.h"
#include "utils.h"

void BeatLibPlugin::registerTypes(const char *uri)
{
  qDebug() << "BeatLib plugin: register types" << uri;

  // @uri at.cb.beatlib
  Q_ASSERT(uri == QString("at.cb.beatlib"));

  // init all beats QML types

  // File selector:
  qmlRegisterType<AudioFileSelector>(uri, 1, 0, "AudioFileSelector");
  qmlRegisterUncreatableType<AudioStream>(uri, 1, 0, "AudioStream", "Only returned by AudioFileSelector.selectAudioStream()");

  // Decoder:
  qmlRegisterType<MP3Decoder>(uri, 1, 0, "MP3Decoder");

  // Abstract effect:
  qmlRegisterUncreatableType<AudioEffect>(uri, 1, 0, "AudioEffect",
                                          "AudioEffect is abstract, only implementations can be instantiated.");

  // Concrete effects:
  qmlRegisterType<BeatDetector>(uri, 1, 0, "BeatDetector");
  qmlRegisterType<DirectForm2Filter>(uri, 1, 0, "DirectForm2Filter");
  qmlRegisterType<MultiEffect>(uri, 1, 0, "MultiEffect");
  qmlRegisterType<PeakFinder>(uri, 1, 0, "PeakFinder");
  qmlRegisterType<VolumeDetector>(uri, 1, 0, "VolumeDetector");

  // Utility types:
  qmlRegisterType<QmlPolygon>(uri, 1, 0, "QmlPolygon");
  qmlRegisterSingletonType<Utils>(uri, 1, 0, "BeatUtils",
                                  [](QQmlEngine*, QJSEngine *) -> QObject * {
    static Utils s_qmlInstance;
    return &s_qmlInstance;
  });

  qRegisterMetaType<areal>("areal");
}

void BeatLibPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
  qDebug() << "Beat lib plugin: initialize engine";

  Q_UNUSED(engine)
  Q_UNUSED(uri)

  mpg123_init();
}

