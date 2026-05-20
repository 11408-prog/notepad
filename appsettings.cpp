#include "appsettings.h"

QSettings& appSettings()
{
    static QSettings settings(QSettings::IniFormat, QSettings::UserScope, "NotePad", "NotePad");
    return settings;
}
