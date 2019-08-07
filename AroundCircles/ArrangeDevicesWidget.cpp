#include "ArrangeDevicesWidget.h"
#include "ui_ArrangeDevicesWidget.h"
#include "AroundDevicesGraphicsView.h"
#include "PixmapDevicesGraphicsView.h"

#include <QDebug>
#include <QLabel>
#include <QMouseEvent>
#include <QMimeData>
#include <QDrag>

// 默认定义的 16 种颜色
static QList<QString> colors = {
    "#00FFCC", "#00FFFF", "#00CCCC", "#00CCFF", "#00FF99", "#CCCC33", "#99CCFF", "#CCFF66",
    "#CCFFCC", "#FF99FF", "#FF9900", "#FF6600", "#FF9999", "#FFFF99", "#DDDDDD", "#FF9900",
};

/*-----------------------------------------------------------------------------|
 |                         ArrangeDevicesWidgetPrivate                         |
 |----------------------------------------------------------------------------*/
class ArrangeDevicesWidgetPrivate {
    friend class ArrangeDevicesWidget;

    ArrangeDevicesWidgetPrivate();

    QPoint startDragPos;
    AroundDevicesGraphicsView *circleDevicesView = nullptr;
    PixmapDevicesGraphicsView *pixmapDevicesView = nullptr;
};

ArrangeDevicesWidgetPrivate::ArrangeDevicesWidgetPrivate() {
    circleDevicesView = new AroundDevicesGraphicsView();
    pixmapDevicesView = new PixmapDevicesGraphicsView();
}

/*-----------------------------------------------------------------------------|
 |                            ArrangeDevicesWidget                             |
 |----------------------------------------------------------------------------*/
ArrangeDevicesWidget::ArrangeDevicesWidget(int type, const QStringList &deviceNames, QWidget *parent)
    : QWidget(parent), ui(new Ui::ArrangeDevicesWidget), d(new ArrangeDevicesWidgetPrivate) {
    ui->setupUi(this);

    // 根据 type 确定布点类型
    if (1 == type) {
        layout()->replaceWidget(ui->placeHolderWidget, d->circleDevicesView);
    } else {
        layout()->replaceWidget(ui->placeHolderWidget, d->pixmapDevicesView);
    }

    // 创建设备列表
    ui->devicesWidget->layout()->addItem(new QSpacerItem(20, 424, QSizePolicy::Minimum, QSizePolicy::Expanding));
    for (int i = 0; i < deviceNames.size(); ++i) {
        addDevice(deviceNames.at(i), colors[i%colors.size()]);
    }
    ui->devicesWidget->layout()->addItem(new QSpacerItem(20, 424, QSizePolicy::Minimum, QSizePolicy::Expanding));

    this->setStyleSheet(QString(".DeviceLabel { border: 1px solid gray; }"));
}

ArrangeDevicesWidget::~ArrangeDevicesWidget() {
    delete ui;
    delete d;
}

bool ArrangeDevicesWidget::eventFilter(QObject *watched, QEvent *event) {
    QLabel *label = qobject_cast<QLabel *>(watched);

    if (nullptr == label) {
        return QObject::eventFilter(watched, event);
    }

    // 设备 label 的拖拽事件
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
        d->startDragPos = me->globalPos();

        return true;
    } else if (event->type() == QEvent::MouseMove) {
        QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);
        int distance = (me->globalPos() - d->startDragPos).manhattanLength();

        if (distance >= QApplication::startDragDistance()) {
            startDrag(label);
        }

        return true;
    }

    return QObject::eventFilter(watched, event);
}

// 开始拖拽设备 label
void ArrangeDevicesWidget::startDrag(QLabel *label) {
    QString color    = label->property("color").toString();
    QString deviceId = label->property("deviceId").toString();
    QString content  = QString("%1,%2").arg(deviceId).arg(color);
    QString mimeType = "text/DnD-DEVICE-CIRCLE";

    QByteArray contentBytes = content.toUtf8();
    QMimeData *mimeData = new QMimeData();
    mimeData->setData(mimeType, contentBytes);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);

    QPixmap pixmap = label->grab();
    drag->setPixmap(pixmap);
    QSize s = pixmap.size();
    drag->setHotSpot(QPoint(s.width()/2, s.height()/2));

#ifdef Q_OS_MAC
    drag->setHotSpot(QPoint(s.width()/4, s.height()/4)); // Mac 下需要调整为四分之一处
#endif

    drag->exec(Qt::CopyAction); // Start drag
}

// 添加设备
void ArrangeDevicesWidget::addDevice(const QString name, const QString color) {
    QLabel *label = new QLabel(name);
    label->setProperty("class", "DeviceLabel");
    label->setProperty("color", color);
    label->setProperty("deviceId", name);
    label->setStyleSheet(QString("background: %1; border-radius: 3px; padding: 3px").arg(color));
    label->installEventFilter(this);

    ui->devicesWidget->layout()->addWidget(label);
}
