#pragma once

#include "Storage/Preset.h"

class CommandBuilder 
{
public:
    static QStringList buildArgs(const Preset& preset,
                                 const QString& input,
                                 const QString& output,
                                 const MediaInfo& info,
                                 int targetSizeMb = 25);

    static QString preview(const QStringList& args);

private:
    static QString quote(const QString& value);
    static QStringList splitCommandLine(const QString& text);
};
