#pragma once

#include <QtCore/QProcess>

class FFmpegRunner : public QObject {
    Q_OBJECT
public:
    explicit FFmpegRunner(QObject* parent);

    void start(const QStringList& ffmpegArgs, double durationSeconds);
    void cancel();
    bool isRunning() const;

signals:
    void logLine(const QString& line);
    void progressChanged(int percent, const QString& speedText);
    void finished(bool success, const QString& message);

private slots:
    void readStdout();
    void readStderr();
    void handleFinished(int exitCode, QProcess::ExitStatus status);

private:
    QProcess* m_process = nullptr;
    double m_durationSeconds = 0.0;
    QString m_buffer;
};
