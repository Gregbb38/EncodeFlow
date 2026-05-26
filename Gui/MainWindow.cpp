#include "MainWindow.h"

#include <Core/FFmpegRunner.h>
#include <Core/FFprobeReader.h>
#include <Storage/PresetStore.h>
#include <Storage/HistoryStore.h>
#include <Core/CommandBuilder.h>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QInputDialog>

#include <QtCore/QMimeData>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setAcceptDrops(true);
    m_presets = PresetStore::load();
    m_runner = new FFmpegRunner(this);
    setupUi();

    connect(m_runner, &FFmpegRunner::logLine, this, [this](const QString& line) {
        m_logs->appendPlainText(line);
        });

    connect(m_runner, &FFmpegRunner::progressChanged, this, [this](int percent, const QString& speed) {
        if (m_currentJobIndex >= 0 && m_currentJobIndex < m_jobs.size()) {
            if (percent >= 0) {
                m_jobs[m_currentJobIndex].progress = percent;
                m_progress->setValue(percent);
                refreshQueueTable();
            }
            if (!speed.isEmpty()) m_speedLabel->setText("Speed: " + speed);
        }
        });

    connect(m_runner, &FFmpegRunner::finished, this, &MainWindow::finishCurrentJob);
}

void MainWindow::setupUi() {
    setWindowTitle("EncodeFlow V2");
    resize(980, 720);

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);

    m_dropLabel = new QLabel("Drop multiple videos here\nor click Add files");
    m_dropLabel->setAlignment(Qt::AlignCenter);
    m_dropLabel->setMinimumHeight(90);
    m_dropLabel->setStyleSheet("QLabel { border: 2px dashed #777; border-radius: 8px; padding: 16px; font-size: 15px; }");
    root->addWidget(m_dropLabel);

    auto* form = new QFormLayout;
    m_presetCombo = new QComboBox;
    for (const Preset& p : m_presets) m_presetCombo->addItem(p.name);
    form->addRow("Preset :", m_presetCombo);

    m_targetSizeSpin = new QSpinBox;
    m_targetSizeSpin->setRange(1, 5000);
    m_targetSizeSpin->setValue(25);
    m_targetSizeSpin->setSuffix(" MB");
    form->addRow("Target size:", m_targetSizeSpin);
    root->addLayout(form);

    auto* buttons = new QHBoxLayout;
    m_addFilesButton = new QPushButton("Add files");
    m_removeButton = new QPushButton("Remove selected");
    m_startButton = new QPushButton("Start");
    m_cancelButton = new QPushButton("Cancel active job");
    m_commandButton = new QPushButton("View command");
    m_customPresetButton = new QPushButton("Add custom preset");
    m_cancelButton->setEnabled(false);

    buttons->addWidget(m_addFilesButton);
    buttons->addWidget(m_removeButton);
    buttons->addWidget(m_startButton);
    buttons->addWidget(m_cancelButton);
    buttons->addWidget(m_commandButton);
    buttons->addWidget(m_customPresetButton);
    root->addLayout(buttons);

    m_queueTable = new QTableWidget(0, 7);
    m_queueTable->setHorizontalHeaderLabels({ "File", "Preset", "Duration", "Size", "Output", "Progress", "Status" });
    m_queueTable->horizontalHeader()->setStretchLastSection(true);
    m_queueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_queueTable->setSelectionMode(QAbstractItemView::SingleSelection);
    root->addWidget(m_queueTable, 2);

    m_progress = new QProgressBar;
    m_speedLabel = new QLabel("Speed: -");
    root->addWidget(m_progress);
    root->addWidget(m_speedLabel);

    auto* tabs = new QTabWidget;
    m_logs = new QPlainTextEdit;
    m_logs->setReadOnly(true);
    m_history = new QPlainTextEdit;
    m_history->setReadOnly(true);
    m_history->setPlainText(HistoryStore::loadLines().join('\n'));
    tabs->addTab(m_logs, "Logs");
    tabs->addTab(m_history, "History");
    root->addWidget(tabs, 1);

    setCentralWidget(central);

    connect(m_addFilesButton, &QPushButton::clicked, this, &MainWindow::addFiles);
    connect(m_removeButton, &QPushButton::clicked, this, &MainWindow::removeSelectedJob);
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startQueue);
    connect(m_cancelButton, &QPushButton::clicked, this, &MainWindow::cancelCurrentJob);
    connect(m_commandButton, &QPushButton::clicked, this, &MainWindow::showSelectedCommand);
    connect(m_customPresetButton, &QPushButton::clicked, this, &MainWindow::addCustomPreset);
    connect(m_presetCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) { updateTargetSizeVisibility(); });
    updateTargetSizeVisibility();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event) {
    for (const QUrl& url : event->mimeData()->urls()) {
        const QString path = url.toLocalFile();
        if (!path.isEmpty()) addFile(path);
    }
}

