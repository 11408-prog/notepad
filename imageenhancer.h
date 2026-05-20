#ifndef IMAGEENHANCER_H
#define IMAGEENHANCER_H

#include <QImage>

/**
 * ImageEnhancer：纯 Qt 实现的图像处理工具类
 * 无需 OpenCV 等第三方库
 *
 * 功能：
 *   - 高斯模糊（降噪）
 *   - 直方图均衡化（亮度/对比度自动增强）
 *   - 组合一键增强（均衡化 + 轻度锐化）
 */
class ImageEnhancer
{
public:
    /**
     * 高斯模糊
     * @param src    原始图像
     * @param radius 模糊半径 1~10
     */
    static QImage gaussianBlur(const QImage &src, int radius);

    /**
     * 直方图均衡化（对 RGB 各通道独立均衡，保留色调）
     * @param src 原始图像
     */
    static QImage histogramEqualize(const QImage &src);

    /**
     * 一键自动增强：先均衡化再轻度锐化
     * @param src 原始图像
     */
    static QImage autoEnhance(const QImage &src);

private:
    // 构建高斯核
    static QVector<double> buildGaussianKernel(int radius);
    // 单通道水平/垂直卷积
    static QImage convolveH(const QImage &src, const QVector<double> &kernel);
    static QImage convolveV(const QImage &src, const QVector<double> &kernel);
    // 轻度锐化（unsharp mask）
    static QImage sharpen(const QImage &src, double strength = 0.4);
};

#endif // IMAGEENHANCER_H
