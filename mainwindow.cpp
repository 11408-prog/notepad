/****************************************************************************
** 文件：mainwindow.cpp
** 描述：NotePad 主窗口（终极稳定版 - 独立运行，按需终端）
** 版本：0.8.6 最终稳定版
****************************************************************************/

#include "mainwindow.h"
#include "appsettings.h"
#include "compilerrunner.h"
#include "databasemanager.h"
#include "filebrowserwidget.h"
#include "codeeditor.h"
#include "dataanalysiswidget.h"
#include "imageviewerwidget.h"
#include "settingswidget.h"
#include "terminalwidget.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFontDialog>
#include <QInputDialog>
#include <QMenu>
#include <QCloseEvent>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QKeyEvent>
#include <QProcess>
#include <QDir>
#include <QListWidget>
#include <QComboBox>
#include <QTimer>
#include <QRegularExpression>

#include <ElaMenu.h>
#include <ElaMenuBar.h>
#include <ElaToolBar.h>
#include <ElaToolButton.h>
#include <ElaStatusBar.h>
#include <ElaPushButton.h>
#include <ElaLineEdit.h>
#include <ElaText.h>
#include <ElaTheme.h>
#include <ElaIcon.h>
#include <ElaTabWidget.h>

#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexercpp.h>

static QString resolveDevCppPath()
{
    const QString configuredPath = appSettings().value("tools/devCppPath").toString();
    if (!configuredPath.isEmpty() && QFileInfo::exists(configuredPath))
        return QFileInfo(configuredPath).absoluteFilePath();

    const QDir appDir(QCoreApplication::applicationDirPath());
    const QStringList candidates = {
        appDir.filePath("Dev-Cpp/devcpp.exe"),
        appDir.filePath("../Dev-Cpp/devcpp.exe"),
        appDir.filePath("../../Dev-Cpp/devcpp.exe"),
        appDir.filePath("../../../Dev-Cpp/devcpp.exe"),
        QDir::current().filePath("Dev-Cpp/devcpp.exe"),
    };

    for (const QString& candidate : candidates) {
        QFileInfo info(candidate);
        if (info.exists())
            return info.absoluteFilePath();
    }
    return QString();
}

MainWindow::MainWindow(QWidget *parent)
    : ElaWindow(parent)
      , m_currentNoteId(-1)
      , m_currentNotebookId(0)
      , m_editorFont("Cascadia Code", 11)
      , lastSidebarWidth(200)
      , m_notebooksLoadedOnce(false)
{
    qDebug() << "MainWindow constructor started";
    m_editorFont.setStyleHint(QFont::Monospace);
    QFont defaultFont("Microsoft YaHei", 10);
    QApplication::setFont(defaultFont);
    setWindowTitle("NotePad");
    resize(1280, 800);
    setUserInfoCardTitle("NotePad");
    setUserInfoCardSubTitle("欢迎使用");
    m_compilerRunner = new CompilerRunner(this);

    setupNavigation();
    setupUI();
    setupConnections();
    applyUIStyles();

    if (sortComboBox_) sortComboBox_->setCurrentIndex(0);
    updateStatusBar();
    loadEditorSettings();

    QTimer::singleShot(0, this, [this]() {
        createEditorWithContent();
        navigation(editorPageKey_);
    });

    connect(this, &ElaWindow::navigationNodeClicked, this, [this](ElaNavigationType::NavigationNodeType, QString nodeKey) {
        if (nodeKey == notebookPageKey_ && !m_notebooksLoadedOnce) {
            m_notebooksLoadedOnce = true;
            loadNotebookList();
        }
    });

    qDebug() << "MainWindow constructor finished";
}

MainWindow::~MainWindow()
{
    if (m_compilerRunner && m_compilerRunner->isRunning())
        m_compilerRunner->terminate(500);
}

// ====================== 笔记本管理 ======================
void MainWindow::loadNotebookList()
{
    if (!notebookListWidget_) return;
    notebookListWidget_->clear();
    auto notebooks = DatabaseManager::instance().getAllNotebooks();
    if (notebooks.isEmpty()) {
        DatabaseManager::instance().addNotebook("默认笔记本");
        notebooks = DatabaseManager::instance().getAllNotebooks();
    }
    for (const auto &nb : notebooks) {
        auto *item = new QListWidgetItem(nb.second);
        item->setData(Qt::UserRole, nb.first);
        notebookListWidget_->addItem(item);
    }
}

void MainWindow::onNotebookSelected(QListWidgetItem *item)
{
    if (!item) return;
    bool ok = false;
    int id = item->data(Qt::UserRole).toInt(&ok);
    m_currentNotebookId = ok ? id : 0;
    loadNoteList();
}

void MainWindow::loadNoteList()
{
    QString orderBy = sortComboBox_ ? sortComboBox_->currentData().toString() : "last_modified DESC";
    m_allNotes = DatabaseManager::instance().getNotesByNotebook(m_currentNotebookId, orderBy);
    filterNoteList(searchBox_->text());
}

