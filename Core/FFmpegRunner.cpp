#include "FFmpegRunner.h"

#include <QtCore/QStringList>
#include <QtCore/QRegularExpression>

FFmpegRunner::FFmpegRunner(QObject* parent) : QObject(parent) {}

bool FFmpegRunner::isRunning() const 
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void FFmpegRunner::start(const QStringList& ffmpegArgs, double durationSeconds) 
{
    if (isRunning()) 
        return;

    m_durationSeconds = durationSeconds;
    m_process = new QProcess(this);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &FFmpegRunner::readStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &FFmpegRunner::readStderr);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &FFmpegRunner::handleFinished);

    QStringList args;
    args << "-progress" << "pipe:1" << "-nostats";
    args << ffmpegArgs;

    emit logLine("Command: ffmpeg " + args.join(' '));
    m_process->start("ffmpeg", args);

    if (!m_process->waitForStarted(3000)) {
        emit finished(false, "Unable to start ffmpeg. Make sure it is installed and available in PATH.");
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void FFmpegRunner::cancel() 
{
    if (!isRunning()) return;
    m_process->kill();
}

void FFmpegRunner::readStdout() 
{
    if (!m_process) return;

    m_buffer += QString::fromUtf8(m_process->readAllStandardOutput());
    const QStringList lines = m_buffer.split('\n');
    m_buffer = lines.last();

    for (int i = 0; i < lines.size() - 1; ++i) 
    {
        const QString line = lines.at(i).trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith("out_time_ms=")) 
        {
            bool ok = false;
            const double us = line.mid(QString("out_time_ms=").size()).toDouble(&ok);
            if (ok && m_durationSeconds > 0.0) {
                const double seconds = us / 1000000.0;
                const int percent = qBound(0, static_cast<int>((seconds / m_durationSeconds) * 100.0), 100);
                emit progressChanged(percent, {});
            }
        } 
        else if (line.startsWith("speed=")) 
        {
            emit progressChanged(-1, line.mid(QString("speed=").size()).trimmed());
        } 
        else if (line.startsWith("progress=end")) 
        {
            emit progressChanged(100, {});
        }
    }
}

void FFmpegRunner::readStderr() 
{
    if (!m_process) 
        return;

    const QString text = QString::fromUtf8(m_process->readAllStandardError());
    for (const QString& line : text.split('\n')) 
    {
        if (!line.trimmed().isEmpty()) emit logLine(line.trimmed());
    }
}

void FFmpegRunner::handleFinished(int exitCode, QProcess::ExitStatus status) 
{
    const bool success = status == QProcess::NormalExit && exitCode == 0;

    if (m_process) 
    {
        m_process->deleteLater();
        m_process = nullptr;
    }

    emit finished(success, success ? "Conversion complete." : "Conversion failed or was canceled.");

}
