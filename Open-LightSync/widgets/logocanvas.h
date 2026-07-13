#ifndef LOGOCANVAS_H
#define LOGOCANVAS_H

#include <QImage>
#include <QJsonObject>
#include <QPixmap>
#include <QRect>
#include <QStringList>
#include <QVector>
#include <QWidget>

class MovableLamp;
class QPaintEvent;
class QResizeEvent;
class QScreen;

class LogoCanvas : public QWidget
{
public:
    explicit LogoCanvas(QWidget *parent = nullptr);

    void setMonitors(const QVector<QScreen *> &screens);
    void setSelectedMonitor(QScreen *screen);

    QScreen *selectedScreen() const;
    QImage captureImage() const;
    const QVector<MovableLamp *> &lampWidgets() const;

    void set_logos(const QStringList &texts);
    void setLampPositions(const QJsonObject &lamps);
    QJsonObject saveLampPositions(const QStringList &entityIds) const;

    void synchronizeLogos();
    QRect imageRect() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void updateCapture();
    void placeNewLogo(MovableLamp *logo);

    QVector<MovableLamp *> logos;
    QPixmap pixmap;
    QPixmap scaledPixmap;
    QRect imageRectValue;
    QVector<QScreen *> monitors;
    QScreen *selectedMonitor = nullptr;
};

#endif // LOGOCANVAS_H