void MainWindow::filterNoteList(const QString &keyword)
{
    if (!listWidget_) return;
    disconnect(listWidget_, &QListWidget::itemClicked, this, &MainWindow::onNoteSelected);
    listWidget_->clear();
    QVector<Note> filteredNotes;
    if (m_searchInContent) {
        filteredNotes = DatabaseManager::instance().searchNotes(keyword, m_currentNotebookId,
                                                                sortComboBox_ ? sortComboBox_->currentData().toString() : "last_modified DESC");
    } else {
        if (keyword.isEmpty()) filteredNotes = m_allNotes;
        else for (const Note &note : std::as_const(m_allNotes))
                if (note.title.contains(keyword, Qt::CaseInsensitive)) filteredNotes.append(note);
    }
    for (const Note &note : filteredNotes) {
        QString displayTitle = note.title;
        if (displayTitle.length() > 30) displayTitle = displayTitle.left(27) + "...";
        auto *item = new QListWidgetItem(displayTitle);
        item->setData(Qt::UserRole, note.id);
        listWidget_->addItem(item);
    }
    connect(listWidget_, &QListWidget::itemClicked, this, &MainWindow::onNoteSelected);
}

void MainWindow::onNoteSelected(QListWidgetItem *item)
{
    if (!item) return;
    int noteId = item->data(Qt::UserRole).toInt();
    for (int i = 0; i < tabWidget_->count(); ++i) {
        auto *editor = qobject_cast<CodeEditor*>(tabWidget_->widget(i));
        if (editor && editor->property("noteId").toInt() == noteId) {
            tabWidget_->setCurrentIndex(i);
            return;
        }
    }
    for (const Note &note : std::as_const(m_allNotes)) {
        if (note.id == noteId) {
            auto *editor = createEditorWithContent(note.content);
            editor->setProperty("noteId", noteId);
            tabWidget_->setTabText(tabWidget_->currentIndex(), note.title);
            editor->setModified(false);
            break;
        }
    }
}

void MainWindow::addNewNotebook()
{
    bool ok;
    QString name = QInputDialog::getText(this, "新建笔记本", "笔记本名称:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        int newId = DatabaseManager::instance().addNotebook(name);
        if (newId != -1) {
            loadNotebookList();
            for (int i = 0; i < notebookListWidget_->count(); ++i) {
                if (notebookListWidget_->item(i)->data(Qt::UserRole).toInt() == newId) {
                    notebookListWidget_->setCurrentRow(i);
                    onNotebookSelected(notebookListWidget_->item(i));
                    break;
                }
            }
        } else QMessageBox::warning(this, "错误", "笔记本名称可能重复或无效。");
    }
}

void MainWindow::renameNotebook()
{
    auto *current = notebookListWidget_->currentItem();
    if (!current) return;
    int id = current->data(Qt::UserRole).toInt();
    if (id == 0) { QMessageBox::information(this, "提示", "默认笔记本不能重命名。"); return; }
    bool ok;
    QString newName = QInputDialog::getText(this, "重命名笔记本", "新名称:", QLineEdit::Normal, current->text(), &ok);
    if (ok && !newName.isEmpty()) {
        if (DatabaseManager::instance().renameNotebook(id, newName)) current->setText(newName);
        else QMessageBox::warning(this, "错误", "重命名失败，可能名称重复。");
    }
}

void MainWindow::deleteNotebook()
{
    auto *current = notebookListWidget_->currentItem();
    if (!current) return;
    int id = current->data(Qt::UserRole).toInt();
    if (id == 0) { QMessageBox::information(this, "提示", "默认笔记本不能删除。"); return; }
    if (QMessageBox::question(this, "删除笔记本", "确定删除该笔记本？其中的笔记将移至“默认笔记本”。") == QMessageBox::Yes) {
        if (DatabaseManager::instance().deleteNotebook(id)) {
            loadNotebookList();
            for (int i = 0; i < notebookListWidget_->count(); ++i) {
                if (notebookListWidget_->item(i)->data(Qt::UserRole).toInt() == 0) {
                    notebookListWidget_->setCurrentRow(i);
                    onNotebookSelected(notebookListWidget_->item(i));
                    break;
                }
            }
        } else QMessageBox::warning(this, "错误", "删除失败。");
    }
}

void MainWindow::onNotebookCustomContextMenuRequested(const QPoint &pos)
{
    auto *item = notebookListWidget_->itemAt(pos);
    if (!item) return;
    QMenu menu;
    QAction *renameAct = menu.addAction("重命名");
    QAction *deleteAct = menu.addAction("删除");
    if (item->data(Qt::UserRole).toInt() == 0) { renameAct->setEnabled(false); deleteAct->setEnabled(false); }
    QAction *selected = menu.exec(notebookListWidget_->mapToGlobal(pos));
    if (selected == renameAct) renameNotebook();
    else if (selected == deleteAct) deleteNotebook();
}

