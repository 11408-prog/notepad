# Engineering Notes

## Build Entrypoints

This project keeps the original `NotePad.pro` qmake project for Qt Creator compatibility and adds a CMake build for repeatable command-line builds.

Recommended Windows command:

```powershell
.\scripts\build.ps1 -Configuration Debug
```

The CMake build expects Qt 6.5+ and the vendored libraries under `lib/`:

- `lib/ElaWidgetTools`
- `lib/QScintilla`

## Source Map

- `main.cpp` starts the Qt application and applies the saved theme.
- `mainwindow.*` owns the primary navigation, editor tabs, notes UI, terminal, settings, and tool pages.
- `appsettings.*` centralizes `QSettings` access.
- `settingswidget.*` owns the settings page UI and persists settings.
- `terminalwidget.*` owns the embedded terminal UI and shell process.
- `compilerrunner.*` owns temporary source creation, compiler process output, and compile diagnostics.
- `databasemanager.*` owns local SQLite note and notebook storage.
- `codeeditor.*` wraps QScintilla editor behavior.
- `filebrowserwidget.*` provides filesystem navigation.
- `dataanalysiswidget.*`, `datamodel.*`, and `chartwidget.*` provide data import and charts.
- `pythonscriptrunner.*` owns Python script process execution for the data analysis page.
- `imageviewerwidget.*`, `imagedisplaywidget.*`, and `imageenhancer.*` provide image viewing, canvas interaction, and enhancement.

## Safety Notes

The image viewer behavior is intentionally left untouched by engineering changes. Treat any future file deletion or image lifecycle changes as high risk and cover them with manual verification before merging.

Generated artifacts such as `moc_*.cpp`, object files, build directories, and IDE metadata should stay out of source control.
