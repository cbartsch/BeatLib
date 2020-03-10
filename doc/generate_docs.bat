set PATH=%PATH%;D:\sdks\Qt\5.12.3\mingw73_32\bin

qdoc beatlib.qdocconf

qhelpgenerator.exe html/beatlib.qhp -o beatlib.qch
