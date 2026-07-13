#include "movablelamp.h"

#include "logocanvas.h"

#include <QApplication>
#include <QFont>
#include <QLabel>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QScreen>

MovableLamp::MovableLamp(const QString &text, QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setFixedSize(30, 60);
    setMouseTracking(true);

    iconLabel = new QLabel(QString::fromUtf8("💡"), this);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setGeometry(0, 0, 30, 60);

    QFont font;
    font.setPointSize(24);
    iconLabel->setFont(font);
    iconLabel->setStyleSheet("background-color: transparent; border: none;");

    textLabel = new QLabel(text, parent);
    textLabel->setAlignment(Qt::AlignCenter);
    textLabel->setWordWrap(true);
    textLabel->setStyleSheet("background-color: transparent; border: none; color: #ffffff; font-size: 10px;");
    textLabel->setGeometry(0, 60, 150, 20);
    textLabel->show();
}

MovableLamp::~MovableLamp()
{
    if (textLabel)
    {
        textLabel->deleteLater();
    }
}

QString MovableLamp::text() const
{
    return textLabel ? textLabel->text() : QString();
}

void MovableLamp::setText(const QString &text)
{
    if (textLabel)
        textLabel->setText(text);
}

void MovableLamp::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragStart = event->position().toPoint();
    }
    QWidget::mousePressEvent(event);
}

void MovableLamp::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        const QPoint diff = event->position().toPoint() - dragStart;
        const QPoint newPos = pos() + diff;
        QWidget *p = parentWidget();
        int minX = 0;
        int minY = 0;
        int maxX = 0;
        int maxY = 0;

        if (p)
        {
            const QRect bounds = p->rect();
            minX = bounds.left();
            minY = bounds.top();
            maxX = bounds.right() - width();
            maxY = bounds.bottom() - height();
            if (maxX < minX)
                maxX = minX;
            if (maxY < minY)
                maxY = minY;
            move(qBound(minX, newPos.x(), maxX), qBound(minY, newPos.y(), maxY));
        }
    }
    QWidget::mouseMoveEvent(event);
}

void MovableLamp::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    mapPosition = event->pos();
    if (textLabel)
    {
        const int labelX = QWidget::x() + width() / 2 - textLabel->width() / 2;
        const int labelY = QWidget::y() + height();
        textLabel->move(labelX, labelY);
    }
}

void MovableLamp::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && parentWidget())
    {
        const QPoint center = mapCenterToScreen();
        if (center != QPoint(-1, -1))
        {
            screenPosition = center;
        }
    }
    QWidget::mouseReleaseEvent(event);
}

QPoint MovableLamp::mapCenterToScreen() const
{
    auto *canvas = static_cast<LogoCanvas *>(parentWidget());
    if (!canvas)
        return QPoint(-1, -1);

    const QRect rect = canvas->imageRect();
    if (rect.isNull())
        return QPoint(-1, -1);

    const QPoint center(x() + width() / 2, y() + height() / 2);
    const qreal relX = qBound(0.0, (center.x() - rect.left()) / double(rect.width()), 1.0);
    const qreal relY = qBound(0.0, (center.y() - rect.top()) / double(rect.height()), 1.0);

    QScreen *screen = canvas->selectedScreen();
    if (!screen)
        screen = QApplication::primaryScreen();
    if (!screen)
        return QPoint(-1, -1);

    const qreal dpr = screen->devicePixelRatio();
    const QSize nativeSize = screen->geometry().size() * dpr;
    return QPoint(int(relX * nativeSize.width()), int(relY * nativeSize.height()));
}
