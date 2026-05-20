#include "dataanalysiswidget.h"
#include "datamodel.h"
#include "chartwidget.h"
#include "pythonscriptrunner.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTableView>
#include <QStandardItemModel>
#include <QPlainTextEdit>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QHeaderView>
#include <QFont>
#include <QSignalBlocker>
#include <QTemporaryFile>
#include <QProgressDialog>
#include <QSet>

#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerpython.h>

DataAnalysisWidget::DataAnalysisWidget(QWidget *parent)
    : QWidget(parent)
      , m_model(new DataModel(this))
      , m_scriptRunner(new PythonScriptRunner(this))
{
    setupUI();

    connect(m_model, &DataModel::dataLoaded, this, &DataAnalysisWidget::refreshDataView);
    connect(m_model, &DataModel::errorOccurred, this, [this](const QString &msg) {
        QMessageBox::warning(this, "数据加载失败", msg);
    });
    connect(m_scriptRunner, &PythonScriptRunner::outputReady, this, [this](const QString &text) {
        m_summaryOutput->appendPlainText(text);
    });
    connect(m_scriptRunner, &PythonScriptRunner::failedToStart, this, [this]() {
        QMessageBox::warning(this, "脚本运行失败", "无法启动 Python，请确认 Python 已安装并在 PATH 中。");
    });
    connect(m_scriptRunner, &PythonScriptRunner::finished, this,
            [this](int exitCode, QProcess::ExitStatus status) {
                const QString msg = (status == QProcess::CrashExit || exitCode != 0)
                ? QString("[脚本异常退出，退出码 %1]").arg(exitCode)
                : "[脚本执行完毕]";
                m_summaryOutput->appendPlainText(msg);
                m_summaryOutput->appendPlainText("----------------------\n");
                m_statusLabel->setText("脚本运行结束");
                if (m_runScriptButton) m_runScriptButton->setEnabled(true);
            });
}

DataAnalysisWidget::~DataAnalysisWidget()
{
    if (m_scriptRunner && m_scriptRunner->isRunning())
        m_scriptRunner->terminate(500);
}

void DataAnalysisWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(4);

    setupToolBar(this, mainLayout);

    QSplitter *mainSplitter = new QSplitter(Qt::Vertical, this);
    QSplitter *topSplitter = new QSplitter(Qt::Horizontal, mainSplitter);

           // 数据表格容器
    QWidget *tableContainer = new QWidget();
    QVBoxLayout *tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *tableTitle = new QLabel("原始数据", tableContainer);
    tableTitle->setStyleSheet("font-weight: bold; padding: 2px 0;");
    m_tableView = new QTableView(tableContainer);
    m_tableModel = new QStandardItemModel(this);
    m_tableView->setModel(m_tableModel);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->verticalHeader()->setDefaultSectionSize(24);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableLayout->addWidget(tableTitle);
    tableLayout->addWidget(m_tableView);

           // 图表
    m_chartWidget = new ChartWidget(this);
    topSplitter->addWidget(tableContainer);
    topSplitter->addWidget(m_chartWidget);
    topSplitter->setStretchFactor(0, 2);
    topSplitter->setStretchFactor(1, 3);

           // 下半部分：脚本编辑器 + 输出
    QSplitter *bottomSplitter = new QSplitter(Qt::Horizontal, mainSplitter);
    QWidget *scriptContainer = new QWidget();
    QVBoxLayout *scriptLayout = new QVBoxLayout(scriptContainer);
    scriptLayout->setContentsMargins(0, 0, 0, 0);
    setupScriptEditor(scriptContainer, scriptLayout);

    QWidget *outputContainer = new QWidget();
    QVBoxLayout *outputLayout = new QVBoxLayout(outputContainer);
    outputLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *outputTopLayout = new QHBoxLayout();
    QLabel *statsTitle = new QLabel("统计摘要", outputContainer);
    statsTitle->setStyleSheet("font-weight: bold; padding: 2px 0;");
    QLabel *colLabel = new QLabel("列：", outputContainer);
    m_colCombo = new QComboBox(outputContainer);
    m_colCombo->setFixedWidth(120);
    connect(m_colCombo, &QComboBox::currentTextChanged,
            this, &DataAnalysisWidget::onColumnSelectionChanged);
    QPushButton *exportBtn = new QPushButton("导出结果", outputContainer);
    exportBtn->setFixedWidth(80);
    connect(exportBtn, &QPushButton::clicked, this, &DataAnalysisWidget::onExportResult);
    outputTopLayout->addWidget(statsTitle);
    outputTopLayout->addWidget(colLabel);
    outputTopLayout->addWidget(m_colCombo);
    outputTopLayout->addStretch();
    outputTopLayout->addWidget(exportBtn);

    m_summaryOutput = new QPlainTextEdit(outputContainer);
    m_summaryOutput->setReadOnly(true);
    QFont summaryFont("Cascadia Code", 10);
    summaryFont.setStyleHint(QFont::Monospace);
    m_summaryOutput->setFont(summaryFont);
    m_summaryOutput->setPlaceholderText("导入数据后，统计摘要与脚本输出将显示在此处");

    outputLayout->addLayout(outputTopLayout);
    outputLayout->addWidget(m_summaryOutput);

    bottomSplitter->addWidget(scriptContainer);
    bottomSplitter->addWidget(outputContainer);
    bottomSplitter->setStretchFactor(0, 1);
    bottomSplitter->setStretchFactor(1, 1);

    mainSplitter->addWidget(topSplitter);
    mainSplitter->addWidget(bottomSplitter);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 2);

    mainLayout->addWidget(mainSplitter);

    m_statusLabel = new QLabel("尚未加载数据", this);
    m_statusLabel->setStyleSheet("color: #888; padding: 2px 4px;");
    mainLayout->addWidget(m_statusLabel);
}

