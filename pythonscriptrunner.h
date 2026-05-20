#ifndef PYTHONSCRIPTRUNNER_H
#define PYTHONSCRIPTRUNNER_H

#include <QObject>
#include <QProcess>

class PythonScriptRunner : public QObject
{
    Q_OBJECT

public:
    explicit PythonScriptRunner(QObject *parent = nullptr);
    ~PythonScriptRunner() override;

    bool isRunning() const;
    void run(const QString &script, const QString &dataFile = QString());
    void terminate(int timeoutMs = 500);

signals:
    void outputReady(const QString &text);
    void failedToStart();
    void finished(int exitCode, QProcess::ExitStatus status);

private:
    QString pythonProgram() const;
    void resetProcess();
    void removeTemporaryScript();
    bool writeTemporaryScript(const QString &script);

    QProcess *m_process = nullptr;
    QString m_tempScriptPath;
};

#endif // PYTHONSCRIPTRUNNER_H
