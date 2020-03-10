QT += multimedia # QAudioOutput
QT += widgets # QFileDialog

android {
  QT += androidextras
}

INCLUDEPATH += $$PWD $$PWD/../include

SOURCES += $$PWD/beatlib_plugin.cpp
HEADERS += $$PWD/beatlib_plugin.h

SOURCES += \
    $$PWD/mp3decoder.cpp \
    $$PWD/audiofileselector.cpp \
    $$PWD/audioeffect.cpp \
    $$PWD/directform2filter.cpp \
    $$PWD/beatdetector.cpp \
    $$PWD/multieffect.cpp \
    $$PWD/utils.cpp \
    $$PWD/volumedetector.cpp \
    $$PWD/qmlpolygon.cpp \
    $$PWD/peakfinder.cpp \
    $$PWD/mp3metadata.cpp

android {
    SOURCES += $$PWD/android/audiofileselector_platform.cpp
} else:ios {
    OBJECTIVE_SOURCES += $$PWD/ios/audiofileselector_platform.mm
} else {
    SOURCES += $$PWD/generic/audiofileselector_platform.cpp
}

HEADERS += \
    $$PWD/mp3decoder.h \
    $$PWD/audiofileselector.h \
    $$PWD/audioeffect.h \
    $$PWD/directform2filter.h \
    $$PWD/beatdetector.h \
    $$PWD/multieffect.h \
    $$PWD/utils.h \
    $$PWD/volumedetector.h \
    $$PWD/qmlpolygon.h \
    $$PWD/peakfinder.h \
    $$PWD/mp3metadata.h

msvc {
  DEFINES += ssize_t=int
}

# could use float instead of qreal (double) as it is said to have better performance on ARM processors
# did not notice a significant improvement in speed in testing though
android {
 # DEFINES += USE_FLOAT
}

android {
  equals(ANDROID_TARGET_ARCH, armeabi-v7a)|equals(ANDROID_TARGET_ARCH, armeabi) {
    LIBS += $$PWD/../lib/libmpg123-android-arm.a
  }
  else:equals(ANDROID_TARGET_ARCH, arm64-v8a) {
    LIBS += $$PWD/../lib/libmpg123-android-arm64.a
  }
  else:equals(ANDROID_TARGET_ARCH, x86)  {
    LIBS += $$PWD/../lib/libmpg123-android-x86.a
  }
  else:error(This Android ABI is not supported for LibMpg123: $$ANDROID_TARGET_ARCH)
}
ios {
  LIBS += $$PWD/../lib/libmpg123-ios.a
}
msvc {
  LIBS += $$PWD/../lib/libmpg123-msvc.lib
}
mingw {
  LIBS += $$PWD/../lib/libmpg123-mingw.a
}
macx {
  LIBS += $$PWD/../lib/libmpg123-mac.a
}
