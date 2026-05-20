#include "mainwindow.h"

#include "appsettings.h"
#include "codeeditor.h"
#include "compilerrunner.h"
#include "databasemanager.h"
#include "imageviewerwidget.h"
#include "settingswidget.h"
#include "terminalwidget.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QKeyEvent>
#include <QListWidget>
#include <QMessageBox>
#include <QTextStream>
#include <QTimer>
#include <ElaTabWidget.h>

#include <Qsci/qsciscintilla.h>

// ====================== 标签页辅助函数 ======================
CodeEditor* MainWindow::currentEditor() const
{
    if (!tabWidget_ || tabWidget_->count() == 0) return nullptr;
    return qobject_cast<CodeEditor*>(tabWidget_->currentWidget());
}

void MainWindow::saveTabContent(int tabIndex)
{
    CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget_->widget(tabIndex));
    if (!editor) return;
    QString content = editor->text();
    QString title = tabWidget_->tabText(tabIndex);
    QVariant filePathVar = editor->property("filePath");
    if (filePathVar.isValid() && !filePathVar.toString().isEmpty()) {
        QString path = filePathVar.toString();
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream(&file) << content;
            file.close();
            editor->setModified(false);
            currentFilePath = path;
        } else {
            QMessageBox::warning(this, "错误", "无法写入文件：" + path);
        }
        return;
    }
    QVariant noteIdVar = editor->property("noteId");
    if (noteIdVar.isValid() && noteIdVar.toInt() != -1) {
        int noteId = noteIdVar.toInt();
        for (int i = 0; i < m_allNotes.size(); ++i) {
            if (m_allNotes[i].id == noteId) {
                m_allNotes[i].content = content;
                m_allNotes[i].title = title;
                m_allNotes[i].lastModified = QDateTime::currentDateTime();
                DatabaseManager::instance().updateNote(m_allNotes[i]);
                break;
            }
        }
    } else {
        QString finalTitle = title;
        if (finalTitle.isEmpty() || finalTitle == "未命名") {
            finalTitle = content.left(50).trimmed();
            if (finalTitle.isEmpty()) finalTitle = "Untitled";
        }
        Note newNote(finalTitle, content);
        newNote.created = QDateTime::currentDateTime();
        newNote.lastModified = newNote.created;
        newNote.notebookId = m_currentNotebookId;
        int newId = DatabaseManager::instance().addNote(newNote);
        if (newId != -1) {
            editor->setProperty("noteId", newId);
            tabWidget_->setTabText(tabIndex, finalTitle);
        }
    }
    editor->setModified(false);

    QTimer::singleShot(10, this, [this]() {
        disconnect(listWidget_, &QListWidget::itemClicked, this, &MainWindow::onNoteSelected);
        loadNoteList();
        connect(listWidget_, &QListWidget::itemClicked, this, &MainWindow::onNoteSelected);
    });
}

void MainWindow::onTabChanged(int index)
{
    Q_UNUSED(index);
    updateStatusBar();
}