void MainWindow::addFiles() {
    const QStringList paths = QFileDialog::getOpenFileNames(this, "Add videos", {}, "Media (*.mp4 *.mov *.mkv *.avi *.webm *.m4v *.mp3 *.wav);;All files (*.*)");
    for (const QString& path : paths) addFile(path);
}

void MainWindow::addFile(const QString& path) {
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile()) return;

    QString error;
    FFprobeReader reader(nullptr);
    const MediaInfo info = reader.read(path, &error);
    if (!error.isEmpty()) m_logs->appendPlainText(error);

    EncodeJob job;
    job.inputPath = path;
    job.preset = currentPreset();
    job.mediaInfo = info;
    job.targetSizeMb = m_targetSizeSpin->value();
    job.outputPath = buildOutputPath(path, job.preset);
    const QStringList args = CommandBuilder::buildArgs(job.preset, job.inputPath, job.outputPath, job.mediaInfo, job.targetSizeMb);
    job.commandPreview = CommandBuilder::preview(args);
    m_jobs << job;
    refreshQueueTable();
}

void MainWindow::removeSelectedJob() {
    const int row = m_queueTable->currentRow();
    if (row < 0 || row >= m_jobs.size()) return;
    if (row == m_currentJobIndex && m_runner->isRunning()) {
        QMessageBox::warning(this, "Active job", "Cancel the active job first.");
        return;
    }
    m_jobs.removeAt(row);
    refreshQueueTable();
}

void MainWindow::startQueue() {
    if (m_jobs.isEmpty()) return;
    m_queueRunning = true;
    m_startButton->setEnabled(false);
    m_cancelButton->setEnabled(true);
    startNextJob();
}

void MainWindow::startNextJob() {
    for (int i = 0; i < m_jobs.size(); ++i) 
    {
        if (m_jobs[i].status == "Pending") 
        {
            m_currentJobIndex = i;
            m_jobs[i].status = "Running";
            m_jobs[i].progress = 0;
            refreshQueueTable();

            const QStringList args = CommandBuilder::buildArgs(m_jobs[i].preset, m_jobs[i].inputPath, m_jobs[i].outputPath, m_jobs[i].mediaInfo, m_jobs[i].targetSizeMb);
            m_jobs[i].commandPreview = CommandBuilder::preview(args);
            m_logs->appendPlainText("\n=== New job ===");
            m_logs->appendPlainText(m_jobs[i].commandPreview);
            m_runner->start(args, m_jobs[i].mediaInfo.durationSeconds);
            return;
        }
    }

    m_queueRunning = false;
    m_currentJobIndex = -1;
    m_startButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
    m_progress->setValue(0);
    m_speedLabel->setText("Speed: -");
    QMessageBox::information(this, "Queue complete", "All pending jobs are complete.");
}

void MainWindow::finishCurrentJob(bool success, const QString& message) 
{
    if (m_currentJobIndex >= 0 && m_currentJobIndex < m_jobs.size()) 
    {
        auto& job = m_jobs[m_currentJobIndex];
        job.status = success ? "Complete" : "Failed/canceled";
        job.progress = success ? 100 : job.progress;
        HistoryStore::append({ QDateTime::currentDateTime().toString(Qt::ISODate), job.inputPath, job.outputPath, job.preset.name, job.status, job.commandPreview });
        m_history->setPlainText(HistoryStore::loadLines().join('\n'));
    }
    m_logs->appendPlainText(message);
    refreshQueueTable();

    if (m_queueRunning) 
        startNextJob();
}

void MainWindow::cancelCurrentJob() {
    m_queueRunning = false;
    m_runner->cancel();
    m_startButton->setEnabled(true);
    m_cancelButton->setEnabled(false);
}

