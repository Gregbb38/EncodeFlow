#include "HistoryStore.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

QString HistoryStore::filePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) 
        dir = QCoreApplication::applicationDirPath();
    QDir().mkpath(dir);
    return dir + "/history.json";
}

void HistoryStore::append(const HistoryEntry& entry) 
{
    QJsonArray arr;
    QFile readFile(filePath());
    if (readFile.open(QIODevice::ReadOnly)) 
    {
        arr = QJsonDocument::fromJson(readFile.readAll()).array();
    }

    QJsonObject o;
    o["date"] = entry.date.isEmpty() ? QDateTime::currentDateTime().toString(Qt::ISODate) : entry.date;
    o["input"] = entry.input;
    o["output"] = entry.output;
    o["preset"] = entry.preset;
    o["status"] = entry.status;
    o["command"] = entry.command;
    arr.prepend(o);

    while (arr.size() > 100) arr.removeLast();

    QFile writeFile(filePath());
    if (writeFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) 
    {
        writeFile.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    }
}

QStringList HistoryStore::loadLines() 
{
    QStringList lines;
    QFile file(filePath());
    if (!file.open(QIODevice::ReadOnly)) 
        return lines;

    const QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array();
    for (const QJsonValue& v : arr) 
    {
        const QJsonObject o = v.toObject();
        lines << QString("[%1] %2 → %3 | %4 | %5")
                    .arg(o.value("date").toString())
                    .arg(o.value("input").toString())
                    .arg(o.value("output").toString())
                    .arg(o.value("preset").toString())
                    .arg(o.value("status").toString());
    }
    return lines;
}
