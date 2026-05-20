#include "pythonscriptrunner.h"

#include <QDir>
#include <QFile>
#include <QTemporaryFile>
#include <QTextStream>
#include <QProcessEnvironment>

PythonScriptRunner::PythonScriptRunner(QObject *parent)
    : QObject(parent)
{
}

PythonScriptRunner::~PythonScriptRunner()
{
    terminate(500);
    removeTemporaryScript();
}

bool PythonScriptRunner::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void PythonScriptRunner::run(const QString &script, const QString &dataFile)
{
    if (isRunning())
        return;

    if (!writeTemporaryScript(script)) {
        emit outputReady("[错误] 无法创建临时脚本文件");
        emit finished(-1, QProcess::CrashExit);
        return;
    }

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

           // 设置环境变量，传递数据文件路径
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!dataFile.isEmpty()) {
        env.insert("NOTEPAD_DATA_FILE", dataFile);
    }
    m_process->setProcessEnvironment(env);

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        const QByteArray data = m_process->readAllStandardOutput();
        if (!data.isEmpty())
            emit outputReady(QString::fromUtf8(data));
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
                removeTemporaryScript();
                resetProcess();
                emit finished(exitCode, status);
            });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        removeTemporaryScript();
        resetProcess();
        emit failedToStart();
        emit finished(-1, QProcess::CrashExit);
    });

    QStringList args;
    args << m_tempScriptPath;
    if (!dataFile.isEmpty()) {
        args << dataFile;   // 作为命令行参数传递
    }
    m_process->start(pythonProgram(), args);
}

void PythonScriptRunner::terminate(int timeoutMs)
{
    if (!isRunning())
        return;

    m_process->terminate();
    if (!m_process->waitForFinished(timeoutMs))
        m_process->kill();
}

QString PythonScriptRunner::pythonProgram() const
{
#ifdef Q_OS_WIN
    return "python";
#else
    return "python3";
#endif
}

void PythonScriptRunner::resetProcess()
{
    if (!m_process)
        return;
    m_process->deleteLater();
    m_process = nullptr;
}

void PythonScriptRunner::removeTemporaryScript()
{
    if (m_tempScriptPath.isEmpty())
        return;
    QFile::remove(m_tempScriptPath);
    m_tempScriptPath.clear();
}

bool PythonScriptRunner::writeTemporaryScript(const QString &script)
{
    QTemporaryFile tmpFile(QDir::tempPath() + "/notepad_script_XXXXXX.py");
    tmpFile.setAutoRemove(false);
    if (!tmpFile.open())
        return false;

    m_tempScriptPath = tmpFile.fileName();
    QTextStream(&tmpFile) << script;
    tmpFile.close();
    return true;
}