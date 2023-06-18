#include "audiofileselector.h"

#include <QJniEnvironment>
#include <QJniObject>
#include <QMetaObject>
#include <QUrl>


static constexpr int REQUEST_CODE_FILE = 17;
static constexpr int REQUEST_CODE_STREAM = 18;

void check(QJniEnvironment &env) {
  if(env->ExceptionCheck()) {
    env->ExceptionDescribe();
  }
}


QString toString(const QJniObject &obj) {
  return obj.callObjectMethod("toString", "()Ljava/lang/String;").toString();
}

struct AudioStreamPrivate {
  AudioStreamPrivate(QJniObject inputStream) : inputStream(inputStream) {
  }

  ~AudioStreamPrivate() {
  }

  QJniObject inputStream;
};

struct AudioFileSelectorPrivate/* : QAndroidActivityResultReceiver*/ {
  AudioFileSelector &selector;
public: AudioFileSelectorPrivate(AudioFileSelector &selector) : selector(selector) {}
  virtual void handleActivityResult(int receiverRequestCode, int resultCode, const QJniObject &data);

  static QJniObject qtActivity() {
    return QJniObject::callStaticObjectMethod("org/qtproject/qt/android/QtNative", "activity", "()Landroid/app/Activity;");
  }

  void startActivity(QJniObject intent, int requestCode) {
    qtActivity().callMethod<void>("startActivityForResult", "(Landroid/content/Intent;IJ)V",
                                  intent.object(),
                                  requestCode,
                                  jlong(this));
  }
};

AudioStream::~AudioStream()
{
  delete d;
}

QByteArray AudioStream::read(int numBytes) {
  QJniEnvironment env;

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

  d->startActivity(
        QJniObject("android/content/Intent", "(Ljava/lang/String;)V",
                          QJniObject::getStaticObjectField(
                            "android/content/Intent", "ACTION_OPEN_DOCUMENT", "Ljava/lang/String;"
                            ).object<jstring>())
        .callObjectMethod("addCategory", "(Ljava/lang/String;)Landroid/content/Intent;",
                          QJniObject::fromString("android.intent.category.OPENABLE").object<jstring>())
        .callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;",
                          QJniObject::fromString("audio/mpeg").object<jstring>())
        .callObjectMethod("setFlags", "(I)Landroid/content/Intent;",
                          jint(0x41))
        , REQUEST_CODE_FILE);
}

void AudioFileSelector::selectAudioStream() {
  //java code:
  //startActivityForResult(new Intent(Intent.ACTION_GET_CONTENT)
  //    .setType("audio/mpeg")), REQUEST_CODE_STREAM);

  d->startActivity(
        QJniObject("android/content/Intent", "(Ljava/lang/String;)V",
                          QJniObject::getStaticObjectField(
                            "android/content/Intent", "ACTION_GET_CONTENT", "Ljava/lang/String;"
                            ).object<jstring>())
        .callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;",
                          QJniObject::fromString("audio/mpeg").object<jstring>())
        , REQUEST_CODE_STREAM);
}

void AudioFileSelectorPrivate::handleActivityResult(int receiverRequestCode, int resultCode, const QJniObject &data) {
  const auto RESULT_OK = QJniObject::getStaticField<int>("android/app/Activity", "RESULT_OK");

  qDebug() << "handle activity result:" << receiverRequestCode << REQUEST_CODE_STREAM << resultCode << RESULT_OK;

  if(receiverRequestCode == REQUEST_CODE_STREAM && resultCode == RESULT_OK) {
    //Uri uri = data.getData();
    QJniObject jUri = data.callObjectMethod("getData", "()Landroid/net/Uri;");

    QJniObject resolver = qtActivity().callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");

    //InputStream inputStream = QtActivity.getContentResolver().openInputStream(uri);
    QJniObject jInputStream = resolver.callObjectMethod("openInputStream", "(Landroid/net/Uri;)Ljava/io/InputStream;", jUri.object<jobject>());

    QMetaObject::invokeMethod(&selector, "audioStreamSelected", Q_ARG(AudioStream*, new AudioStream(&selector, new AudioStreamPrivate(jInputStream))));

  } else if(receiverRequestCode == REQUEST_CODE_FILE && resultCode == RESULT_OK) {
    //Uri uri = data.getData();
    QJniObject jUri = data.callObjectMethod("getData", "()Landroid/net/Uri;");

    qDebug() << "file selected" << jUri.toString();
    QJniObject resolver = AudioFileSelectorPrivate::qtActivity().callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");

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

  QJniObject jFileName = QJniObject::fromString(fileName);

  //Uri uri = Uri.parse(fileName);
  QJniObject jUri = QJniObject::callStaticObjectMethod("android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;", jFileName.object<jstring>());

  //InputStream inputStream = QtActivity.getContentResolver().openInputStream(uri);
  QJniObject resolver = AudioFileSelectorPrivate::qtActivity().callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
  QJniObject jInputStream = resolver.callObjectMethod("openInputStream", "(Landroid/net/Uri;)Ljava/io/InputStream;", jUri.object<jobject>());

  return new AudioStream(this, new AudioStreamPrivate(jInputStream));
}

extern "C"
{

JNIEXPORT void JNICALL Java_at_cb_BeatsActivity_nativeActivityResult(JNIEnv*, jclass, jlong nativeRef, jint requestCode, jint resultCode, jobject data)
{
  qDebug() << "native activity result" << nativeRef;
  auto *item = reinterpret_cast<AudioFileSelectorPrivate *>(nativeRef);
  if(item) {
    item->handleActivityResult(requestCode, resultCode, QJniObject(data));
  }
}

// note: since this is loaded as a QML plugin, we need to register the native implementations manually:

static const JNINativeMethod methods[] = {
  {"nativeActivityResult", "(JIILandroid/content/Intent;)V", (void *)&Java_at_cb_BeatsActivity_nativeActivityResult},
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
  qDebug() << "JNI_OnLoad from AudioFileSelector.";

  Q_UNUSED(vm)
  Q_UNUSED(reserved)

  QJniEnvironment env;

  QJniObject activity = AudioFileSelectorPrivate::qtActivity();
  env.registerNativeMethods(activity.objectClass(), methods, 1);

  return JNI_VERSION_1_6;
}

}