void DataAnalysisWidget::setupToolBar(QWidget *parent, QVBoxLayout *layout)
{
    QHBoxLayout *tbLayout = new QHBoxLayout();
    tbLayout->setSpacing(6);

    auto makeBtn = [&](const QString &text, const QString &tip, auto slot) {
        QPushButton *btn = new QPushButton(text, parent);
        btn->setToolTip(tip);
        btn->setFixedHeight(28);
        connect(btn, &QPushButton::clicked, this, slot);
        tbLayout->addWidget(btn);
        return btn;
    };

    makeBtn("导入 CSV",  "从 CSV 文件加载数据",       &DataAnalysisWidget::onImportCSV);
    makeBtn("导入 JSON", "从 JSON 文件加载数据",      &DataAnalysisWidget::onImportJSON);
    tbLayout->addStretch();

    layout->addLayout(tbLayout);
}

void DataAnalysisWidget::setupScriptEditor(QWidget *parent, QVBoxLayout *layout)
{
    auto *scriptTitleLayout = new QHBoxLayout();
    auto *scriptTitle = new QLabel("Python 脚本", parent);
    scriptTitle->setStyleSheet("font-weight: bold; padding: 2px 0;");

    m_runScriptButton = new QPushButton("▶ 运行", parent);
    m_runScriptButton->setFixedWidth(70);
    m_runScriptButton->setToolTip("运行 Python 脚本（需本机安装 Python）");
    connect(m_runScriptButton, &QPushButton::clicked, this, &DataAnalysisWidget::onRunScript);

    scriptTitleLayout->addWidget(scriptTitle);
    scriptTitleLayout->addStretch();
    scriptTitleLayout->addWidget(m_runScriptButton);

    m_scriptEditor = new QsciScintilla(parent);
    auto *pyLexer = new QsciLexerPython(m_scriptEditor);
    QFont monoFont("Cascadia Code", 10);
    monoFont.setStyleHint(QFont::Monospace);
    pyLexer->setFont(monoFont);

    m_scriptEditor->setLexer(pyLexer);
    m_scriptEditor->setFont(monoFont);
    m_scriptEditor->setMarginType(1, QsciScintilla::NumberMargin);
    m_scriptEditor->setMarginWidth(1, "000");
    m_scriptEditor->setAutoIndent(true);
    m_scriptEditor->setIndentationGuides(true);
    m_scriptEditor->setIndentationsUseTabs(false);
    m_scriptEditor->setTabWidth(4);
    m_scriptEditor->setText(defaultScriptTemplate());

    layout->addLayout(scriptTitleLayout);
    layout->addWidget(m_scriptEditor);
}

void DataAnalysisWidget::refreshDataView(int rows, int cols)
{
    populateTable();
    m_chartWidget->setModel(m_model);

    QSignalBlocker blocker(m_colCombo);
    m_colCombo->clear();
    m_colCombo->addItems(m_model->columnNames());

    const int displayRows = qMin(rows, 1000);
    const QString previewHint = rows > displayRows
                                    ? QString("，表格预览前 %1 行").arg(displayRows)
                                    : QString();
    m_statusLabel->setText(QString("已加载 %1 行 × %2 列%3").arg(rows).arg(cols).arg(previewHint));
    m_summaryOutput->setPlainText(generateDataOverview() + "\n\n" + m_model->fullSummaryText());
    updateStatisticsCache();
}

