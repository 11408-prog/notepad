#include "datamodel.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QtMath>
#include <algorithm>

DataModel::DataModel(QObject *parent) : QObject(parent) {}

void DataModel::clear()
{
    m_headers.clear();
    m_data.clear();
}

bool DataModel::isEmpty() const
{
    return m_data.isEmpty();
}

// ====================== 数据加载 ======================

bool DataModel::loadCSV(const QString &filePath, QChar delimiter)
{
    clear();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = "无法打开文件：" + filePath;
        emit errorOccurred(m_lastError);
        return false;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    bool firstLine = true;
    int expectedCols = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList fields;
        if (!parseCSVLine(line, delimiter, fields)) continue;

        if (firstLine) {
            m_headers = fields;
            expectedCols = fields.size();
            firstLine = false;
        } else {
            // 列数不足时补空
            while (fields.size() < expectedCols) fields.append("");
            QVector<QVariant> row;
            for (int i = 0; i < expectedCols; ++i) {
                QString val = fields[i].trimmed();
                bool ok = false;
                double d = val.toDouble(&ok);
                row.append(ok ? QVariant(d) : QVariant(val));
            }
            m_data.append(row);
        }
    }

    file.close();

    if (m_headers.isEmpty()) {
        m_lastError = "CSV 文件为空或格式错误";
        emit errorOccurred(m_lastError);
        return false;
    }

    emit dataLoaded(m_data.size(), m_headers.size());
    return true;
}

bool DataModel::loadJSON(const QString &filePath)
{
    clear();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "无法打开文件：" + filePath;
        emit errorOccurred(m_lastError);
        return false;
    }

    QByteArray raw = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (doc.isNull()) {
        m_lastError = "JSON 解析失败：" + err.errorString();
        emit errorOccurred(m_lastError);
        return false;
    }

    // 支持两种格式：
    // 1. 对象数组：[{"col1":v1,"col2":v2}, ...]
    // 2. 列对象：{"col1":[v1,v2,...], "col2":[...]}
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        if (arr.isEmpty()) { m_lastError = "JSON 数组为空"; return false; }

        // 从第一个对象取列名
        QJsonObject firstObj = arr[0].toObject();
        m_headers = firstObj.keys();
        int cols = m_headers.size();

        for (const QJsonValue &val : arr) {
            QJsonObject obj = val.toObject();
            QVector<QVariant> row;
            for (const QString &key : std::as_const(m_headers)) {
                QJsonValue v = obj.value(key);
                if (v.isDouble()) row.append(v.toDouble());
                else row.append(v.toVariant());
            }
            while (row.size() < cols) row.append(QVariant());
            m_data.append(row);
        }
    } else if (doc.isObject()) {
        QJsonObject obj = doc.object();
        m_headers = obj.keys();
        int maxRows = 0;
        QMap<QString, QJsonArray> colArrays;
        for (const QString &key : std::as_const(m_headers)) {
            QJsonArray arr = obj.value(key).toArray();
            colArrays[key] = arr;
            maxRows = qMax(maxRows, arr.size());
        }
        for (int r = 0; r < maxRows; ++r) {
            QVector<QVariant> row;
            for (const QString &key : std::as_const(m_headers)) {
                QJsonArray arr = colArrays[key];
                QJsonValue v = (r < arr.size()) ? arr[r] : QJsonValue();
                if (v.isDouble()) row.append(v.toDouble());
                else row.append(v.toVariant());
            }
            m_data.append(row);
        }
    } else {
        m_lastError = "不支持的 JSON 格式";
        emit errorOccurred(m_lastError);
        return false;
    }

    emit dataLoaded(m_data.size(), m_headers.size());
    return true;
}

// ====================== 数据访问 ======================

QStringList DataModel::columnNames() const { return m_headers; }
int DataModel::rowCount() const { return m_data.size(); }
int DataModel::columnCount() const { return m_headers.size(); }

QVector<double> DataModel::getNumericColumn(const QString &colName) const
{
    int idx = m_headers.indexOf(colName);
    if (idx < 0) return {};
    QVector<double> result;
    for (const auto &row : m_data) {
        if (idx >= row.size()) continue;
        bool ok = false;
        double d = row[idx].toDouble(&ok);
        if (ok) result.append(d);
    }
    return result;
}

QStringList DataModel::getStringColumn(const QString &colName) const
{
    int idx = m_headers.indexOf(colName);
    if (idx < 0) return {};
    QStringList result;
    for (const auto &row : m_data) {
        result.append(idx < row.size() ? row[idx].toString() : "");
    }
    return result;
}

QVariant DataModel::cellValue(int row, int col) const
{
    if (row < 0 || row >= m_data.size()) return QVariant();
    if (col < 0 || col >= m_data[row].size()) return QVariant();
    return m_data[row][col];
}

