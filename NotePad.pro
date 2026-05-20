QT += core gui widgets sql printsupport charts
CONFIG += c++17 warn_on

TARGET = NotePad
TEMPLATE = app

MOC_DIR = $$OUT_PWD/.moc
OBJECTS_DIR = $$OUT_PWD/.obj
UI_DIR = $$OUT_PWD/.ui
RCC_DIR = $$OUT_PWD/.rcc

SOURCES += main.cpp \
           appsettings.cpp \
           compilerrunner.cpp \
           codeeditor.cpp \
           databasemanager.cpp \
           filebrowserwidget.cpp \
           datamodel.cpp \
           chartwidget.cpp \
           dataanalysiswidget.cpp \
           pythonscriptrunner.cpp \
           imagedisplaywidget.cpp \
           imageviewerwidget.cpp \
           imageenhancer.cpp \
           settingswidget.cpp \
           terminalwidget.cpp \
           mainwindow_ui.cpp \
           mainwindow_editor.cpp    \
           mainwindow.cpp

HEADERS += appsettings.h \
           compilerrunner.h \
           databasemanager.h \
           datamodel.h \
           chartwidget.h \
           dataanalysiswidget.h \
           pythonscriptrunner.h \
           imagedisplaywidget.h \
           imageviewerwidget.h \
           codeeditor.h \
           filebrowserwidget.h \
           mainwindow.h \
           imageenhancer.h \
           settingswidget.h \
           terminalwidget.h \
           note.h

FORMS += mainwindow.ui

# ========== ElaWidgetTools 配置 ==========
win32 {
    INCLUDEPATH += $$PWD/lib/ElaWidgetTools/include
    LIBS += -L$$PWD/lib/ElaWidgetTools/lib -lElaWidgetTools
}

# ========== QScintilla 配置 ==========
INCLUDEPATH += $$PWD/lib/QScintilla/include
win32 {
    CONFIG(debug, debug|release) {
        LIBS += -L$$PWD/lib/QScintilla/lib -lqscintilla2_qt6d
        QMAKE_POST_LINK += $$quote(cmd /c copy /y \"$$shell_path($$PWD/lib/QScintilla/lib/qscintilla2_qt6d.dll)\" \"$$shell_path($$OUT_PWD/debug)\" &)
    } else {
        LIBS += -L$$PWD/lib/QScintilla/lib -lqscintilla2_qt6
        QMAKE_POST_LINK += $$quote(cmd /c copy /y \"$$shell_path($$PWD/lib/QScintilla/lib/qscintilla2_qt6.dll)\" \"$$shell_path($$OUT_PWD/release)\" &)
    }
}

# ========== 复制 ElaWidgetTools.dll ==========
win32 {
    CONFIG(debug, debug|release) {
        QMAKE_POST_LINK += $$quote(cmd /c copy /y \"$$shell_path($$PWD/lib/ElaWidgetTools/bin/ElaWidgetTools.dll)\" \"$$shell_path($$OUT_PWD/debug)\")
    } else {
        QMAKE_POST_LINK += $$quote(cmd /c copy /y \"$$shell_path($$PWD/lib/ElaWidgetTools/bin/ElaWidgetTools.dll)\" \"$$shell_path($$OUT_PWD/release)\")
    }
}

# ========== 预留 QTermWidget 集成（直接链接） ==========
# 通过 vcpkg 安装 qtermwidget:x64-windows 后，取消下面两行的注释
# win32 {
#     INCLUDEPATH += C:/dev/vcpkg/installed/x64-windows/include/qtermwidget6
#     LIBS += -LC:/dev/vcpkg/installed/x64-windows/lib -lqtermwidget6
# }

win32:LIBS += -luser32 -lshell32 -ladvapi32

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