void DataAnalysisWidget::populateTable()
{
    m_tableModel->clear();

    if (!m_model || m_model->isEmpty()) return;

    QStringList headers = m_model->columnNames();
    m_tableModel->setHorizontalHeaderLabels(headers);

    int rows = m_model->rowCount();
    int cols = m_model->columnCount();
    int displayRows = qMin(rows, 1000);
    m_tableModel->setRowCount(displayRows);

    for (int r = 0; r < displayRows; ++r) {
        for (int c = 0; c < cols; ++c) {
            auto *item = new QStandardItem(m_model->cellString(r, c));
            item->setTextAlignment(Qt::AlignCenter);
            m_tableModel->setItem(r, c, item);
        }
    }
}

// ====================== 数据加载 ======================
void DataAnalysisWidget::onImportCSV()
{
    QString path = QFileDialog::getOpenFileName(
        this, "导入 CSV", QDir::homePath(),
        "CSV 文件 (*.csv);;文本文件 (*.txt);;所有文件 (*.*)");
    if (path.isEmpty()) return;
    m_model->loadCSV(path);
}

void DataAnalysisWidget::onImportJSON()
{
    QString path = QFileDialog::getOpenFileName(
        this, "导入 JSON", QDir::homePath(),
        "JSON 文件 (*.json);;所有文件 (*.*)");
    if (path.isEmpty()) return;
    m_model->loadJSON(path);
}

