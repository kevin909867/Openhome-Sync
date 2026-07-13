#include "togglebutton.h"

ToggleButton::ToggleButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    setCheckable(true);
    setMinimumHeight(40);
    setStyleSheet(styleOff());
    connect(this, &QPushButton::clicked, this, &ToggleButton::updateStyle);
}

void ToggleButton::updateStyle()
{
    if (isChecked())
    {
        setStyleSheet(styleOn());
        status = true;
    }
    else
    {
        setStyleSheet(styleOff());
        status = false;
    }
}

QString ToggleButton::styleOn() const
{
    return "QPushButton { background-color: #4CAF50; color: white; border-radius: 8px; font-size: 16px; } QPushButton:hover { border: 1px solid #eb5e28; }";
}

QString ToggleButton::styleOff() const
{
    return "QPushButton { background-color: #403d39; color: white; border-radius: 8px; font-size: 16px; } QPushButton:hover { border: 1px solid #eb5e28; }";
}
