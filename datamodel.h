#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QObject>
#include <QVector>
#include <QStringList>
#include <QMap>
#include <QVariant>

/**
 * DataModel：负责数据的加载、存储与统计计算
 * 支持 CSV / JSON 两种格式
 */
class DataModel : public QObject
{
    Q_OBJECT
public:
    explicit DataModel(QObject *parent = nullptr);

    // ========== 数据加载 ==========
    bool loadCSV(const QString &filePath, QChar delimiter = ',');
    bool loadJSON(const QString &filePath);
    void clear();
    bool isEmpty() const;

    // ========== 数据访问 ==========
    QStringList columnNames() const;
    int rowCount() const;
    int columnCount() const;

    // 返回某列所有数值（非数字行自动跳过）
    QVector<double> getNumericColumn(const QString &colName) const;
    // 返回某列所有字符串值
    QStringList getStringColumn(const QString &colName) const;
    // 返回某单元格原始值
    QVariant cellValue(int row, int col) const;
    QString cellString(int row, int col) const;

    // ========== 统计计算 ==========
    double mean(const QString &colName) const;
    double median(const QString &colName) const;
    double variance(const QString &colName) const;
    double stddev(const QString &colName) const;
    double minValue(const QString &colName) const;
    double maxValue(const QString &colName) const;
    double sum(const QString &colName) const;
    // Pearson 相关系数
    double correlation(const QString &colA, const QString &colB) const;

    // 返回完整统计摘要文本
    QString summaryText(const QString &colName) const;
    QString fullSummaryText() const;

    // ========== 错误信息 ==========
    QString lastError() const { return m_lastError; }

signals:
    void dataLoaded(int rows, int cols);
    void errorOccurred(const QString &msg);

private:
    QStringList m_headers;
    // m_data[row][col]
    QVector<QVector<QVariant>> m_data;
    QString m_lastError;

    bool parseCSVLine(const QString &line, QChar delimiter, QStringList &fields) const;
};

#endif // DATAMODEL_H