// ====================== 新增方法 ======================
bool DataAnalysisWidget::exportDataToCSV(const QString &filePath, int maxRows)
{
    if (m_model->isEmpty()) return false;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    QStringList headers = m_model->columnNames();
    out << headers.join(",") << "\n";

    int rows = qMin(m_model->rowCount(), maxRows);
    QProgressDialog progress("导出数据中...", "取消", 0, rows, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(200);

    for (int r = 0; r < rows; ++r) {
        if (progress.wasCanceled()) {
            file.remove();
            return false;
        }
        progress.setValue(r);
        QStringList rowVals;
        for (int c = 0; c < headers.size(); ++c) {
            QString val = m_model->cellString(r, c);
            if (val.contains(',') || val.contains('"') || val.contains('\n')) {
                val.replace("\"", "\"\"");
                val = "\"" + val + "\"";
            }
            rowVals << val;
        }
        out << rowVals.join(",") << "\n";
    }
    progress.setValue(rows);
    return true;
}

void DataAnalysisWidget::runPythonScriptWithData(const QString &script)
{
    // 已直接在 onRunScript 中实现，此函数保留供将来使用
    Q_UNUSED(script);
}

QString DataAnalysisWidget::generateDataOverview() const
{
    if (m_model->isEmpty()) return "无数据";
    QStringList lines;
    lines << "╔══════════════════════════════════════════════════════════════╗";
    lines << "║                        数据概览                              ║";
    lines << "╚══════════════════════════════════════════════════════════════╝";
    lines << QString("总行数: %1, 总列数: %2").arg(m_model->rowCount()).arg(m_model->columnCount());
    lines << "";
    lines << "列名                     类型       非空数    唯一值";
    lines << "--------------------------------------------------";

    for (const QString &col : m_model->columnNames()) {
        int idx = m_model->columnNames().indexOf(col);
        bool allNumeric = true;
        for (int r = 0; r < qMin(10, m_model->rowCount()); ++r) {
            bool ok;
            m_model->cellString(r, idx).toDouble(&ok);
            if (!ok) { allNumeric = false; break; }
        }
        QString type = allNumeric ? "数值" : "文本";

        int nonNull = 0;
        QSet<QString> uniqueSet;
        int limit = qMin(5000, m_model->rowCount());
        for (int r = 0; r < limit; ++r) {
            QString val = m_model->cellString(r, idx);
            if (!val.isEmpty()) nonNull++;
            uniqueSet.insert(val);
        }
        int unique = uniqueSet.size();
        if (limit < m_model->rowCount()) {
            lines << QString("%1 %2 %3 %4 (近似)")
                         .arg(col, -24).arg(type, -8).arg(nonNull, -8).arg(unique);
        } else {
            lines << QString("%1 %2 %3 %4")
            .arg(col, -24).arg(type, -8).arg(nonNull, -8).arg(unique);
        }
    }
    return lines.join("\n");
}

void DataAnalysisWidget::updateStatisticsCache()
{
    if (m_model->isEmpty()) {
        m_statsValid = false;
        return;
    }
    m_statsCache.clear();
    for (const QString &col : m_model->columnNames()) {
        QVector<double> nums = m_model->getNumericColumn(col);
        if (nums.isEmpty()) continue;
        ColumnStats st;
        st.count = nums.size();
        double sum = 0;
        for (double v : nums) sum += v;
        st.sum = sum;
        st.mean = sum / st.count;
        std::sort(nums.begin(), nums.end());
        if (st.count % 2 == 0)
            st.median = (nums[st.count/2 - 1] + nums[st.count/2]) / 2.0;
        else
            st.median = nums[st.count/2];
        st.min = nums.first();
        st.max = nums.last();
        double var = 0;
        for (double v : nums) var += (v - st.mean) * (v - st.mean);
        st.variance = var / (st.count - 1);
        st.stddev = sqrt(st.variance);
        m_statsCache[col] = st;
    }
    m_statsValid = true;
}

void DataAnalysisWidget::onRunScript()
{
    if (m_scriptRunner->isRunning()) {
        QMessageBox::information(this, "提示", "已有脚本正在运行，请等待完成。");
        return;
    }

    if (m_model->isEmpty()) {
        QMessageBox::warning(this, "无数据", "请先通过 CSV/JSON 导入数据。");
        return;
    }

    QTemporaryFile tmpFile;
    if (!tmpFile.open()) {
        QMessageBox::warning(this, "错误", "无法创建临时文件。");
        return;
    }
    QString csvPath = tmpFile.fileName();
    tmpFile.close();

    if (!exportDataToCSV(csvPath, 50000)) {
        QMessageBox::warning(this, "错误", "无法导出数据到临时 CSV。");
        return;
    }

    QString preamble = QString(
                           "import pandas as pd\n"
                           "import sys\n"
                           "import os\n"
                           "data_file = r'%1'\n"
                           "df = pd.read_csv(data_file, encoding='utf-8')\n"
                           "print(f'数据已加载: {df.shape[0]} 行, {df.shape[1]} 列')\n"
                           "print('列名:', list(df.columns))\n\n"
                           ).arg(csvPath);

    QString fullScript = preamble + m_scriptEditor->text();

    m_summaryOutput->appendPlainText("\n------ 脚本输出 ------");
    m_statusLabel->setText("脚本运行中...");
    if (m_runScriptButton) m_runScriptButton->setEnabled(false);

    m_scriptRunner->run(fullScript, csvPath);
}

void DataAnalysisWidget::onExportResult()
{
    QString path = QFileDialog::getSaveFileName(
        this, "导出统计结果", "analysis_result.txt",
        "文本文件 (*.txt);;所有文件 (*.*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法写入文件：" + path);
        return;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << m_summaryOutput->toPlainText();
    file.close();
    m_statusLabel->setText("结果已导出：" + path);
}

void DataAnalysisWidget::onColumnSelectionChanged(const QString &colName)
{
    if (!m_model || colName.isEmpty()) return;

    if (m_statsValid && m_statsCache.contains(colName)) {
        const ColumnStats &st = m_statsCache[colName];
        QString text = QString(
                           "列: %1\n"
                           "  样本数: %2\n"
                           "  最小值: %3\n"
                           "  最大值: %4\n"
                           "  均值:   %5\n"
                           "  中位数: %6\n"
                           "  标准差: %7\n"
                           "  方差:   %8\n"
                           "  总和:   %9"
                           ).arg(colName)
                           .arg(st.count)
                           .arg(st.min, 0, 'f', 4)
                           .arg(st.max, 0, 'f', 4)
                           .arg(st.mean, 0, 'f', 4)
                           .arg(st.median, 0, 'f', 4)
                           .arg(st.stddev, 0, 'f', 4)
                           .arg(st.variance, 0, 'f', 4)
                           .arg(st.sum, 0, 'f', 4);
        m_summaryOutput->setPlainText(text);
    } else {
        m_summaryOutput->setPlainText(m_model->summaryText(colName));
    }
}

QString DataAnalysisWidget::defaultScriptTemplate() const
{
    return
        "# 数据分析脚本示例\n"
        "# 数据已通过 CSV/JSON 加载，并自动导出为临时 CSV 文件\n"
        "# 变量 df 是 pandas DataFrame，可直接分析\n\n"
        "print('数据预览:')\n"
        "print(df.head())\n\n"
        "print('\\n描述性统计:')\n"
        "print(df.describe())\n\n"
        "# 在这里编写您的分析代码\n";
}