void MainWindow::deleteCurrentNote()
{
    QListWidgetItem *current = listWidget_->currentItem();
    if (!current) return;
    int id = current->data(Qt::UserRole).toInt();
    if (QMessageBox::question(this, "删除笔记", "确定删除这条笔记吗？") == QMessageBox::Yes) {
        if (DatabaseManager::instance().deleteNote(id)) {
            for (int i = 0; i < tabWidget_->count(); ++i) {
                CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget_->widget(i));
                if (editor && editor->property("noteId").toInt() == id) {
                    editor->clear();
                    editor->setModified(false);
                    tabWidget_->setTabText(i, "未命名");
                    editor->setProperty("noteId", QVariant());
                    editor->setProperty("filePath", QVariant());
                }
            }
            loadNoteList();
        } else {
            QMessageBox::warning(this, "错误", "删除失败。");
        }
    }
}

void MainWindow::renameCurrentNote()
{
    QListWidgetItem *current = listWidget_->currentItem();
    if (!current) return;
    int id = current->data(Qt::UserRole).toInt();
    for (int i = 0; i < m_allNotes.size(); ++i) {
        if (m_allNotes[i].id == id) {
            bool ok;
            QString newTitle = QInputDialog::getText(this, "重命名", "新标题:", QLineEdit::Normal, m_allNotes[i].title, &ok);
            if (ok && !newTitle.isEmpty()) {
                m_allNotes[i].title = newTitle;
                if (DatabaseManager::instance().updateNote(m_allNotes[i])) {
                    loadNoteList();
                    for (int j = 0; j < tabWidget_->count(); ++j) {
                        auto *editor = qobject_cast<CodeEditor*>(tabWidget_->widget(j));
                        if (editor && editor->property("noteId").toInt() == id)
                            tabWidget_->setTabText(j, newTitle);
                    }
                } else QMessageBox::warning(this, "错误", "重命名失败。");
            }
            break;
        }
    }
}

// ====================== 编译功能 ======================
void MainWindow::on_compile_triggered()
{
    if (m_compilerRunner->isRunning()) {
        terminalPage_->appendPlainText("[警告] 已有编译任务正在进行...");
        navigation(terminalPageKey_);
        return;
    }

    CodeEditor *editor = currentEditor();
    if (!editor) return;

    const QString code = editor->text();
    if (code.isEmpty()) {
        terminalPage_->appendPlainText("[错误] 没有可编译的代码。");
        navigation(terminalPageKey_);
        return;
    }

    navigation(terminalPageKey_);
    const QString compilerPath = appSettings().value("compilerPath").toString();
    m_compilerRunner->compile(code, compilerPath);
}

void MainWindow::parseCompilerErrors(const QString &output)
{
    CodeEditor *editor = currentEditor();
    if (!editor) return;
    editor->clearErrorMarkers();
    QRegularExpression re(R"(^([^:]+):(\d+):(\d+):\s+(error|warning|fatal error):\s+(.*)$)", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator it = re.globalMatch(output);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        bool ok;
        int line = match.captured(2).toInt(&ok);
        if (!ok || line < 1) continue;
        QString message = match.captured(5);
        editor->addErrorMarker(line - 1, message);
    }
}

// ====================== 在 Dev-C++ 中打开 ======================
void MainWindow::on_actionOpenInDevCpp_triggered()
{
    CodeEditor *editor = currentEditor();
    if (!editor) {
        QMessageBox::information(this, "提示", "没有打开的编辑器。");
        return;
    }

    QString tempDir = QDir::tempPath();
    QString tempFilePath = tempDir + "/note_temp.cpp";
    QFile tempFile(tempFilePath);
    if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法创建临时文件。");
        return;
    }
    QTextStream out(&tempFile);
    out << editor->text();
    tempFile.close();

    QString devCppPath = resolveDevCppPath();
    if (devCppPath.isEmpty()) {
        QMessageBox::warning(this, "错误",
                             "启动 Dev-C++ 失败，请检查 Dev-Cpp/devcpp.exe 是否存在，"
                             "或在设置中配置 tools/devCppPath。");
        return;
    }
    QStringList arguments;
    arguments << tempFilePath;

    bool success = QProcess::startDetached(devCppPath, arguments);
    if (!success) {
        QMessageBox::warning(this, "错误",
                             "启动 Dev-C++ 失败，请检查路径是否正确。\n路径: " + devCppPath);
    }
}

// ====================== 搜索与排序 ======================
void MainWindow::onSearchTextChanged(const QString &text) { filterNoteList(text); }
void MainWindow::onListWidgetCustomContextMenuRequested(const QPoint &pos)
{
    if (!listWidget_->itemAt(pos)) return;
    QMenu menu;
    QAction *renameAct = menu.addAction("重命名");
    QAction *deleteAct = menu.addAction("删除");
    QAction *selected = menu.exec(listWidget_->mapToGlobal(pos));
    if (selected == renameAct) renameCurrentNote();
    else if (selected == deleteAct) deleteCurrentNote();
}
void MainWindow::onSortChanged(int) { loadNoteList(); }