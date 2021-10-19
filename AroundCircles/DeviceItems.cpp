#include "DeviceItems.h"

#include <QDebug>
#include <QPainter>
#include <QMimeData>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>

// 查找 scene 中名字为传入的 name 的 item
DeviceItem *findDeviceItemByName(QGraphicsScene *scene, const QString &name) {
    for (QGraphicsItem *item : scene->items()) {
        DeviceItem *device = dynamic_cast<DeviceItem *>(item);

        if (nullptr != device && device->getName() == name) {
            return device;
        }
    }

    return nullptr;
}

/*-----------------------------------------------------------------------------|
 |                                  DialPlate                                  |
 |----------------------------------------------------------------------------*/
DialPlate::DialPlate(int n, double radius, int p, QGraphicsItem *parent)
    : QGraphicsEllipseItem(QRectF(-p-radius, -p-radius, p+p+radius+radius, p+p+radius+radius), parent), n(n), radius(radius) {
}

void DialPlate::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHints(QPainter::Antialiasing);

    // 圆上的序号: 逆时针，1 到 n
    for (int i = 0; i < n; ++i) {
        painter->save();
        painter->rotate(-360.0/n*i);
        painter->translate(0, radius-5);
        painter->drawText(QRectF(-8, -8, 16, 16), Qt::AlignCenter, QString::number(i+1));
        painter->restore();
    }

    // 画圆
    painter->drawEllipse(rect());
}


/*-----------------------------------------------------------------------------|
 |                                 DeviceItem                                  |
 |----------------------------------------------------------------------------*/
DeviceItem::~DeviceItem() {}

// 设置背景色
void DeviceItem::setBgcolor(const QString &bgcolor) {
    QColor temp(bgcolor);
    this->bgcolor = temp.isValid() ? temp : Qt::transparent;
}

// 设置显示的值
void DeviceItem::setValue(const QString &value) {
    if (this->valueChangable) {
        this->value = value;
    }
}

// 设置 scene 中名字为 name 的 CircleDevice 的 item 的背景色
void DeviceItem::setBgcolorByName(QGraphicsScene *scene, const QString &name, const QString &bgcolor) {
    DeviceItem *item = findDeviceItemByName(scene, name);

    if (nullptr != item) {
        item->setBgcolor(bgcolor);
        item->doUpdate();
    }
}

// 设置 scene 中名字为 name 的 DeviceItem 的 item 显示的值
void DeviceItem::setValueByName(QGraphicsScene *scene, const QString &name, const QString &value) {
    DeviceItem *item = findDeviceItemByName(scene, name);

    if (nullptr != item) {
        item->setValue(value);
        item->doUpdate();
    }
}

// 重置背景色和名字
void DeviceItem::reset() {
    name    = "";
    bgcolor = Qt::transparent;

    if (valueChangable) {
        value = "";
    }
}

// 重置 scene 中名字为 name 的圆的名字和背景色
void DeviceItem::resetByName(QGraphicsScene *scene, const QString &name) {
    DeviceItem *item = findDeviceItemByName(scene, name);

    if (nullptr != item) {
        item->reset();
        item->doUpdate();
    }
}

void DeviceItem::doUpdate() {
    QGraphicsItem *item = dynamic_cast<QGraphicsItem *>(this);

    if (nullptr != item) {
        item->update();
    }
}

/*-----------------------------------------------------------------------------|
 |                                CircleDevice                                 |
 |----------------------------------------------------------------------------*/
CircleDevice::CircleDevice(const QString &name, const QString &value, double radius, bool valueChangable, QGraphicsItem *parent)
    : QGraphicsEllipseItem(QRectF(-radius, -radius, radius+radius, radius+radius), parent) {
    setAcceptDrops(true);
    setAcceptHoverEvents(true);
    this->name  = name;
    this->value = value;
    this->valueChangable = valueChangable;
}

// 鼠标进入和离开、拖拽进入和离开时高亮圆
void CircleDevice::hoverEnterEvent(QGraphicsSceneHoverEvent *) {
    hover = true;
    update();
}

void CircleDevice::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    hover = false;
    update();
}

void CircleDevice::dragEnterEvent(QGraphicsSceneDragDropEvent *) {
    hover = true;
    update();
}

void CircleDevice::dragLeaveEvent(QGraphicsSceneDragDropEvent *) {
    hover = false;
    update();
}

// 拖放时设置设备的背景色和名字
void CircleDevice::dropEvent(QGraphicsSceneDragDropEvent *event) {
    if (event->mimeData()->hasFormat("text/DnD-DEVICE-CIRCLE")) {
        event->accept();

        // 数据格式: name,color
        QStringList data = QString::fromUtf8(event->mimeData()->data("text/DnD-DEVICE-CIRCLE")).split(",");
        QString name = data.value(0);

        DeviceItem::resetByName(scene(), name);
        this->name = name;
        this->setBgcolor(data.value(1));
    }

    hover = false;
    update();
}

void CircleDevice::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHints(QPainter::Antialiasing);
    painter->setPen(hover ? Qt::darkRed : Qt::black);
    painter->setBrush(bgcolor);
    painter->drawEllipse(rect());
    painter->drawText(rect(), Qt::AlignCenter, value);
}


/*-----------------------------------------------------------------------------|
 |                                 RectDevice                                  |
 |----------------------------------------------------------------------------*/
RectDevice::RectDevice(const QString &name, const QString &value, const QString &bgcolor,
                       const QRectF &rect, QGraphicsItem *parent) : QGraphicsRectItem(rect, parent) {
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsFocusable | QGraphicsItem::ItemIsSelectable);
    this->name = name;
    this->value = value;
    this->bgcolor = bgcolor;
}

void RectDevice::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHints(QPainter::Antialiasing);
    QPen pen(QColor("#555"), 2);

    // 选中是使用虚线边框
    if (isSelected()) {
        pen.setStyle(Qt::DashLine);
    }

    // 绘制圆角矩形
    painter->setPen(pen);
    painter->setBrush(bgcolor);
    painter->drawRoundedRect(rect(), 5, 5);

    // 绘制文本
    painter->setPen(Qt::black);
    painter->drawText(rect(), Qt::AlignCenter, value);
}
