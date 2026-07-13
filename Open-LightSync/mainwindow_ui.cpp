#include "mainwindow.h"

#include "widgets/logocanvas.h"
#include "widgets/togglebutton.h"

#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

#include <algorithm>

void MainWindow::setupInterface()
{
    setWindowTitle("Open LightSync");
    setWindowIcon(QIcon("icon.ico"));
    setMinimumSize(900, 450);
    setStyleSheet("background-color: #131515;");

    central = new QWidget(this);
    setCentralWidget(central);
    mainLayout = new QHBoxLayout(central);

    leftContainer = new QWidget(central);
    leftLayout = new QVBoxLayout(leftContainer);

    rightContainer = new QWidget(central);
    rightLayout = new QVBoxLayout(rightContainer);

    auto *topLayout = new QHBoxLayout();
    inputUrl = new QLineEdit();
    inputUrl->setPlaceholderText("https://your.ip.or.domain:8123");
    inputToken = new QLineEdit();
    inputToken->setPlaceholderText("Your token");
    topLayout->addWidget(inputUrl);
    topLayout->addWidget(inputToken);
    leftLayout->addLayout(topLayout);

    auto *monitorRow = new QHBoxLayout();
    auto *monitorLabel = new QLabel("Monitor:");
    monitorLabel->setStyleSheet("color: white;");
    monitorCombo = new QComboBox();
    monitorRow->addWidget(monitorLabel);
    monitorRow->addWidget(monitorCombo, 1);
    leftLayout->addLayout(monitorRow);

    auto *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    leftLayout->addWidget(separator);

    dynamicLayout = new QVBoxLayout();
    leftLayout->addLayout(dynamicLayout);
    addDynamicRowWithText(QString());
    leftLayout->addStretch();

    auto *selectGroup = new QGroupBox("Mode Selector");
    auto *selectLayout = new QHBoxLayout(selectGroup);
    modeBtnScreen = new QPushButton("Screen Mode");
    modeBtnAverage = new QPushButton("Average Mode");
    modeBtnCrazy = new QPushButton("Crazy Mode");
    modeGroup = new QButtonGroup(this);

    for (auto *btn : {modeBtnScreen, modeBtnAverage, modeBtnCrazy})
    {
        btn->setCheckable(true);
        btn->setMinimumHeight(60);
        btn->setMinimumWidth(120);
        selectLayout->addWidget(btn);
        btn->setStyleSheet("QPushButton { padding: 15px; font-size: 16px; border-radius: 4px; background-color: #403d39; } QPushButton:checked { border: 1px solid #eb5e28; background-color: #252422; }");
    }

    modeGroup->addButton(modeBtnScreen, 1);
    modeGroup->addButton(modeBtnAverage, 2);
    modeGroup->addButton(modeBtnCrazy, 3);
    modeBtnScreen->setChecked(true);
    leftLayout->addWidget(selectGroup);

    auto *autoGroup = new QGroupBox("Autostart Selector");
    auto *autoLayout = new QHBoxLayout(autoGroup);
    autostartBox = new QCheckBox("Autostart Application");
    minimizedBox = new QCheckBox("Autostart minimized");
    startLampBox = new QCheckBox("Autostart Lamps");

    for (auto *box : {autostartBox, minimizedBox, startLampBox})
    {
        box->setStyleSheet("QCheckBox::indicator { background: transparent; border: none; width: 0px; } QCheckBox { padding: 15px; font-size: 16px; border-radius: 4px; background-color: #403d39; } QCheckBox:checked { border: 1px solid #eb5e28; background-color: #00a67d; }");
        autoLayout->addWidget(box);
    }
    leftLayout->addWidget(autoGroup);

    auto *topLayout2 = new QHBoxLayout();
    saveButton = new QPushButton("SAVE");
    loadButton = new QPushButton("LOAD");
    helpButton = new QPushButton("HELP");
    for (auto *btn : {saveButton, loadButton, helpButton})
    {
        btn->setStyleSheet("QPushButton { padding: 15px; font-size: 16px; border-radius: 4px; background-color: #403d39; } QPushButton:hover { border: 1px solid #eb5e28; background-color: #252422; }");
        topLayout2->addWidget(btn);
    }
    rightLayout->addLayout(topLayout2);

    logoCanvas = new LogoCanvas();
    rightLayout->addWidget(logoCanvas);
    toggleButton = new ToggleButton("START");
    rightLayout->addWidget(toggleButton);

    mainLayout->addWidget(leftContainer, 1);
    mainLayout->addWidget(rightContainer, 1);
}

