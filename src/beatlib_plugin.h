#ifndef BEATLIB_PLUGIN_H
#define BEATLIB_PLUGIN_H

#include <QQmlExtensionPlugin>

class BeatLibPlugin : public QQmlEngineExtensionPlugin // QQmlExtensionPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
  void initializeEngine(QQmlEngine *engine, const char *uri) override;

  [[deprecated]]
  void registerTypes(const char *uri)/* override*/; // Qt 6: unused
};

#endif // BEATLIB_PLUGIN_H
