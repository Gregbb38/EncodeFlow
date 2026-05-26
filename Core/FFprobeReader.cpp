#include "FFprobeReader.h"

#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QProcess>

FFprobeReader::FFprobeReader(QObject* parent) : QObject(parent) {}

MediaInfo FFprobeReader::read(const QString& filePath, QString* error) {
    MediaInfo info;
    info.sizeBytes = QFileInfo(filePath).size();

    QProcess process;
    QStringList args{
        "-v", "quiet",
        "-print_format", "json",
        "-show_format",
        "-show_streams",
        filePath
    };

    process.start("ffprobe", args);
    if (!process.waitForFinished(8000)) {
        if (error) *error = "ffprobe is not responding. Make sure ffprobe is installed and available in PATH.";
        return info;
    }

    const QByteArray output = process.readAllStandardOutput();
    const QByteArray stderrOutput = process.readAllStandardError();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(output, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) *error = "Unable to read media info. " + QString::fromUtf8(stderrOutput);
        return info;
    }

    const QJsonObject root = doc.object();
    const QJsonObject format = root.value("format").toObject();
    info.durationSeconds = format.value("duration").toString().toDouble();

    const QJsonArray streams = root.value("streams").toArray();
    for (const QJsonValue& value : streams) {
        const QJsonObject stream = value.toObject();
        if (stream.value("codec_type").toString() == "video") {
            info.width = stream.value("width").toInt();
            info.height = stream.value("height").toInt();
            break;
        }
    }

    return info;
}
