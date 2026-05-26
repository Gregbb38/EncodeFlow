#include "CommandBuilder.h"

#include <QStringList>

QStringList CommandBuilder::buildArgs(const Preset& preset,
                                      const QString& input,
                                      const QString& output,
                                      const MediaInfo& info,
                                      int targetSizeMb) 
{
    switch (preset.kind) 
    {
    case PresetKind::StandardMp4:
        return {"-y", "-i", input, "-c:v", "libx264", "-preset", "medium", "-crf", "23", "-c:a", "aac", "-b:a", "160k", output};

    case PresetKind::Youtube1080p:
        return {"-y", "-i", input, "-vf", "scale=-2:1080", "-c:v", "libx264", "-preset", "medium", "-crf", "20", "-c:a", "aac", "-b:a", "192k", output};

    case PresetKind::Discord25mb: 
    {
        const int audioKbps = 96;
        const double targetBytes = 25.0 * 1024.0 * 1024.0;
        int videoKbps = 700;
        if (info.durationSeconds > 0.0) {
            const double totalKbps = (targetBytes * 8.0 / info.durationSeconds) / 1000.0;
            videoKbps = qMax(150, static_cast<int>(totalKbps - audioKbps));
        }
        return {"-y", "-i", input, "-c:v", "libx264", "-b:v", QString::number(videoKbps) + "k", "-c:a", "aac", "-b:a", QString::number(audioKbps) + "k", output};
    }

    case PresetKind::Mp3Audio:
        return {"-y", "-i", input, "-vn", "-c:a", "libmp3lame", "-b:a", "192k", output};

    case PresetKind::FastCompress:
        return {"-y", "-i", input, "-c:v", "libx264", "-preset", "veryfast", "-crf", "28", "-c:a", "aac", "-b:a", "128k", output};

    case PresetKind::TargetSize: 
    {
        const int audioKbps = 128;
        const double targetBytes = static_cast<double>(targetSizeMb) * 1024.0 * 1024.0;
        int videoKbps = 900;
        if (info.durationSeconds > 0.0) {
            const double totalKbps = (targetBytes * 8.0 / info.durationSeconds) / 1000.0;
            videoKbps = qMax(150, static_cast<int>(totalKbps - audioKbps));
        }
        return {"-y", "-i", input, "-c:v", "libx264", "-b:v", QString::number(videoKbps) + "k", "-c:a", "aac", "-b:a", QString::number(audioKbps) + "k", output};
    }

    case PresetKind::CustomArgs: 
    {
        QString t = preset.customArgsTemplate;
        t.replace("{input}", quote(input));
        t.replace("{output}", quote(output));
        return splitCommandLine(t);
    }
    }
    return {};
}

QString CommandBuilder::preview(const QStringList& args) 
{
    QStringList quoted;
    for (const QString& arg : args) quoted << quote(arg);
    return "ffmpeg " + quoted.join(' ');
}

QString CommandBuilder::quote(const QString& value) {
    if (value.contains(' ') || value.contains('\\') || value.contains('/') || value.contains(':')) 
    {
        QString v = value;
        v.replace('"', "\\\"");
        return '"' + v + '"';
    }
    return value;
}

QStringList CommandBuilder::splitCommandLine(const QString& text) 
{
    QStringList result;
    QString current;
    bool inQuotes = false;
    for (int i = 0; i < text.size(); ++i) 
    {
        const QChar ch = text.at(i);
        if (ch == '"') 
        {
            inQuotes = !inQuotes;
        } 
        else if (ch.isSpace() && !inQuotes) 
        {
            if (!current.isEmpty()) {
                result << current;
                current.clear();
            }
        } 
        else 
        {
            current.append(ch);
        }
    }
    if (!current.isEmpty()) 
        result << current;
    return result;
}
