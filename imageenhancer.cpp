#include "imageenhancer.h"
#include <QtMath>
#include <QVector>
#include <algorithm>

// ====================== 高斯核 ======================

QVector<double> ImageEnhancer::buildGaussianKernel(int radius)
{
    int size = 2 * radius + 1;
    QVector<double> kernel(size);
    double sigma = radius / 2.0;
    double sum = 0.0;
    for (int i = 0; i < size; ++i) {
        double x = i - radius;
        kernel[i] = qExp(-(x * x) / (2 * sigma * sigma));
        sum += kernel[i];
    }
    for (auto &v : kernel) v /= sum;
    return kernel;
}

// ====================== 水平卷积 ======================

QImage ImageEnhancer::convolveH(const QImage &src, const QVector<double> &kernel)
{
    int radius = kernel.size() / 2;
    int w = src.width(), h = src.height();
    QImage dst(w, h, QImage::Format_RGB32);

    for (int y = 0; y < h; ++y) {
        const QRgb *srcLine = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        QRgb       *dstLine = reinterpret_cast<QRgb*>(dst.scanLine(y));
        for (int x = 0; x < w; ++x) {
            double r = 0, g = 0, b = 0;
            for (int k = -radius; k <= radius; ++k) {
                int sx = qBound(0, x + k, w - 1);
                QRgb px = srcLine[sx];
                double w = kernel[k + radius];
                r += qRed(px)   * w;
                g += qGreen(px) * w;
                b += qBlue(px)  * w;
            }
            dstLine[x] = qRgb(qBound(0,(int)r,255),
                               qBound(0,(int)g,255),
                               qBound(0,(int)b,255));
        }
    }
    return dst;
}

// ====================== 垂直卷积 ======================

QImage ImageEnhancer::convolveV(const QImage &src, const QVector<double> &kernel)
{
    int radius = kernel.size() / 2;
    int w = src.width(), h = src.height();
    QImage dst(w, h, QImage::Format_RGB32);

    for (int y = 0; y < h; ++y) {
        QRgb *dstLine = reinterpret_cast<QRgb*>(dst.scanLine(y));
        for (int x = 0; x < w; ++x) {
            double r = 0, g = 0, b = 0;
            for (int k = -radius; k <= radius; ++k) {
                int sy = qBound(0, y + k, h - 1);
                QRgb px = reinterpret_cast<const QRgb*>(src.constScanLine(sy))[x];
                double wt = kernel[k + radius];
                r += qRed(px)   * wt;
                g += qGreen(px) * wt;
                b += qBlue(px)  * wt;
            }
            dstLine[x] = qRgb(qBound(0,(int)r,255),
                               qBound(0,(int)g,255),
                               qBound(0,(int)b,255));
        }
    }
    return dst;
}

// ====================== 高斯模糊 ======================

QImage ImageEnhancer::gaussianBlur(const QImage &src, int radius)
{
    if (src.isNull() || radius < 1) return src;
    radius = qBound(1, radius, 10);

    QImage rgb = src.convertToFormat(QImage::Format_RGB32);
    QVector<double> kernel = buildGaussianKernel(radius);
    QImage tmp = convolveH(rgb, kernel);
    return convolveV(tmp, kernel);
}

// ====================== 直方图均衡化 ======================

QImage ImageEnhancer::histogramEqualize(const QImage &src)
{
    if (src.isNull()) return src;
    QImage rgb = src.convertToFormat(QImage::Format_RGB32);
    int w = rgb.width(), h = rgb.height();
    int total = w * h;

    // 对 R G B 三通道分别均衡
    for (int ch = 0; ch < 3; ++ch) {
        // 1. 统计直方图
        int hist[256] = {};
        for (int y = 0; y < h; ++y) {
            const QRgb *line = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));
            for (int x = 0; x < w; ++x) {
                int val = (ch == 0) ? qRed(line[x])
                        : (ch == 1) ? qGreen(line[x])
                                    : qBlue(line[x]);
                hist[val]++;
            }
        }
        // 2. 累积分布函数
        int cdf[256] = {};
        cdf[0] = hist[0];
        for (int i = 1; i < 256; ++i) cdf[i] = cdf[i-1] + hist[i];

        // 找最小非零 cdf
        int cdfMin = 0;
        for (int i = 0; i < 256; ++i) { if (cdf[i] > 0) { cdfMin = cdf[i]; break; } }

        // 3. 映射表
        int lut[256] = {};
        for (int i = 0; i < 256; ++i) {
            if (total == cdfMin)
                lut[i] = i;
            else
                lut[i] = qBound(0, (int)qRound((double)(cdf[i] - cdfMin)
                                                 / (total - cdfMin) * 255.0), 255);
        }

        // 4. 应用映射
        for (int y = 0; y < h; ++y) {
            QRgb *line = reinterpret_cast<QRgb*>(rgb.scanLine(y));
            for (int x = 0; x < w; ++x) {
                int r = qRed(line[x]), g = qGreen(line[x]), b = qBlue(line[x]);
                if      (ch == 0) r = lut[r];
                else if (ch == 1) g = lut[g];
                else              b = lut[b];
                line[x] = qRgb(r, g, b);
            }
        }
    }
    return rgb;
}

// ====================== 轻度锐化（Unsharp Mask） ======================

QImage ImageEnhancer::sharpen(const QImage &src, double strength)
{
    // unsharp mask = original + strength * (original - blurred)
    QImage blurred = gaussianBlur(src, 1);
    int w = src.width(), h = src.height();
    QImage rgb = src.convertToFormat(QImage::Format_RGB32);
    QImage result(w, h, QImage::Format_RGB32);

    for (int y = 0; y < h; ++y) {
        const QRgb *srcLine  = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));
        const QRgb *blurLine = reinterpret_cast<const QRgb*>(blurred.constScanLine(y));
        QRgb       *dstLine  = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            auto sharp = [&](int orig, int blur) {
                return qBound(0, (int)(orig + strength * (orig - blur)), 255);
            };
            dstLine[x] = qRgb(
                sharp(qRed(srcLine[x]),   qRed(blurLine[x])),
                sharp(qGreen(srcLine[x]), qGreen(blurLine[x])),
                sharp(qBlue(srcLine[x]),  qBlue(blurLine[x]))
            );
        }
    }
    return result;
}

// ====================== 一键自动增强 ======================

QImage ImageEnhancer::autoEnhance(const QImage &src)
{
    if (src.isNull()) return src;
    // 先均衡化提升对比度，再轻度锐化
    QImage equalized = histogramEqualize(src);
    return sharpen(equalized, 0.35);
}
