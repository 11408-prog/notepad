#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QPointer>
#include <QProcess>
#include <QWidget>

#include <ElaTheme.h>

class ElaLineEdit;
class ElaPlainTextEdit;

class TerminalWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget *parent = nullptr);
    ~TerminalWidget() override;

    void appendPlainText(const QString &text);
    void clear();
    void applyTheme(ElaThemeType::ThemeMode mode);
    void terminateProcess(int timeoutMs = 500);

private slots:
    void submitCommand();
    void readProcessOutput();
    void handleProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void ensureProcessRunning();
    void scrollToEnd();

    ElaPlainTextEdit *m_output = nullptr;
    ElaLineEdit *m_input = nullptr;
    QPointer<QProcess> m_process;
};

#endif // TERMINALWIDGET_H