void MainWindow::setupStyles()
{
    const QString inputStyle = "QLineEdit { background-color: #403d39; border-radius: 3px; color: white; padding: 4px; } QLineEdit:focus { border: 1px solid #eb5e28; border-radius: 3px; }";
    inputUrl->setStyleSheet(inputStyle);
    inputToken->setStyleSheet(inputStyle);
}

void MainWindow::setupConnections()
{
    connect(inputUrl, &QLineEdit::textChanged, this, &MainWindow::refreshLogos);
    connect(inputToken, &QLineEdit::textChanged, this, &MainWindow::refreshLogos);
    connect(monitorCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::onMonitorChanged);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveClick);
    connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadClick);
    connect(helpButton, &QPushButton::clicked, this, &MainWindow::helpClick);
}

void MainWindow::populateMonitorList()
{
    knownScreens = QApplication::screens().toVector();
    monitorCombo->clear();
    for (QScreen *screen : knownScreens)
    {
        monitorCombo->addItem(screen ? screen->name() : QStringLiteral("Unknown"), monitorIdentity(screen));
    }
    if (monitorCombo->count() == 0)
    {
        monitorCombo->addItem("Primary", "primary");
    }
    logoCanvas->setMonitors(knownScreens);
}

QString MainWindow::monitorIdentity(QScreen *screen) const
{
    if (!screen)
        return QStringLiteral("primary");
    return screen->name().isEmpty() ? QStringLiteral("primary") : screen->name();
}

int MainWindow::findMonitorByIdentity(const QString &identity) const
{
    for (int i = 0; i < monitorCombo->count(); ++i)
    {
        if (monitorCombo->itemData(i).toString() == identity)
            return i;
    }
    return 0;
}

QScreen *MainWindow::selectedScreen() const
{
    const int idx = monitorCombo ? monitorCombo->currentIndex() : -1;
    if (idx >= 0 && idx < knownScreens.size())
        return knownScreens[idx];
    return QApplication::primaryScreen();
}

void MainWindow::onMonitorChanged(int)
{
    logoCanvas->setSelectedMonitor(selectedScreen());
    refreshLogos();
}

void MainWindow::addDynamicRow()
{
    addDynamicRowWithText(QString());
}

void MainWindow::addDynamicRowWithText(const QString &text)
{
    auto *rowWidget = new QWidget();
    auto *rowLayout = new QHBoxLayout(rowWidget);

    auto *lineEdit = new QLineEdit();
    lineEdit->setPlaceholderText("Your device ID...");
    lineEdit->setText(text);
    lineEdit->setStyleSheet(inputUrl->styleSheet());
    connect(lineEdit, &QLineEdit::textChanged, this, &MainWindow::refreshLogos);

    auto *plusButton = new QPushButton("+");
    plusButton->setFixedWidth(30);
    auto *deleteButton = new QPushButton("-");
    deleteButton->setFixedWidth(30);
    for (auto *btn : {plusButton, deleteButton})
    {
        btn->setStyleSheet("QPushButton { background-color: #403d39; } QPushButton:hover { border: 0.5px solid #eb5e28; border-radius: 7px; background-color: #252422; }");
    }

    connect(plusButton, &QPushButton::clicked, this, &MainWindow::addDynamicRow);
    connect(deleteButton, &QPushButton::clicked, this, [this, rowWidget, lineEdit]() {
        deleteDynamicRow(rowWidget, lineEdit);
    });

    rowLayout->addWidget(lineEdit);
    rowLayout->addWidget(plusButton);
    rowLayout->addWidget(deleteButton);
    dynamicLayout->addWidget(rowWidget);
    dynamicRows.push_back({rowWidget, lineEdit});

    refreshLogos();
}

void MainWindow::deleteDynamicRow(QWidget *rowWidget, QLineEdit *lineEdit)
{
    if (dynamicRows.size() <= 1)
    {
        if (lineEdit)
            lineEdit->clear();
        refreshLogos();
        return;
    }

    dynamicRows.erase(std::remove_if(dynamicRows.begin(), dynamicRows.end(), [rowWidget](const DynamicRow &row) {
        return row.rowWidget == rowWidget;
    }), dynamicRows.end());

    dynamicLayout->removeWidget(rowWidget);
    rowWidget->setParent(nullptr);
    rowWidget->deleteLater();
    refreshLogos();
}

QStringList MainWindow::collectAllInputs() const
{
    QStringList values;
    for (const auto &row : dynamicRows)
    {
        const QString text = row.lineEdit ? row.lineEdit->text().trimmed() : QString();
        if (!text.isEmpty())
            values << text;
    }
    return values;
}

void MainWindow::refreshLogos()
{
    if (!logoCanvas)
        return;
    logoCanvas->set_logos(collectAllInputs());
}
