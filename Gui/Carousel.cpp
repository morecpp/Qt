#include "Carousel.h"

#include <QDebug>
#include <QPainter>
#include <QMatrix4x4>
#include <QVector3D>

struct CarouselItem {
    CarouselItem(const QString &imagePath) : pixmap(imagePath) {}

    QPixmap   pixmap; // item 的图片
    QRect     rect;   // item 所在矩形
    QVector3D center; // item 的中心坐标
};

Carousel::Carousel(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet("background: #444;");

    // [1] 初始化 items
    for (int i = 1; i <= 17; ++i) {
        items << new CarouselItem(QString("%1.jpg").arg(i));
    }

    QVector3D rotateAxis(0, 1, -0.2);     // items 绕此轴旋转
    QVector3D frontItemCenter(0, 0, 200); // 最前面 item 的中心坐标
    int       frontItemWidth = 200;       // 最前面 item 的宽
    int       frontItemHeight = 100;      // 最前面 item 的高
    double    minZoom = 0.3;              // 最后面 item 与最前面 item 的比例

    for (int i = 0; i < items.size(); ++i) {
        CarouselItem *item = items[i];
        QMatrix4x4 matrix;

        // [2] item 绕轴 rotateAxis 旋转，得到旋转后 item 的中心坐标
        matrix.rotate(360 / items.size() * i, rotateAxis);
        item->center = matrix.map(frontItemCenter);

        // [3] 根据 item 中心到 distance 的距离，计算 item 的矩形范围
        double rate = minZoom +  (1-minZoom) * (1 - (frontItemCenter.z() - item->center.z()) / 400);
        item->rect.setRect(0, 0, frontItemWidth*rate, frontItemHeight*rate);
        item->rect.moveCenter(item->center.toPoint());
    }

    // [4] 把 items 按 z 坐标升序排序，z 越小的 item 越先绘制，避免后面的 item 覆盖前面的 item
    std::sort(items.begin(), items.end(), [](CarouselItem* a, CarouselItem* b) -> bool {
        return a->center.z() < b->center.z();
    });
}

Carousel::~Carousel() {
}

void Carousel::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.translate(width()/2, height()/2);

    // [5] 显示 items
    foreach (const CarouselItem *item, items) {
        painter.drawPixmap(item->rect, item->pixmap);
    }
}



