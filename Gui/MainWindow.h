#pragma once

#include <Storage/Preset.h>

#include <QtWidgets/QMainWindow>

class QComboBox;
class QLabel;
class QSpinBox;
class QPushButton;
class QProgressBar;
class QTableWidget;
class QPlainTextEdit;

class FFmpegRunner;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void addFiles();
    void removeSelectedJob();
    void startQueue();
    void cancelCurrentJob();
    void showSelectedCommand();
    void addCustomPreset();
    void updateTargetSizeVisibility();

private:
    void setupUi();
    void addFile(const QString& path);
    QString buildOutputPath(const QString& input, const Preset& preset) const;
    void refreshQueueTable();
    void startNextJob();
    void finishCurrentJob(bool success, const QString& message);
    QString formatBytes(qint64 bytes) const;
    QString formatDuration(double seconds) const;
    Preset currentPreset() const;

    QLabel* m_dropLabel = nullptr;
    QComboBox* m_presetCombo = nullptr;
    QSpinBox* m_targetSizeSpin = nullptr;
    QPushButton* m_addFilesButton = nullptr;
    QPushButton* m_removeButton = nullptr;
    QPushButton* m_startButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QPushButton* m_commandButton = nullptr;
    QPushButton* m_customPresetButton = nullptr;
    QTableWidget* m_queueTable = nullptr;
    QProgressBar* m_progress = nullptr;
    QLabel* m_speedLabel = nullptr;
    QPlainTextEdit* m_logs = nullptr;
    QPlainTextEdit* m_history = nullptr;

    QVector<Preset> m_presets;
    QVector<EncodeJob> m_jobs;
    FFmpegRunner* m_runner = nullptr;
    int m_currentJobIndex = -1;
    bool m_queueRunning = false;
};
