#ifndef MOVABLELAMP_H
#define MOVABLELAMP_H

#include <QPoint>
#include <QString>
#include <QWidget>

class QLabel;
class QMouseEvent;
class QMoveEvent;

class MovableLamp : public QWidget
{
public:
    explicit MovableLamp(const QString &text, QWidget *parent = nullptr);
    ~MovableLamp() override;

    QString text() const;
    void setText(const QString &text);

    QPoint mapPosition{0, 0};
    QPoint screenPosition{0, 0};

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPoint mapCenterToScreen() const;

    QLabel *iconLabel = nullptr;
    QLabel *textLabel = nullptr;
    QPoint dragStart{0, 0};
};

#endif // MOVABLELAMP_H
