TEMPLATE = lib
TARGET = BeatLib
QT += qml quick
CONFIG += plugin c++17

# always build debug + release versions
CONFIG += debug_and_release make_all build_all

include(src/beatlib_sources.pri)

TARGET = $$qtLibraryTarget($$TARGET)
uri = at.cb.beatlib
uri_path = $$replace(uri, '\.', "/")

COMPILER_PATH = $$[QT_INSTALL_PREFIX]
PLATFORM_LIBRARY_POSTFIX = $$section(COMPILER_PATH, "/", -1, -1)

LIBDIR = $$PWD/../lib/$$PLATFORM_LIBRARY_POSTFIX
DESTDIR = $$LIBDIR/$$uri_path

DISTFILES = qmldir

OTHER_FILES += qmldir

qmldir.files = qmldir

# Copy the qmldir file to the same folder as the plugin binary
cpqmldir.files = qmldir
cpqmldir.path = $$DESTDIR
COPIES += cpqmldir

# Generate qmltypes file after linking
QMLTYPESFILE_OUTPUT = $$DESTDIR/plugins.qmltypes
qtPrepareTool(QMLPLUGINDUMP, qmlplugindump)
mac: !exists($$QMLPLUGINDUMP): QMLPLUGINDUMP = "$${QMLPLUGINDUMP}.app/Contents/MacOS/qmlplugindump"
QMAKE_POST_LINK += $$QMLPLUGINDUMP -v -nonrelocatable $$uri "1.0" $$LIBDIR > $$QMLTYPESFILE_OUTPUT

OTHER_FILES += doc/*.qdocconf doc/src/* doc/style/* doc/images/*