QString DataModel::cellString(int row, int col) const
{
    return cellValue(row, col).toString();
}

// ====================== 统计计算 ======================

double DataModel::mean(const QString &colName) const
{
    QVector<double> v = getNumericColumn(colName);
    if (v.isEmpty()) return qQNaN();
    double s = 0;
    for (double d : v) s += d;
    return s / v.size();
}

double DataModel::median(const QString &colName) const
{
    QVector<double> v = getNumericColumn(colName);
    if (v.isEmpty()) return qQNaN();
    std::sort(v.begin(), v.end());
    int n = v.size();
    return (n % 2 == 0) ? (v[n/2 - 1] + v[n/2]) / 2.0 : v[n/2];
}

double DataModel::variance(const QString &colName) const
{
    QVector<double> v = getNumericColumn(colName);
    if (v.size() < 2) return qQNaN();
    double m = mean(colName);
    double s = 0;
    for (double d : v) s += (d - m) * (d - m);
    return s / (v.size() - 1);  // 样本方差
}

double DataModel::stddev(const QString &colName) const
{
    double var = variance(colName);
    return qIsNaN(var) ? qQNaN() : qSqrt(var);
}

double DataModel::minValue(const QString &colName) const
{
    QVector<double> v = getNumericColumn(colName);
    if (v.isEmpty()) return qQNaN();
    return *std::min_element(v.begin(), v.end());
}

double DataModel::maxValue(const QString &colName) const
{
    QVector<double> v = getNumericColumn(colName);
    if (v.isEmpty()) return qQNaN();
    return *std::max_element(v.begin(), v.end());
}

double DataModel::sum(const QString &colName) const
{
    QVector<double> v = getNumericColumn(colName);
    double s = 0;
    for (double d : v) s += d;
    return s;
}

double DataModel::correlation(const QString &colA, const QString &colB) const
{
    // 取两列都有数值的行
    int idxA = m_headers.indexOf(colA);
    int idxB = m_headers.indexOf(colB);
    if (idxA < 0 || idxB < 0) return qQNaN();

    QVector<double> va, vb;
    for (const auto &row : m_data) {
        if (idxA >= row.size() || idxB >= row.size()) continue;
        bool okA = false, okB = false;
        double a = row[idxA].toDouble(&okA);
        double b = row[idxB].toDouble(&okB);
        if (okA && okB) { va.append(a); vb.append(b); }
    }
    int n = va.size();
    if (n < 2) return qQNaN();

    double mA = 0, mB = 0;
    for (int i = 0; i < n; ++i) { mA += va[i]; mB += vb[i]; }
    mA /= n; mB /= n;

    double num = 0, denA = 0, denB = 0;
    for (int i = 0; i < n; ++i) {
        double da = va[i] - mA, db = vb[i] - mB;
        num  += da * db;
        denA += da * da;
        denB += db * db;
    }
    double den = qSqrt(denA * denB);
    return (den == 0) ? qQNaN() : num / den;
}

QString DataModel::summaryText(const QString &colName) const
{
    QVector<double> v = getNumericColumn(colName);
    if (v.isEmpty())
        return QString("列 [%1] 无数值数据").arg(colName);

    auto fmt = [](double d) -> QString {
        return qIsNaN(d) ? "N/A" : QString::number(d, 'f', 4);
    };

    return QString(
        "列: %1\n"
        "  样本数:  %2\n"
        "  最小值:  %3\n"
        "  最大值:  %4\n"
        "  均值:    %5\n"
        "  中位数:  %6\n"
        "  标准差:  %7\n"
        "  方差:    %8\n"
        "  总和:    %9"
    ).arg(colName)
     .arg(v.size())
     .arg(fmt(minValue(colName)))
     .arg(fmt(maxValue(colName)))
     .arg(fmt(mean(colName)))
     .arg(fmt(median(colName)))
     .arg(fmt(stddev(colName)))
     .arg(fmt(variance(colName)))
     .arg(fmt(sum(colName)));
}

QString DataModel::fullSummaryText() const
{
    if (m_headers.isEmpty()) return "无数据";
    QStringList parts;
    parts << QString("数据集：%1 行 × %2 列\n").arg(rowCount()).arg(columnCount());
    for (const QString &col : m_headers) {
        if (!getNumericColumn(col).isEmpty())
            parts << summaryText(col) << "\n";
    }
    return parts.join("\n");
}

// ====================== 辅助 ======================

bool DataModel::parseCSVLine(const QString &line, QChar delimiter,
                              QStringList &fields) const
{
    fields.clear();
    QString current;
    bool inQuotes = false;
    for (int i = 0; i < line.size(); ++i) {
        QChar c = line[i];
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i+1] == '"') {
                current += '"'; ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == delimiter && !inQuotes) {
            fields.append(current);
            current.clear();
        } else {
            current += c;
        }
    }
    fields.append(current);
    return true;
}
