#pragma once

#include <QString>

struct HistoryEntry {
    QString date;
    QString input;
    QString output;
    QString preset;
    QString status;
    QString command;
};

class HistoryStore {
public:
    static void append(const HistoryEntry& entry);
    static QStringList loadLines();
    static QString filePath();
};
