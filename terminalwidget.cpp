#include "terminalwidget.h"

#include <QDir>
#include <QShortcut>
#include <QTextCursor>
#include <QTextOption>
#include <QTimer>
#include <QVBoxLayout>

#include <ElaLineEdit.h>
#include <ElaPlainTextEdit.h>

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(5);

    m_output = new ElaPlainTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setWordWrapMode(QTextOption::NoWrap);

    m_input = new ElaLineEdit(this);
    m_input->setPlaceholderText("输入命令，按 Enter 执行... (Ctrl+L 清屏)");

    layout->addWidget(m_output);
    layout->addWidget(m_input);

    auto *clearShortcut = new QShortcut(QKeySequence("Ctrl+L"), m_output);
    connect(clearShortcut, &QShortcut::activated, this, &TerminalWidget::clear);
    connect(m_input, &ElaLineEdit::returnPressed, this, &TerminalWidget::submitCommand);
}

TerminalWidget::~TerminalWidget()
{
    terminateProcess(1000);
}

void TerminalWidget::appendPlainText(const QString &text)
{
    if (!m_output)
        return;
    m_output->appendPlainText(text);
    scrollToEnd();
}

void TerminalWidget::clear()
{
    if (m_output)
        m_output->clear();
}

void TerminalWidget::applyTheme(ElaThemeType::ThemeMode mode)
{
    const QString bg = (mode == ElaThemeType::Dark) ? "#1e1e1e" : "#ffffff";
    const QString fg = (mode == ElaThemeType::Dark) ? "#d4d4d4" : "#000000";
    const QString border = (mode == ElaThemeType::Dark) ? "#3c3c3c" : "#c0c0c0";

    if (m_output) {
        m_output->setStyleSheet(QString("background-color: %1; color: %2; "
                                        "font-family: 'Cascadia Code', Consolas, monospace; font-size: 11pt;")
                                    .arg(bg, fg));
    }

    if (m_input) {
        m_input->setStyleSheet(QString("background-color: %1; color: %2; "
                                       "font-family: 'Cascadia Code', Consolas, monospace; font-size: 11pt; "
                                       "padding: 6px; border: 1px solid %3; border-radius: 4px;")
                                   .arg(bg, fg, border));
    }
}

void TerminalWidget::terminateProcess(int timeoutMs)
{
    if (!m_process || m_process->state() != QProcess::Running)
        return;

    m_process->terminate();
    if (!m_process->waitForFinished(timeoutMs))
        m_process->kill();
}

void TerminalWidget::submitCommand()
{
    const QString command = m_input->text();
    if (command.isEmpty())
        return;

    m_input->clear();
    appendPlainText("$ " + command);

    ensureProcessRunning();
    QTimer::singleShot(100, this, [this, command]() {
        if (m_process && m_process->state() == QProcess::Running) {
            m_process->write(command.toUtf8() + "\n");
        } else {
            appendPlainText("[错误] 终端进程未能启动");
        }
    });
}

void TerminalWidget::readProcessOutput()
{
    auto *process = qobject_cast<QProcess*>(sender());
    if (!process)
        return;

    const QByteArray data = process->readAll();
    if (!data.isEmpty())
        appendPlainText(QString::fromUtf8(data));
}

void TerminalWidget::handleProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    appendPlainText(status == QProcess::CrashExit ? "\n[终端] 异常终止" : "\n[终端] 退出");

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void TerminalWidget::ensureProcessRunning()
{
    if (!m_process) {
        m_process = new QProcess(this);
        m_process->setWorkingDirectory(QDir::homePath());
        connect(m_process, &QProcess::readyReadStandardOutput, this, &TerminalWidget::readProcessOutput);
        connect(m_process, &QProcess::readyReadStandardError, this, &TerminalWidget::readProcessOutput);
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &TerminalWidget::handleProcessFinished);
    }

    if (m_process->state() == QProcess::Running)
        return;

#ifdef Q_OS_WIN
    m_process->start("cmd.exe", QStringList() << "/k" << "chcp 65001 >nul");
#else
    m_process->start("/bin/bash");
#endif
}

void TerminalWidget::scrollToEnd()
{
    if (!m_output)
        return;

    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_output->setTextCursor(cursor);
}
