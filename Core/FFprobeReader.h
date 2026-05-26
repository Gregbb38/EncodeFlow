#pragma once

#include "Storage/Preset.h"
#include <QtCore/QObject>

class FFprobeReader : public QObject {
    Q_OBJECT
public:
    explicit FFprobeReader(QObject* parent);
    MediaInfo read(const QString& filePath, QString* error);
};
