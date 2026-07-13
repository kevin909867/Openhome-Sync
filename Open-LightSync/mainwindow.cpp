#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "widgets/togglebutton.h"

#include <QApplication>
#include <QDir>
#include <QNetworkAccessManager>
#include <QScreen>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupInterface();
    setupStyles();
    setupConnections();

    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(baseDir);
    savePath = baseDir + "/save.dat";
    networkManager = new QNetworkAccessManager(this);

    populateMonitorList();
    loadOnStartup();

    timer.setInterval(100);
    connect(&timer, &QTimer::timeout, this, &MainWindow::updateLight);
    timer.start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateLight()
{
    const QStringList entities = collectAllInputs();
    const QImage image = captureSelectedScreenImage();
    QScreen *screen = selectedScreen();
    if (!screen)
        screen = QApplication::primaryScreen();

    if (toggleButton->status)
    {
        lampStatus = true;
        if (modeBtnScreen->isChecked())
            runScreenMode(entities, image, screen);
        else if (modeBtnAverage->isChecked())
            runAverageMode(entities, image, screen);
        else
            runCrazyMode(entities, image, screen);
    }
    else if (lampStatus)
    {
        turnOff();
        lampStatus = false;
    }

    if (autostartBox->isChecked())
    {
        if (!isInAutostart())
            addSelfToAutostart();
    }
    else if (isInAutostart())
    {
        removeSelfFromAutostart();
    }
}
