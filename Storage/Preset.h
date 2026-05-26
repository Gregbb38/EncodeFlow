#pragma once

#include <QtCore/QString>

struct MediaInfo {
    double durationSeconds = 0.0;
    qint64 sizeBytes = 0;
    int width = 0;
    int height = 0;
};

enum class PresetKind {
    StandardMp4,
    Youtube1080p,
    Discord25mb,
    Mp3Audio,
    FastCompress,
    TargetSize,
    CustomArgs
};

struct Preset {
    QString name;
    QString outputExtension = "mp4";
    PresetKind kind = PresetKind::StandardMp4;
    QString customArgsTemplate; // e.g. -y -i {input} -c:v libx264 -crf 23 {output}
};

struct EncodeJob {
    QString inputPath;
    QString outputPath;
    Preset preset;
    MediaInfo mediaInfo;
    int targetSizeMb = 25;
    int progress = 0;
    QString status = "Pending";
    QString commandPreview;
};
