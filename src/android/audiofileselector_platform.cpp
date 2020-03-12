#include "audiofileselector.h"

#include <QtAndroid>
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
#include <QAndroidActivityResultReceiver>
#include <QMetaObject>
#include <QUrl>


static constexpr int REQUEST_CODE_FILE = 17;
static constexpr int REQUEST_CODE_STREAM = 18;

void check(QAndroidJniEnvironment &env) {
  if(env->ExceptionCheck()) {
    env->ExceptionDescribe();
  }
}


QString toString(const QAndroidJniObject &obj) {
  return obj.callObjectMethod("toString", "()Ljava/lang/String;").toString();
}

struct AudioStreamPrivate {
  AudioStreamPrivate(QAndroidJniObject inputStream) : inputStream(inputStream) {
  }

  ~AudioStreamPrivate() {
  }

  QAndroidJniObject inputStream;
};

struct AudioFileSelectorPrivate : QAndroidActivityResultReceiver {
  AudioFileSelector &selector;
public: AudioFileSelectorPrivate(AudioFileSelector &selector) : selector(selector) {}
  virtual void handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &data);
};

AudioStream::~AudioStream()
{
  delete d;
}

QByteArray AudioStream::read(int numBytes) {
  QAndroidJniEnvironment env;

  //byte[] jArr = new byte[numBytes];
  //int numBytes = inputStream.read(jArr, 0, numBytes);
  jbyteArray jArr = env->NewByteArray(numBytes);
  int numRead = d->inputStream.callMethod<int>("read", "([BII)I", jArr, 0, jint(numBytes));

  QByteArray arr;
  if(numRead > 0) {
    arr = QByteArray(numRead, Qt::Uninitialized);
    env->GetByteArrayRegion(jArr, 0, numRead, reinterpret_cast<jbyte *>(arr.data()));
    env->DeleteLocalRef(jArr);
  }

  check(env);

  return arr;
}


void AudioFileSelector::platform_init() {
  d = new AudioFileSelectorPrivate(*this);
}

void AudioFileSelector::selectAudioFile() {
  //java code:
  //startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT)
  //    .addCategory(Intent.CATEGORY_OPENABLE)
  //    .setType("audio/mpeg")
  //    .setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
  //        | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)), REQUEST_CODE_FILE);

  // ACTION_OPEN_DOCUMENT combined with FLAG_GRANT_PERSISTABLE_URI_PERMISSION
  // lets you obtain a long-term permission to the file with takePersistableUriPermission()

  QtAndroid::startActivity(
        QAndroidJniObject("android/content/Intent", "(Ljava/lang/String;)V",
                          QAndroidJniObject::getStaticObjectField(
                            "android/content/Intent", "ACTION_OPEN_DOCUMENT", "Ljava/lang/String;"
                            ).object<jstring>())
        .callObjectMethod("addCategory", "(Ljava/lang/String;)Landroid/content/Intent;",
                          QAndroidJniObject::fromString("android.intent.category.OPENABLE").object<jstring>())
        .callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;",
                          QAndroidJniObject::fromString("audio/mpeg").object<jstring>())
        .callObjectMethod("setFlags", "(I)Landroid/content/Intent;",
                          jint(0x41))
        , REQUEST_CODE_FILE, d);
}

void AudioFileSelector::selectAudioStream() {
  //java code:
  //startActivityForResult(new Intent(Intent.ACTION_GET_CONTENT)
  //    .setType("audio/mpeg")), REQUEST_CODE_STREAM);

  QtAndroid::startActivity(
        QAndroidJniObject("android/content/Intent", "(Ljava/lang/String;)V",
                          QAndroidJniObject::getStaticObjectField(
                            "android/content/Intent", "ACTION_GET_CONTENT", "Ljava/lang/String;"
                            ).object<jstring>())
        .callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;",
                          QAndroidJniObject::fromString("audio/mpeg").object<jstring>())
        , REQUEST_CODE_STREAM, d);
}

void AudioFileSelectorPrivate::handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &data) {
  const auto RESULT_OK = QAndroidJniObject::getStaticField<int>("android/app/Activity", "RESULT_OK");

  if(receiverRequestCode == REQUEST_CODE_STREAM && resultCode == RESULT_OK) {
    //Uri uri = data.getData();
    QAndroidJniObject jUri = data.callObjectMethod("getData", "()Landroid/net/Uri;");

    QAndroidJniObject resolver = QtAndroid::androidActivity().callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");

    //InputStream inputStream = QtActivity.getContentResolver().openInputStream(uri);
    QAndroidJniObject jInputStream = resolver.callObjectMethod("openInputStream", "(Landroid/net/Uri;)Ljava/io/InputStream;", jUri.object<jobject>());

    QMetaObject::invokeMethod(&selector, "audioStreamSelected", Q_ARG(AudioStream*, new AudioStream(&selector, new AudioStreamPrivate(jInputStream))));

  } else if(receiverRequestCode == REQUEST_CODE_FILE && resultCode == RESULT_OK) {
    //Uri uri = data.getData();
    QAndroidJniObject jUri = data.callObjectMethod("getData", "()Landroid/net/Uri;");

    qDebug() << "file selected" << jUri.toString();
    QAndroidJniObject resolver = QtAndroid::androidActivity().callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");

    //getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
    resolver.callMethod<void>("takePersistableUriPermission", "(Landroid/net/Uri;I)V",
                              jUri.object<jstring>(), 1);

    QMetaObject::invokeMethod(&selector, "audioFileSelected", Q_ARG(QString, jUri.toString()));
  }
}

AudioStream *AudioFileSelector::openAudioStream(QString fileName)
{
  QUrl url(fileName);
  if(url.scheme().isEmpty()) {
    // assume file URL when fileName has no URL scheme
    fileName = "file://" + fileName;
  }

  qDebug() << "open stream to URL" << fileName;

  QAndroidJniObject jFileName = QAndroidJniObject::fromString(fileName);

  //Uri uri = Uri.parse(fileName);
  QAndroidJniObject jUri = QAndroidJniObject::callStaticObjectMethod("android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;", jFileName.object<jstring>());

  //InputStream inputStream = QtActivity.getContentResolver().openInputStream(uri);
  QAndroidJniObject resolver = QtAndroid::androidActivity().callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
  QAndroidJniObject jInputStream = resolver.callObjectMethod("openInputStream", "(Landroid/net/Uri;)Ljava/io/InputStream;", jUri.object<jobject>());

  return new AudioStream(this, new AudioStreamPrivate(jInputStream));
}