void MainWindow::onTabCloseRequested(int index)
{
    CodeEditor *editor = qobject_cast<CodeEditor*>(tabWidget_->widget(index));
    if (!editor) return;

    if (editor->isModified()) {
        auto ret = QMessageBox::question(this, "关闭标签", "内容已修改，是否保存？",
                                         QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            saveTabContent(index);
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }

    editor->clear();
    editor->setModified(false);
    tabWidget_->setTabText(index, "未命名");
    editor->setProperty("filePath", QVariant());
    editor->setProperty("noteId", QVariant());

    bool hasContent = false;
    for (int i = 0; i < tabWidget_->count(); ++i) {
        CodeEditor *e = qobject_cast<CodeEditor*>(tabWidget_->widget(i));
        if (e && !e->text().isEmpty()) {
            hasContent = true;
            break;
        }
    }
    if (!hasContent) {
        tabWidget_->setCurrentIndex(0);
    }
}

// ====================== 菜单动作响应 ======================
void MainWindow::on_actionNew_triggered() { createEditorWithContent(); }
void MainWindow::on_actionOpen_triggered()
{
    if (!maybeSave()) return;
    QString path = QFileDialog::getOpenFileName(this, "打开", "", "文本文件 (*.txt);;所有文件 (*.*)");
    if (!path.isEmpty()) { loadFromFile(path); listWidget_->setCurrentRow(-1); }
}
void MainWindow::on_actionSave_triggered()
{
    int idx = tabWidget_->currentIndex();
    if (idx != -1) saveTabContent(idx);
}
void MainWindow::on_actionSaveAs_triggered()
{
    CodeEditor *editor = currentEditor();
    if (!editor) return;
    QString path = QFileDialog::getSaveFileName(this, "另存为", "", "文本文件 (*.txt);;所有文件 (*.*)");
    if (!path.isEmpty()) { saveToFile(path); currentFilePath = path; editor->setModified(false); updateStatusBar(); }
}
void MainWindow::on_actionExit_triggered() { close(); }
void MainWindow::on_actionFont_triggered()
{
    CodeEditor *e = currentEditor();
    if (!e) return;
    bool ok;
    QFont font = QFontDialog::getFont(&ok, e->font(), this, "选择编辑器字体");
    if (ok) {
        applyFontToAllEditors(font);
        saveFontSettings(font);
    }
}
void MainWindow::on_actionWordWrap_triggered(bool checked)
{
    CodeEditor *e = currentEditor();
    if (e) e->setWrapMode(checked ? QsciScintilla::WrapWord : QsciScintilla::WrapNone);
}
void MainWindow::on_actionAbout_triggered() { QMessageBox::about(this, "关于", "NotePad\n版本 0.8.5\n基于 Qt 和 SQLite"); }
void MainWindow::on_actionUndo_triggered() { CodeEditor *e = currentEditor(); if (e) e->undo(); }
void MainWindow::on_actionRedo_triggered() { CodeEditor *e = currentEditor(); if (e) e->redo(); }
void MainWindow::on_actionCut_triggered() { CodeEditor *e = currentEditor(); if (e) e->cut(); }
void MainWindow::on_actionCopy_triggered() { CodeEditor *e = currentEditor(); if (e) e->copy(); }
void MainWindow::on_actionPaste_triggered() { CodeEditor *e = currentEditor(); if (e) e->paste(); }
void MainWindow::on_actionSelectAll_triggered() { CodeEditor *e = currentEditor(); if (e) e->selectAll(); }

bool MainWindow::maybeSave()
{
    CodeEditor *editor = currentEditor();
    if (!editor || !editor->isModified()) return true;
    QMessageBox::StandardButton ret = QMessageBox::warning(this, "NotePad",
                                                           "文档已被修改。\n是否保存更改？",
                                                           QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save) {
        on_actionSave_triggered();
        return true;
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

void MainWindow::saveToFile(const QString &path)
{
    CodeEditor *editor = currentEditor();
    if (!editor) return;
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&file) << editor->text();
        editor->setModified(false);
    } else QMessageBox::warning(this, "错误", "无法写入文件。");
}

void MainWindow::loadFromFile(const QString &path)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QTextStream(&file).readAll();
        auto *editor = createEditorWithContent(content);
        editor->setProperty("filePath", path);
        int idx = tabWidget_->currentIndex();
        tabWidget_->setTabText(idx, QFileInfo(path).fileName());
        currentFilePath = path;
        editor->setModified(false);
        updateStatusBar();
    } else QMessageBox::warning(this, "错误", "无法读取文件。");
}

