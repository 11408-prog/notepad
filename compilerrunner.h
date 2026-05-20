#ifndef COMPILERRUNNER_H
#define COMPILERRUNNER_H

#include <QObject>
#include <QProcess>

class QTemporaryDir;

class CompilerRunner : public QObject
{
    Q_OBJECT

public:
    explicit CompilerRunner(QObject *parent = nullptr);
    ~CompilerRunner() override;

    bool isRunning() const;
    void compile(const QString &code, const QString &compilerPath);
    void terminate(int timeoutMs = 500);

signals:
    void outputReady(const QString &text);
    void compileFailed(const QString &diagnostics);
    void compileSucceeded(const QString &executablePath);
    void finished();

private:
    QString defaultCompiler() const;
    void resetProcess();

    QProcess *m_process = nullptr;
    QTemporaryDir *m_tempDir = nullptr;
    QString m_tempSourcePath;
    QString m_errorOutput;
};

#endif // COMPILERRUNNER_H
