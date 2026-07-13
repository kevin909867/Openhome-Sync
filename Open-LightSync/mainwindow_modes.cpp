#include "mainwindow.h"

#include "widgets/logocanvas.h"
#include "widgets/movablelamp.h"

#include <QApplication>
#include <QCursor>
#include <QScreen>

QImage MainWindow::captureSelectedScreenImage() const
{
    QScreen *screen = selectedScreen();
    if (!screen)
        screen = QApplication::primaryScreen();
    if (!screen)
        return QImage();

    QPixmap pm = screen->grabWindow(0);
    if (pm.isNull())
        return QImage(640, 360, QImage::Format_RGB32);
    return pm.toImage();
}

QColor MainWindow::sampleFromImage(const QImage &image, const QPoint &globalPos) const
{
    if (image.isNull())
        return QColor(0, 0, 0);

    const QPoint clamped(qBound(0, globalPos.x(), image.width() - 1),
                         qBound(0, globalPos.y(), image.height() - 1));
    return image.pixelColor(clamped);
}

QColor MainWindow::sampleAverageColor(const QImage &image, QScreen *screen) const
{
    Q_UNUSED(screen)
    if (image.isNull())
        return QColor(0, 0, 0);

    const QVector<QPoint> points = {
        QPoint(image.width() / 4, image.height() / 4),
        QPoint((3 * image.width()) / 4, image.height() / 4),
        QPoint(image.width() / 4, (3 * image.height()) / 4),
        QPoint((3 * image.width()) / 4, (3 * image.height()) / 4),
    };

    int r = 0;
    int g = 0;
    int b = 0;
    for (const QPoint &p : points)
    {
        const QColor c = sampleFromImage(image, p);
        r += c.red();
        g += c.green();
        b += c.blue();
    }

    return QColor(r / points.size(), g / points.size(), b / points.size());
}

QColor MainWindow::sampleCrazyColor(const QImage &image, QScreen *screen) const
{
    Q_UNUSED(screen)
    return sampleFromImage(image, QCursor::pos());
}

QPoint MainWindow::clampToScreen(const QPoint &globalPos, QScreen *screen) const
{
    if (!screen)
        return globalPos;
    QRect g = screen->geometry();
    return QPoint(qBound(g.left(), globalPos.x(), g.right()), qBound(g.top(), globalPos.y(), g.bottom()));
}

void MainWindow::runScreenMode(const QStringList &entities, const QImage &image, QScreen *screen)
{
    if (!logoCanvas || !screen)
        return;

    const QRect rect = logoCanvas->imageRect();
    if (rect.isNull())
        return;

    const auto lamps = logoCanvas->lampWidgets();
    for (int i = 0; i < entities.size() && i < lamps.size(); ++i)
    {
        MovableLamp *lamp = lamps[i];
        const QPoint center(lamp->x() + lamp->width() / 2, lamp->y() + lamp->height() / 2);
        const qreal relX = qBound(0.0, (center.x() - rect.left()) / double(rect.width()), 1.0);
        const qreal relY = qBound(0.0, (center.y() - rect.top()) / double(rect.height()), 1.0);
        const int px = qBound(0, int(relX * image.width()), image.width() - 1);
        const int py = qBound(0, int(relY * image.height()), image.height() - 1);
        const QColor color = image.pixelColor(px, py);
        sendTurnOn(entities[i], color, 0.5);
    }
}

void MainWindow::runAverageMode(const QStringList &entities, const QImage &image, QScreen *screen)
{
    const QColor color = sampleAverageColor(image, screen);
    for (const QString &entity : entities)
        sendTurnOn(entity, color, 1.0);
}

void MainWindow::runCrazyMode(const QStringList &entities, const QImage &image, QScreen *screen)
{
    const QColor color = sampleCrazyColor(image, screen);
    for (const QString &entity : entities)
        sendTurnOn(entity, color, 0.2);
}
