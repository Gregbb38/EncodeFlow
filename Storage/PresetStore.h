#pragma once

#include "Preset.h"

class PresetStore {
public:
    static QVector<Preset> load();
    static bool saveCustomPreset(const Preset& preset, QString* error = nullptr);
    static QString filePath();

private:
    static QVector<Preset> builtins();
    static QString kindToString(PresetKind kind);
    static PresetKind kindFromString(const QString& value);
};
