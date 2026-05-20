#ifndef DATAANALYSISWIDGET_H
#define DATAANALYSISWIDGET_H

#include <QWidget>
#include <QHash>
#include <QVBoxLayout>      // 新增

class DataModel;
class ChartWidget;
class PythonScriptRunner;
class QTableView;
class QStandardItemModel;
class QPlainTextEdit;
class QSplitter;
class QLabel;
class QComboBox;
class QPushButton;
class QsciScintilla;

class DataAnalysisWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DataAnalysisWidget(QWidget *parent = nullptr);
    ~DataAnalysisWidget();

private slots:
    void onImportCSV();
    void onImportJSON();
    void onRunScript();
    void onExportResult();
    void onColumnSelectionChanged(const QString &colName);

private:
    void setupUI();
    void setupToolBar(QWidget *parent, QVBoxLayout *layout);
    void setupScriptEditor(QWidget *parent, QVBoxLayout *layout);
    void refreshDataView(int rows, int cols);
    void populateTable();
    QString defaultScriptTemplate() const;

           // 新增方法
    bool exportDataToCSV(const QString &filePath, int maxRows = 10000);
    void runPythonScriptWithData(const QString &script);
    QString generateDataOverview() const;
    void updateStatisticsCache();          // 预计算统计信息

    DataModel          *m_model       = nullptr;
    ChartWidget        *m_chartWidget = nullptr;
    PythonScriptRunner *m_scriptRunner = nullptr;
    QTableView         *m_tableView   = nullptr;
    QStandardItemModel *m_tableModel  = nullptr;

    QsciScintilla      *m_scriptEditor = nullptr;
    QPlainTextEdit     *m_summaryOutput = nullptr;
    QComboBox          *m_colCombo = nullptr;
    QLabel             *m_statusLabel = nullptr;
    QPushButton        *m_runScriptButton = nullptr;

           // 统计缓存结构
    struct ColumnStats {
        double mean, median, stddev, variance, min, max, sum;
        int count;
    };
    QHash<QString, ColumnStats> m_statsCache;
    bool m_statsValid = false;
};

#endif // DATAANALYSISWIDGET_H