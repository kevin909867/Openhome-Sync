#include "logocanvas.h"

#include "movablelamp.h"

#include <QApplication>
#include <QJsonArray>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScreen>

#include <random>

LogoCanvas::LogoCanvas(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(300, 300);
    setStyleSheet("QWidget { background-color: #000000; border: 2px solid #666; }");
    updateCapture();
}

void LogoCanvas::setMonitors(const QVector<QScreen *> &screens)
{
    monitors = screens;
    if (!selectedMonitor && !monitors.isEmpty())
    {
        selectedMonitor = monitors.first();
    }
    updateCapture();
    synchronizeLogos();
}

void LogoCanvas::setSelectedMonitor(QScreen *screen)
{
    selectedMonitor = screen;
    updateCapture();
    synchronizeLogos();
}

QScreen *LogoCanvas::selectedScreen() const
{
    return selectedMonitor;
}

QImage LogoCanvas::captureImage() const
{
    return pixmap.toImage();
}

const QVector<MovableLamp *> &LogoCanvas::lampWidgets() const
{
    return logos;
}

void LogoCanvas::set_logos(const QStringList &texts)
{
    const int n = texts.size();
    while (logos.size() > n)
    {
        auto *logo = logos.takeLast();
        if (logo)
            logo->close();
    }

    while (logos.size() < n)
    {
        auto *logo = new MovableLamp(QString(), this);
        placeNewLogo(logo);
        logo->show();
        logos.push_back(logo);
    }

    for (int i = 0; i < logos.size() && i < texts.size(); ++i)
    {
        logos[i]->setText(texts[i]);
    }
}

void LogoCanvas::setLampPositions(const QJsonObject &lamps)
{
    const auto keys = lamps.keys();
    for (int i = 0; i < keys.size() && i < logos.size(); ++i)
    {
        const QJsonArray arr = lamps.value(keys[i]).toArray();
        if (arr.size() >= 2)
        {
            const int x = arr.at(0).toInt();
            const int y = arr.at(1).toInt();
            logos[i]->move(x, y);
            logos[i]->mapPosition = QPoint(x, y);
        }
    }
}

QJsonObject LogoCanvas::saveLampPositions(const QStringList &entityIds) const
{
    QJsonObject lamps;
    for (int i = 0; i < entityIds.size() && i < logos.size(); ++i)
    {
        QJsonArray pos;
        pos.append(logos[i]->mapPosition.x());
        pos.append(logos[i]->mapPosition.y());
        lamps.insert(entityIds[i], pos);
    }
    return lamps;
}

void LogoCanvas::synchronizeLogos()
{
    for (auto *logo : logos)
    {
        placeNewLogo(logo);
    }
}

QRect LogoCanvas::imageRect() const
{
    return imageRectValue;
}

void LogoCanvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateCapture();
    synchronizeLogos();
}

void LogoCanvas::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    if (!scaledPixmap.isNull() && !imageRectValue.isNull())
    {
        painter.drawPixmap(imageRectValue.topLeft(), scaledPixmap);
    }
}

void LogoCanvas::updateCapture()
{
    if (selectedMonitor)
    {
        pixmap = selectedMonitor->grabWindow(0);
        if (pixmap.isNull())
        {
            pixmap = QPixmap(640, 360);
            pixmap.fill(Qt::darkGray);
        }
    }
    else if (QApplication::primaryScreen())
    {
        pixmap = QApplication::primaryScreen()->grabWindow(0);
    }
    else
    {
        pixmap = QPixmap(640, 360);
        pixmap.fill(Qt::darkGray);
    }

    scaledPixmap = pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const int x = (width() - scaledPixmap.width()) / 2;
    const int y = (height() - scaledPixmap.height()) / 2;
    imageRectValue = QRect(QPoint(x, y), scaledPixmap.size());
    update();
}

void LogoCanvas::placeNewLogo(MovableLamp *logo)
{
    if (!logo)
        return;

    int x = 100;
    int y = 100;
    if (!imageRectValue.isNull())
    {
        int minX = imageRectValue.left();
        int maxX = imageRectValue.right() - logo->width();
        int minY = imageRectValue.top();
        int maxY = imageRectValue.bottom() - logo->height();
        if (maxX < minX)
            maxX = minX;
        if (maxY < minY)
            maxY = minY;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dx(minX, maxX);
        std::uniform_int_distribution<> dy(minY, maxY);
        x = dx(gen);
        y = dy(gen);
    }
    logo->move(x, y);
    logo->mapPosition = QPoint(x, y);
}
