#ifndef TOGGLEBUTTON_H
#define TOGGLEBUTTON_H

#include <QPushButton>
#include <QString>

class ToggleButton : public QPushButton
{
public:
    explicit ToggleButton(const QString &text = QStringLiteral("Activate"), QWidget *parent = nullptr);

    bool status = false;

private:
    void updateStyle();
    QString styleOn() const;
    QString styleOff() const;
};

#endif // TOGGLEBUTTON_H
