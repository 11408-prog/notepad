#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <Qsci/qsciscintilla.h>

class CodeEditor : public QsciScintilla
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    // ✅ 新增：错误标记相关
    void clearErrorMarkers();
    void addErrorMarker(int line, const QString &message);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void autoInsertPair(const QString &left, const QString &right);

    // ✅ 新增：缩进辅助
    void indentSelectedLines();
    void unindentSelectedLines();
};

#endif // CODEEDITOR_H