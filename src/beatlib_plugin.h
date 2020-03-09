#ifndef BEATLIB_PLUGIN_H
#define BEATLIB_PLUGIN_H

#include <QQmlExtensionPlugin>

class BeatLibPlugin : public QQmlExtensionPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
  void registerTypes(const char *uri) override;
  void initializeEngine(QQmlEngine *engine, const char *uri) override;
};

#endif // BEATLIB_PLUGIN_H
