#include "PresetStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

QVector<Preset> PresetStore::builtins() 
{
    return 
    {
        {"MP4 H.264 standard", "mp4", PresetKind::StandardMp4, {}},
        {"YouTube 1080p", "mp4", PresetKind::Youtube1080p, {}},
        {"Discord 25 MB", "mp4", PresetKind::Discord25mb, {}},
        {"MP3 audio only", "mp3", PresetKind::Mp3Audio, {}},
        {"Fast compression", "mp4", PresetKind::FastCompress, {}},
        {"Compress under X MB", "mp4", PresetKind::TargetSize, {}}
    };
}

QString PresetStore::filePath() 
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) 
        dir = QCoreApplication::applicationDirPath();

    QDir().mkpath(dir);
    return dir + "/presets.json";
}

QVector<Preset> PresetStore::load() 
{
    QVector<Preset> result = builtins();

    QFile file(filePath());
    if (!file.open(QIODevice::ReadOnly)) 
        return result;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonArray arr = doc.array();
    for (const QJsonValue& v : arr) 
    {
        const QJsonObject o = v.toObject();
        Preset p;
        p.name = o.value("name").toString();
        p.outputExtension = o.value("outputExtension").toString("mp4");
        p.kind = kindFromString(o.value("kind").toString("custom"));
        p.customArgsTemplate = o.value("args").toString();
        if (!p.name.isEmpty()) result << p;
    }
    return result;
}

bool PresetStore::saveCustomPreset(const Preset& preset, QString* error) 
{
    QJsonArray arr;
    QFile readFile(filePath());
    if (readFile.open(QIODevice::ReadOnly)) 
    {
        arr = QJsonDocument::fromJson(readFile.readAll()).array();
    }

    QJsonObject o;
    o["name"] = preset.name;
    o["outputExtension"] = preset.outputExtension;
    o["kind"] = kindToString(preset.kind);
    o["args"] = preset.customArgsTemplate;
    arr.append(o);

    QFile writeFile(filePath());
    if (!writeFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) 
    {
        if (error) *error = "Unable to write presets.json";
        return false;
    }
    writeFile.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    return true;
}

QString PresetStore::kindToString(PresetKind kind) 
{
    switch (kind) 
    {
    case PresetKind::StandardMp4: return "standard";
    case PresetKind::Youtube1080p: return "youtube";
    case PresetKind::Discord25mb: return "discord25";
    case PresetKind::Mp3Audio: return "mp3";
    case PresetKind::FastCompress: return "fast";
    case PresetKind::TargetSize: return "target-size";
    case PresetKind::CustomArgs: return "custom";
    }
    return "custom";
}

PresetKind PresetStore::kindFromString(const QString& value) 
{
    if (value == "standard") return PresetKind::StandardMp4;
    if (value == "youtube") return PresetKind::Youtube1080p;
    if (value == "discord25") return PresetKind::Discord25mb;
    if (value == "mp3") return PresetKind::Mp3Audio;
    if (value == "fast") return PresetKind::FastCompress;
    if (value == "target-size") return PresetKind::TargetSize;
    return PresetKind::CustomArgs;
}