void MainWindow::showSelectedCommand() {
    int row = m_queueTable->currentRow();
    if (row < 0 && !m_jobs.isEmpty()) row = 0;
    if (row < 0 || row >= m_jobs.size()) return;
    QMessageBox::information(this, "FFmpeg command", m_jobs[row].commandPreview);
}

void MainWindow::addCustomPreset() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Custom preset", "Preset name:", QLineEdit::Normal, "Custom preset", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    const QString args = QInputDialog::getMultiLineText(
        this,
        "Arguments FFmpeg",
        "Use {input} and {output}. Example:\n-y -i {input} -c:v libx264 -crf 21 -c:a aac {output}",
        "-y -i {input} -c:v libx264 -preset medium -crf 23 -c:a aac {output}",
        &ok);
    if (!ok || args.trimmed().isEmpty()) return;

    const QString ext = QInputDialog::getText(this, "Extension", "Output extension:", QLineEdit::Normal, "mp4", &ok);
    if (!ok || ext.trimmed().isEmpty()) return;

    Preset preset{ name.trimmed(), ext.trimmed().remove('.'), PresetKind::CustomArgs, args.trimmed() };
    QString error;
    if (!PresetStore::saveCustomPreset(preset, &error)) {
        QMessageBox::warning(this, "Error", error);
        return;
    }

    m_presets = PresetStore::load();
    m_presetCombo->clear();
    for (const Preset& p : m_presets) m_presetCombo->addItem(p.name);
    m_presetCombo->setCurrentIndex(m_presetCombo->count() - 1);
}

void MainWindow::updateTargetSizeVisibility() {
    const Preset p = currentPreset();
    m_targetSizeSpin->setEnabled(p.kind == PresetKind::TargetSize);
}

Preset MainWindow::currentPreset() const {
    const int index = m_presetCombo ? m_presetCombo->currentIndex() : 0;
    if (index >= 0 && index < m_presets.size()) return m_presets[index];
    return { "MP4 H.264 standard", "mp4", PresetKind::StandardMp4, {} };
}

QString MainWindow::buildOutputPath(const QString& input, const Preset& preset) const {
    QFileInfo fi(input);
    return fi.absolutePath() + "/" + fi.completeBaseName() + "_encodeflow." + preset.outputExtension;
}

void MainWindow::refreshQueueTable() {
    m_queueTable->setRowCount(m_jobs.size());
    for (int i = 0; i < m_jobs.size(); ++i) {
        const EncodeJob& j = m_jobs[i];
        QFileInfo fi(j.inputPath);
        m_queueTable->setItem(i, 0, new QTableWidgetItem(fi.fileName()));
        m_queueTable->setItem(i, 1, new QTableWidgetItem(j.preset.name));
        m_queueTable->setItem(i, 2, new QTableWidgetItem(formatDuration(j.mediaInfo.durationSeconds)));
        m_queueTable->setItem(i, 3, new QTableWidgetItem(formatBytes(j.mediaInfo.sizeBytes)));
        m_queueTable->setItem(i, 4, new QTableWidgetItem(QFileInfo(j.outputPath).fileName()));
        m_queueTable->setItem(i, 5, new QTableWidgetItem(QString::number(j.progress) + "%"));
        m_queueTable->setItem(i, 6, new QTableWidgetItem(j.status));
    }
    m_queueTable->resizeColumnsToContents();
}

QString MainWindow::formatBytes(qint64 bytes) const {
    double value = static_cast<double>(bytes);
    QStringList units{ "B", "KB", "MB", "GB" };
    int unit = 0;
    while (value >= 1024.0 && unit < units.size() - 1) {
        value /= 1024.0;
        ++unit;
    }
    return QString::number(value, 'f', unit == 0 ? 0 : 1) + " " + units[unit];
}

QString MainWindow::formatDuration(double seconds) const {
    if (seconds <= 0.0) return "-";
    const int s = static_cast<int>(seconds);
    return QString("%1:%2:%3")
        .arg(s / 3600, 2, 10, QChar('0'))
        .arg((s % 3600) / 60, 2, 10, QChar('0'))
        .arg(s % 60, 2, 10, QChar('0'));
}
