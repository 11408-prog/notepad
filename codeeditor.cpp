#include "codeeditor.h"
#include <QKeyEvent>

static const int ERROR_MARKER_NUM = 8;  // ✅ 错误标记编号

CodeEditor::CodeEditor(QWidget *parent)
    : QsciScintilla(parent)
{
    // ✅ 初始化错误标记样式
    markerDefine(QsciScintilla::Circle, ERROR_MARKER_NUM);
    setMarkerBackgroundColor(QColor(255, 100, 100, 150), ERROR_MARKER_NUM);
}

void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    QString keyText = e->text();  // ✅ 改名，避免遮蔽基类 text() 方法
    bool handled = false;

    // ✅ 处理 Tab 缩进
    if (e->key() == Qt::Key_Tab) {
        if (hasSelectedText()) {
            indentSelectedLines();
        } else {
            insert("    ");
        }
        handled = true;
    }
    else if (e->key() == Qt::Key_Backtab) {
        unindentSelectedLines();
        handled = true;
    }
    // 成对符号映射
    else if (keyText == "(") {
        autoInsertPair("(", ")");
        handled = true;
    } else if (keyText == "[") {
        autoInsertPair("[", "]");
        handled = true;
    } else if (keyText == "{") {
        autoInsertPair("{", "}");
        handled = true;
    } else if (keyText == "\"") {
        autoInsertPair("\"", "\"");
        handled = true;
    } else if (keyText == "'") {
        autoInsertPair("'", "'");
        handled = true;
    }
    // ✅ 智能跳过右括号（光标右侧已有相同字符时不插入）
    else if (keyText == ")" || keyText == "]" || keyText == "}" || keyText == "\"" || keyText == "'") {
        int line, index;
        getCursorPosition(&line, &index);
        QString lineText = text(line);  // ✅ 现在 text() 是基类方法
        if (index < lineText.length() && lineText.mid(index, 1) == keyText) {
            setCursorPosition(line, index + 1);
            handled = true;
        }
    }

    if (handled) {
        return;
    }

    QsciScintilla::keyPressEvent(e);
}
void CodeEditor::autoInsertPair(const QString &left, const QString &right)
{
    int line, index;
    getCursorPosition(&line, &index);

    beginUndoAction();
    insert(left + right);
    setCursorPosition(line, index + left.length());
    endUndoAction();
}

// ✅ 新增：缩进选中行
void CodeEditor::indentSelectedLines()
{
    int lineFrom, indexFrom, lineTo, indexTo;
    getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
    if (lineFrom > lineTo) {
        std::swap(lineFrom, lineTo);
    }
    beginUndoAction();
    for (int line = lineFrom; line <= lineTo; ++line) {
        setCursorPosition(line, 0);
        insert("    ");
    }
    // 恢复选区（大致位置）
    setSelection(lineFrom, indexFrom + 4, lineTo, indexTo + 4);
    endUndoAction();
}

// ✅ 新增：反缩进选中行
void CodeEditor::unindentSelectedLines()
{
    int lineFrom, indexFrom, lineTo, indexTo;
    getSelection(&lineFrom, &indexFrom, &lineTo, &indexTo);
    if (lineFrom > lineTo) {
        std::swap(lineFrom, lineTo);
    }
    beginUndoAction();
    for (int line = lineFrom; line <= lineTo; ++line) {
        QString lineText = text(line);
        if (lineText.startsWith("    ")) {
            setCursorPosition(line, 0);
            setSelection(line, 0, line, 4);
            removeSelectedText();
        } else if (lineText.startsWith("\t")) {
            setCursorPosition(line, 0);
            setSelection(line, 0, line, 1);
            removeSelectedText();
        }
    }
    endUndoAction();
}

// ✅ 新增：清除所有错误标记
void CodeEditor::clearErrorMarkers()
{
    markerDeleteAll(ERROR_MARKER_NUM);
}

// ✅ 新增：添加错误标记
void CodeEditor::addErrorMarker(int line, const QString &message)
{
    Q_UNUSED(message);
    markerAdd(line, ERROR_MARKER_NUM);
}