// ====================== 状态栏与事件 ======================
void MainWindow::updateStatusBar()
{
    CodeEditor *editor = currentEditor();
    if (!editor || !statusLabel_) return;
    int line, col, chars;
    editor->getCursorPosition(&line, &col);
    chars = editor->text().length();
    QString text = QString("行: %1  列: %2  字符数: %3").arg(line + 1).arg(col + 1).arg(chars);
    if (!currentFilePath.isEmpty()) text += " | 文件: " + QFileInfo(currentFilePath).fileName();
    else {
        QVariant noteId = editor->property("noteId");
        if (noteId.isValid() && noteId.toInt() != -1) text += " | 笔记 ID: " + noteId.toString();
    }
    statusLabel_->setText(text);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_compilerRunner && m_compilerRunner->isRunning()) {
        if (QMessageBox::question(this, "编译进行中", "编译尚未完成，确定退出吗？") == QMessageBox::No) {
            event->ignore();
            return;
        }
    }
    for (int i = 0; i < tabWidget_->count(); ++i) {
        tabWidget_->setCurrentIndex(i);
        if (!maybeSave()) {
            event->ignore();
            return;
        }
    }
    if (terminalPage_) terminalPage_->terminateProcess(500);
    if (m_compilerRunner) m_compilerRunner->terminate(500);
    event->accept();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == listWidget_ && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (auto *current = listWidget_->currentItem()) { onNoteSelected(current); return true; }
        }
    }
    return ElaWindow::eventFilter(obj, event);
}

// ====================== 编辑器设置 ======================
void MainWindow::loadEditorSettings()
{
    QSettings& settings = appSettings();
    m_showLineNumbers = settings.value("editor/showLineNumbers", true).toBool();
    m_highlightCurrentLine = settings.value("editor/highlightCurrentLine", true).toBool();
    QColor defaultColor(80, 80, 80, 80);
    m_currentHighlightColor = settings.value("editor/highlightColor", defaultColor).value<QColor>();

    QFont defaultFont("Cascadia Code", 11);
    defaultFont.setStyleHint(QFont::Monospace);
    m_editorFont = settings.value("editor/font", defaultFont).value<QFont>();

    if (settingPage_) settingPage_->loadFromSettings();
    applyEditorSettingsToAllEditors();
}

void MainWindow::applyEditorSettingsToAllEditors()
{
    for (int i = 0; i < tabWidget_->count(); ++i) {
        auto *editor = qobject_cast<CodeEditor*>(tabWidget_->widget(i));
        if (editor) applyEditorSettings(editor);
    }
}

void MainWindow::applyEditorSettings(CodeEditor *editor)
{
    if (!editor) return;

    editor->setMarginType(1, QsciScintilla::NumberMargin);
    editor->setMarginWidth(1, m_showLineNumbers ? "0000" : "0");
    editor->setCaretLineVisible(m_highlightCurrentLine);
    editor->setCaretLineBackgroundColor(m_currentHighlightColor);
    editor->setFont(m_editorFont);
}

void MainWindow::on_actionNewCodeNote_triggered()
{
    QString templateContent =
        "/*\n * 笔记说明：\n * \n * \n */\n\n"
        "// 代码区：\n#include <iostream>\nusing namespace std;\n\nint main() {\n    \n    return 0;\n}\n";
    CodeEditor *editor = createEditorWithContent(templateContent);
    int index = tabWidget_->currentIndex();
    tabWidget_->setTabText(index, "代码笔记");
    editor->setCursorPosition(10, 4);
}

// ====================== 文件浏览器相关 ======================
void MainWindow::onFileDoubleClicked(const QString &filePath)
{
    static const QStringList imgExts = {
        "png","jpg","jpeg","bmp","gif","webp","tiff","tif","ico"
    };
    QString ext = QFileInfo(filePath).suffix().toLower();
    if (imgExts.contains(ext)) {
        imagePage_->openImage(filePath);
        navigation(imagePageKey_);
        return;
    }
    loadFromFile(filePath);
    navigation(editorPageKey_);
}

void MainWindow::saveFontSettings(const QFont &font)
{
    m_editorFont = font;
    appSettings().setValue("editor/font", font);
}

void MainWindow::applyFontToAllEditors(const QFont &font)
{
    for (int i = 0; i < tabWidget_->count(); ++i) {
        auto *editor = qobject_cast<CodeEditor*>(tabWidget_->widget(i));
        if (editor) editor->setFont(font);
    }
}
