#include "compilerrunner.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

CompilerRunner::CompilerRunner(QObject *parent)
    : QObject(parent)
{
}

CompilerRunner::~CompilerRunner()
{
    terminate(500);
    delete m_tempDir;
}

bool CompilerRunner::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void CompilerRunner::compile(const QString &code, const QString &compilerPath)
{
    if (isRunning()) {
        emit outputReady("[警告] 已有编译任务正在进行...");
        return;
    }

    delete m_tempDir;
    m_tempDir = new QTemporaryDir();
    m_tempDir->setAutoRemove(true);
    if (!m_tempDir->isValid()) {
        emit outputReady("[错误] 无法创建临时目录。");
        return;
    }

    const QString tempDirPath = m_tempDir->path();
    m_tempSourcePath = tempDirPath + "/temp.cpp";
    QFile tempFile(m_tempSourcePath);
    if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit outputReady("[错误] 无法创建临时文件。");
        return;
    }

    QTextStream(&tempFile) << code;
    tempFile.close();

    const QString executablePath = tempDirPath + "/temp.exe";
    const QString compiler = compilerPath.isEmpty() ? defaultCompiler() : compilerPath;
    m_errorOutput.clear();

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(tempDirPath);

    connect(m_process, &QProcess::started, this, [this]() {
        emit outputReady("编译器已启动。\n");
    });
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        const QByteArray data = m_process->readAllStandardOutput();
        if (!data.isEmpty())
            emit outputReady(QString::fromUtf8(data));
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        const QByteArray data = m_process->readAllStandardError();
        if (data.isEmpty())
            return;

        const QString text = QString::fromUtf8(data);
        m_errorOutput += text;
        emit outputReady(text);
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, executablePath](int exitCode, QProcess::ExitStatus exitStatus) {
                QFile::remove(m_tempSourcePath);
                if (exitStatus == QProcess::CrashExit || exitCode != 0) {
                    emit outputReady("[失败] 编译出错。");
                    emit compileFailed(m_errorOutput);
                } else {
                    emit outputReady("[成功] 编译完成。正在独立窗口中运行...");
                    emit compileSucceeded(executablePath);
                }
                resetProcess();
                emit finished();
            });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;

        emit outputReady("[错误] 无法启动编译器。");
        resetProcess();
        emit finished();
    });

    emit outputReady("编译中...\n");
    m_process->start(compiler, QStringList() << m_tempSourcePath << "-o" << executablePath);
}

void CompilerRunner::terminate(int timeoutMs)
{
    if (!isRunning())
        return;

    m_process->terminate();
    if (!m_process->waitForFinished(timeoutMs))
        m_process->kill();
}

QString CompilerRunner::defaultCompiler() const
{
#ifdef Q_OS_WIN
    return "g++.exe";
#else
    return "g++";
#endif
}

void CompilerRunner::resetProcess()
{
    if (!m_process)
        return;

    m_process->deleteLater();
    m_process = nullptr;
}
