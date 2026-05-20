#include <QApplication>
#include <QFont>
#include <QDebug>
#include <exception>
#include <ElaApplication.h>
#include <ElaTheme.h>
#include "appsettings.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFont::insertSubstitution("Fixedsys", "Consolas");
    QFont monoFont("Consolas", 10);
    a.setFont(monoFont, "QPlainTextEdit");

    ElaApplication::getInstance()->init();

    QFont defaultFont("Microsoft YaHei", 10);
    a.setFont(defaultFont);

    QString theme = appSettings().value("theme", "Light").toString();
    eTheme->setThemeMode(theme == "Light" ? ElaThemeType::Light : ElaThemeType::Dark);

    // 全局异常捕获
    try {
        MainWindow w;
        w.show();
        return a.exec();
    } catch (const std::exception &e) {
        qCritical() << "Fatal std::exception:" << e.what();
        return 1;
    } catch (...) {
        qCritical() << "Fatal unknown exception";
        return 1;
    }
}
