#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <ElaWindow.h>
#include "note.h"
#include "codeeditor.h"
#include <QColor>
#include <QFont>
#include <QList>
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexercpp.h>
#include <ElaDockWidget.h>
#include "dataanalysiswidget.h"
#include "imageviewerwidget.h"

class FileBrowserWidget;
class CompilerRunner;
class SettingsWidget;
class TerminalWidget;
class ElaMenuBar;
class ElaToolBar;
class ElaStatusBar;
class ElaLineEdit;
class ElaPushButton;
class ElaTabWidget;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QComboBox;
class ElaDockWidget;

class MainWindow : public ElaWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void on_actionOpenInDevCpp_triggered();
    void on_actionNewCodeNote_triggered();
    void onFileDoubleClicked(const QString &filePath);
    void onNotebookSelected(QListWidgetItem *item);
    void addNewNotebook();
    void renameNotebook();
    void deleteNotebook();
    void onNotebookCustomContextMenuRequested(const QPoint &pos);
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionExit_triggered();
    void on_actionFont_triggered();
    void on_actionWordWrap_triggered(bool checked);
    void on_actionAbout_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionCut_triggered();
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionSelectAll_triggered();
    void onNoteSelected(QListWidgetItem *item);
    void onSearchTextChanged(const QString &text);
    void onListWidgetCustomContextMenuRequested(const QPoint &pos);
    void deleteCurrentNote();
    void renameCurrentNote();
    void updateStatusBar();
    void onSortChanged(int index);
    void onTabCloseRequested(int index);
    void onTabChanged(int index);
    void on_compile_triggered();

private:
    QAction *showNotesAction = nullptr;
    ElaDockWidget *notesDock_ = nullptr;
    QWidget *sidebarContainer_ = nullptr;
    QString _workspaceKey;
    QString _aboutKey;
    int m_currentNoteId = -1;
    int m_currentNotebookId = 0;
    QFont m_editorFont;
    int lastSidebarWidth = 200;
    bool m_notebooksLoadedOnce = false;

    FileBrowserWidget *m_fileBrowserWidget = nullptr;
    QWidget *fileBrowserPage_ = nullptr;
    QString fileBrowserPageKey_;

    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupNavigation();
    void setupStatusBar();
    void setupConnections();
    void loadNotebookList();
    void loadNoteList();
    void filterNoteList(const QString &keyword);
    CodeEditor* currentEditor() const;
    CodeEditor* createEditorWithContent(const QString &content = QString());
    void saveTabContent(int tabIndex);
    void saveToFile(const QString &path);
    void loadFromFile(const QString &path);
    bool maybeSave();
    void loadEditorSettings();
    void applyEditorSettingsToAllEditors();
    void applyEditorSettings(CodeEditor *editor);
    void parseCompilerErrors(const QString &output);
    void saveFontSettings(const QFont &font);
    void applyFontToAllEditors(const QFont &font);
    void applyUIStyles();

    ElaMenuBar *menuBar_ = nullptr;
    ElaToolBar *toolBar_ = nullptr;
    ElaStatusBar *statusBar_ = nullptr;
    QListWidget *listWidget_ = nullptr;
    ElaTabWidget *tabWidget_ = nullptr;
    ElaLineEdit *searchBox_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QComboBox *sortComboBox_ = nullptr;
    QWidget *notebookPage_ = nullptr;
    QListWidget *notebookListWidget_ = nullptr;
    ElaPushButton *addNotebookBtn_ = nullptr;
    QString notebookPageKey_;
    CompilerRunner *m_compilerRunner = nullptr;
    TerminalWidget *terminalPage_ = nullptr;
    QString terminalPageKey_;
    SettingsWidget *settingPage_ = nullptr;
    QString settingPageKey_;
    DataAnalysisWidget *dataAnalysisPage_ = nullptr;
    QString             dataAnalysisPageKey_;
    bool m_showLineNumbers = true;
    bool m_highlightCurrentLine = true;
    QColor m_currentHighlightColor;
    QWidget *editorPage_ = nullptr;
    QString editorPageKey_;
    QVector<Note> m_allNotes;
    QString currentFilePath;
    bool m_searchInContent = true;

    ImageViewerWidget *imagePage_    = nullptr;
    QString            imagePageKey_;

};

#endif // MAINWINDOW_H
