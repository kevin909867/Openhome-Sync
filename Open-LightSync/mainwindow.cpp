#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QFont>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScreen>
#include <QStandardPaths>
#include <QStyle>
#include <QTextStream>
#include <QResizeEvent>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QUrl>
#include <QCursor>
#include <QSettings>

#include <algorithm>
#include <random>

class MovableLamp : public QWidget
{
public:
    explicit MovableLamp(const QString &text, QWidget *parent = nullptr)
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

    ~MovableLamp() override
    {
        if (textLabel) {
            textLabel->deleteLater();
        }
    }

    QString text() const { return textLabel ? textLabel->text() : QString(); }
    void setText(const QString &text) { if (textLabel) textLabel->setText(text); }
    QPoint mapPosition{0, 0};
    QPoint screenPosition{0, 0};

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            dragStart = event->position().toPoint();
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (event->buttons() & Qt::LeftButton) {
            const QPoint diff = event->position().toPoint() - dragStart;
            const QPoint newPos = pos() + diff;
            QWidget *p = parentWidget();
            int minX = 0;
            int minY = 0;
            int maxX = 0;
            int maxY = 0;

            if (p) {
                const QRect bounds = p->rect();
                minX = bounds.left();
                minY = bounds.top();
                maxX = bounds.right() - width();
                maxY = bounds.bottom() - height();
                if (maxX < minX) maxX = minX;
                if (maxY < minY) maxY = minY;
                move(qBound(minX, newPos.x(), maxX), qBound(minY, newPos.y(), maxY));
            }
        }
        QWidget::mouseMoveEvent(event);
    }

    void moveEvent(QMoveEvent *event) override
    {
        QWidget::moveEvent(event);
        mapPosition = event->pos();
        if (textLabel) {
            const int labelX = QWidget::x() + width() / 2 - textLabel->width() / 2;
            const int labelY = QWidget::y() + height();
            textLabel->move(labelX, labelY);
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && parentWidget()) {
            const QPoint center = mapCenterToScreen();
            if (center != QPoint(-1, -1)) {
                screenPosition = center;
            }
        }
        QWidget::mouseReleaseEvent(event);
    }

private:
    QPoint mapCenterToScreen() const;
    QLabel *iconLabel = nullptr;
    QLabel *textLabel = nullptr;
    QPoint dragStart{0, 0};
};

class LogoCanvas : public QWidget
{
public:
    explicit LogoCanvas(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumSize(300, 300);
        setStyleSheet("QWidget { background-color: #000000; border: 2px solid #666; }");
        updateCapture();
    }

    void setMonitors(const QVector<QScreen *> &screens)
    {
        monitors = screens;
        if (!selectedMonitor && !monitors.isEmpty()) {
            selectedMonitor = monitors.first();
        }
        updateCapture();
        synchronizeLogos();
    }

    void setSelectedMonitor(QScreen *screen)
    {
        selectedMonitor = screen;
        updateCapture();
        synchronizeLogos();
    }

    QScreen *selectedScreen() const { return selectedMonitor; }

    QImage captureImage() const { return pixmap.toImage(); }

    const QVector<MovableLamp *> &lampWidgets() const { return logos; }

    void set_logos(const QStringList &texts)
    {
        const int n = texts.size();
        while (logos.size() > n) {
            auto *logo = logos.takeLast();
            if (logo) logo->close();
        }

        while (logos.size() < n) {
            auto *logo = new MovableLamp(QString(), this);
            placeNewLogo(logo);
            logo->show();
            logos.push_back(logo);
        }

        for (int i = 0; i < logos.size() && i < texts.size(); ++i) {
            logos[i]->setText(texts[i]);
        }
    }

    void setLampPositions(const QJsonObject &lamps)
    {
        const auto keys = lamps.keys();
        for (int i = 0; i < keys.size() && i < logos.size(); ++i) {
            const QJsonArray arr = lamps.value(keys[i]).toArray();
            if (arr.size() >= 2) {
                const int x = arr.at(0).toInt();
                const int y = arr.at(1).toInt();
                logos[i]->move(x, y);
                logos[i]->mapPosition = QPoint(x, y);
            }
        }
    }

    QJsonObject saveLampPositions(const QStringList &entityIds) const
    {
        QJsonObject lamps;
        for (int i = 0; i < entityIds.size() && i < logos.size(); ++i) {
            QJsonArray pos;
            pos.append(logos[i]->mapPosition.x());
            pos.append(logos[i]->mapPosition.y());
            lamps.insert(entityIds[i], pos);
        }
        return lamps;
    }

    void synchronizeLogos()
    {
        for (auto *logo : logos) {
            placeNewLogo(logo);
        }
    }

    QRect imageRect() const { return imageRectValue; }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        updateCapture();
        synchronizeLogos();
    }

    void paintEvent(QPaintEvent *event) override
    {
        QWidget::paintEvent(event);
        QPainter painter(this);
        if (!scaledPixmap.isNull() && !imageRectValue.isNull()) {
            painter.drawPixmap(imageRectValue.topLeft(), scaledPixmap);
        }
    }

private:
    void updateCapture()
    {
        if (selectedMonitor) {
            pixmap = selectedMonitor->grabWindow(0);
            if (pixmap.isNull()) {
                pixmap = QPixmap(640, 360);
                pixmap.fill(Qt::darkGray);
            }
        } else if (QApplication::primaryScreen()) {
            pixmap = QApplication::primaryScreen()->grabWindow(0);
        } else {
            pixmap = QPixmap(640, 360);
            pixmap.fill(Qt::darkGray);
        }

        scaledPixmap = pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const int x = (width() - scaledPixmap.width()) / 2;
        const int y = (height() - scaledPixmap.height()) / 2;
        imageRectValue = QRect(QPoint(x, y), scaledPixmap.size());
        update();
    }

    void placeNewLogo(MovableLamp *logo)
    {
        if (!logo) return;
        int x = 100;
        int y = 100;
        if (!imageRectValue.isNull()) {
            int minX = imageRectValue.left();
            int maxX = imageRectValue.right() - logo->width();
            int minY = imageRectValue.top();
            int maxY = imageRectValue.bottom() - logo->height();
            if (maxX < minX) maxX = minX;
            if (maxY < minY) maxY = minY;
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

    QVector<MovableLamp *> logos;
    QPixmap pixmap;
    QPixmap scaledPixmap;
    QRect imageRectValue;
    QVector<QScreen *> monitors;
    QScreen *selectedMonitor = nullptr;
};

class ToggleButton : public QPushButton
{
public:
    explicit ToggleButton(const QString &text = QStringLiteral("Activate"), QWidget *parent = nullptr)
        : QPushButton(text, parent)
    {
        setCheckable(true);
        setMinimumHeight(40);
        setStyleSheet(styleOff());
        connect(this, &QPushButton::clicked, this, &ToggleButton::updateStyle);
    }

    bool status = false;

private slots:
    void updateStyle()
    {
        if (isChecked()) {
            setStyleSheet(styleOn());
            status = true;
        } else {
            setStyleSheet(styleOff());
            status = false;
        }
    }

private:
    QString styleOn() const
    {
        return "QPushButton { background-color: #4CAF50; color: white; border-radius: 8px; font-size: 16px; } QPushButton:hover { border: 1px solid #eb5e28; }";
    }

    QString styleOff() const
    {
        return "QPushButton { background-color: #403d39; color: white; border-radius: 8px; font-size: 16px; } QPushButton:hover { border: 1px solid #eb5e28; }";
    }
};

static QPoint centerOnScreenFallback(const QImage &image, QScreen *screen)
{
    if (image.isNull() || !screen) return QPoint(0, 0);
    return QPoint(image.width() / 2, image.height() / 2);
}

QPoint MovableLamp::mapCenterToScreen() const
{
    auto *canvas = static_cast<LogoCanvas *>(parentWidget());
    if (!canvas) return QPoint(-1, -1);
    const QRect rect = canvas->imageRect();
    if (rect.isNull()) return QPoint(-1, -1);
    const QPoint center(x() + width() / 2, y() + height() / 2);
    const qreal relX = qBound(0.0, (center.x() - rect.left()) / double(rect.width()), 1.0);
    const qreal relY = qBound(0.0, (center.y() - rect.top()) / double(rect.height()), 1.0);
    QScreen *screen = canvas->selectedScreen();
    if (!screen) screen = QApplication::primaryScreen();
    if (!screen) return QPoint(-1, -1);
    const qreal dpr = screen->devicePixelRatio();
    const QSize nativeSize = screen->geometry().size() * dpr;
    return QPoint(int(relX * nativeSize.width()), int(relY * nativeSize.height()));
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
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

void MainWindow::setupInterface()
{
    setWindowTitle("Open-LightSync");
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
    for (auto *btn : {modeBtnScreen, modeBtnAverage, modeBtnCrazy}) {
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
    for (auto *box : {autostartBox, minimizedBox, startLampBox}) {
        box->setStyleSheet("QCheckBox::indicator { background: transparent; border: none; width: 0px; } QCheckBox { padding: 15px; font-size: 16px; border-radius: 4px; background-color: #403d39; } QCheckBox:checked { border: 1px solid #eb5e28; background-color: #00a67d; }");
        autoLayout->addWidget(box);
    }
    leftLayout->addWidget(autoGroup);

    auto *topLayout2 = new QHBoxLayout();
    saveButton = new QPushButton("SAVE");
    loadButton = new QPushButton("LOAD");
    helpButton = new QPushButton("HELP");
    for (auto *btn : {saveButton, loadButton, helpButton}) {
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
    connect(monitorCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onMonitorChanged);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveClick);
    connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadClick);
    connect(helpButton, &QPushButton::clicked, this, &MainWindow::helpClick);
}

void MainWindow::populateMonitorList()
{
    knownScreens = QApplication::screens().toVector();
    monitorCombo->clear();
    for (QScreen *screen : knownScreens) {
        monitorCombo->addItem(screen ? screen->name() : QStringLiteral("Unknown"), monitorIdentity(screen));
    }
    if (monitorCombo->count() == 0) {
        monitorCombo->addItem("Primary", "primary");
    }
    logoCanvas->setMonitors(knownScreens);
}

QString MainWindow::monitorIdentity(QScreen *screen) const
{
    if (!screen) return QStringLiteral("primary");
    return screen->name().isEmpty() ? QStringLiteral("primary") : screen->name();
}

int MainWindow::findMonitorByIdentity(const QString &identity) const
{
    for (int i = 0; i < monitorCombo->count(); ++i) {
        if (monitorCombo->itemData(i).toString() == identity) return i;
    }
    return 0;
}

QScreen *MainWindow::selectedScreen() const
{
    const int idx = monitorCombo ? monitorCombo->currentIndex() : -1;
    if (idx >= 0 && idx < knownScreens.size()) return knownScreens[idx];
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
    auto *deleteButton = new QPushButton("–");
    deleteButton->setFixedWidth(30);
    for (auto *btn : {plusButton, deleteButton}) {
        btn->setStyleSheet("QPushButton { background-color: #403d39; } QPushButton:hover { border: 0.5px solid #eb5e28; border-radius: 7px; background-color: #252422; }");
    }
    connect(plusButton, &QPushButton::clicked, this, &MainWindow::addDynamicRow);
    connect(deleteButton, &QPushButton::clicked, this, [this, rowWidget, lineEdit]() { deleteDynamicRow(rowWidget, lineEdit); });

    rowLayout->addWidget(lineEdit);
    rowLayout->addWidget(plusButton);
    rowLayout->addWidget(deleteButton);
    dynamicLayout->addWidget(rowWidget);
    dynamicRows.push_back({rowWidget, lineEdit});
    refreshLogos();
}

void MainWindow::deleteDynamicRow(QWidget *rowWidget, QLineEdit *lineEdit)
{
    if (dynamicRows.size() <= 1) {
        if (lineEdit) lineEdit->clear();
        refreshLogos();
        return;
    }
    dynamicRows.erase(std::remove_if(dynamicRows.begin(), dynamicRows.end(), [rowWidget](const DynamicRow &row) { return row.rowWidget == rowWidget; }), dynamicRows.end());
    dynamicLayout->removeWidget(rowWidget);
    rowWidget->setParent(nullptr);
    rowWidget->deleteLater();
    refreshLogos();
}

QStringList MainWindow::collectAllInputs() const
{
    QStringList values;
    for (const auto &row : dynamicRows) {
        const QString t = row.lineEdit ? row.lineEdit->text().trimmed() : QString();
        if (!t.isEmpty()) values << t;
    }
    return values;
}

QImage MainWindow::captureSelectedScreenImage() const
{
    QScreen *screen = selectedScreen();
    if (!screen) screen = QApplication::primaryScreen();
    if (!screen) return QImage();
    QPixmap pm = screen->grabWindow(0);
    if (pm.isNull()) return QImage(640, 360, QImage::Format_RGB32);
    return pm.toImage();
}

QColor MainWindow::sampleFromImage(const QImage &image, const QPoint &globalPos) const
{
    if (image.isNull()) return QColor(0, 0, 0);
    const QPoint clamped(qBound(0, globalPos.x(), image.width() - 1), qBound(0, globalPos.y(), image.height() - 1));
    return image.pixelColor(clamped);
}

QColor MainWindow::sampleAverageColor(const QImage &image, QScreen *screen) const
{
    Q_UNUSED(screen)
    if (image.isNull()) return QColor(0, 0, 0);
    const QVector<QPoint> points = {
        QPoint(image.width() / 4, image.height() / 4),
        QPoint((3 * image.width()) / 4, image.height() / 4),
        QPoint(image.width() / 4, (3 * image.height()) / 4),
        QPoint((3 * image.width()) / 4, (3 * image.height()) / 4),
    };
    int r = 0, g = 0, b = 0;
    for (const QPoint &p : points) {
        const QColor c = sampleFromImage(image, p);
        r += c.red(); g += c.green(); b += c.blue();
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
    if (!screen) return globalPos;
    QRect g = screen->geometry();
    return QPoint(qBound(g.left(), globalPos.x(), g.right()), qBound(g.top(), globalPos.y(), g.bottom()));
}

void MainWindow::runScreenMode(const QStringList &entities, const QImage &image, QScreen *screen)
{
    if (!logoCanvas || !screen) return;
    const QRect rect = logoCanvas->imageRect();
    if (rect.isNull()) return;
    const auto lamps = logoCanvas->lampWidgets();
    for (int i = 0; i < entities.size() && i < lamps.size(); ++i) {
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
    Q_UNUSED(screen)
    const QColor color = sampleAverageColor(image, screen);
    for (const QString &entity : entities) sendTurnOn(entity, color, 1.0);
}

void MainWindow::runCrazyMode(const QStringList &entities, const QImage &image, QScreen *screen)
{
    Q_UNUSED(screen)
    const QColor color = sampleCrazyColor(image, screen);
    for (const QString &entity : entities) sendTurnOn(entity, color, 0.2);
}

void MainWindow::sendTurnOn(const QString &entityId, const QColor &color, double transitionSeconds)
{
    if (inputUrl->text().trimmed().isEmpty() || inputToken->text().trimmed().isEmpty() || entityId.trimmed().isEmpty()) return;
    const QColor previousColor = lastSentColors.value(entityId);
    if (lastSentColors.contains(entityId) && previousColor == color) {
        return;
    }

    lastSentColors.insert(entityId, color);

    QJsonObject payload;
    payload.insert("entity_id", entityId);
    payload.insert("rgb_color", QJsonArray{color.red(), color.green(), color.blue()});
    payload.insert("brightness", 255);
    payload.insert("transition", transitionSeconds);
    postJson(QUrl(inputUrl->text().trimmed() + "/api/services/light/turn_on"), payload);
}

void MainWindow::turnOff()
{
    const QStringList entities = collectAllInputs();
    if (entities.isEmpty()) return;
    QJsonObject payload;
    QJsonArray entityArray;
    for (const QString &entity : entities) {
        entityArray.append(entity);
    }
    payload.insert("entity_id", entityArray);
    postJson(QUrl(inputUrl->text().trimmed() + "/api/services/light/turn_off"), payload);
    lastSentColors.clear();
}

void MainWindow::postJson(const QUrl &url, const QJsonObject &payload)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", QByteArray("Bearer ") + inputToken->text().trimmed().toUtf8());
    auto *reply = networkManager->post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}

void MainWindow::updateLight()
{
    const QStringList entities = collectAllInputs();
    const QImage image = captureSelectedScreenImage();
    QScreen *screen = selectedScreen();
    if (!screen) screen = QApplication::primaryScreen();

    if (toggleButton->status) {
        lampStatus = true;
        if (modeBtnScreen->isChecked()) runScreenMode(entities, image, screen);
        else if (modeBtnAverage->isChecked()) runAverageMode(entities, image, screen);
        else runCrazyMode(entities, image, screen);
    } else if (lampStatus) {
        turnOff();
        lampStatus = false;
    }

    if (autostartBox->isChecked()) {
        if (!isInAutostart()) addSelfToAutostart();
    } else if (isInAutostart()) {
        removeSelfFromAutostart();
    }
}

void MainWindow::saveClick()
{
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    QJsonDocument doc(buildSaveObject());
    file.write(doc.toJson(QJsonDocument::Indented));
    QMessageBox::information(this, "Saved Successfully!", "Credentials and Lamps Saved Successfully!");
}

void MainWindow::loadClick()
{
    QFile file(savePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    applyLoadedData(doc.object());
}

void MainWindow::helpClick()
{
    QDesktopServices::openUrl(QUrl("https://github.com/Butter-mit-Brot/Openhome-Sync"));
}

void MainWindow::refreshLogos()
{
    if (!logoCanvas) return;
    logoCanvas->set_logos(collectAllInputs());
}

QJsonObject MainWindow::buildSaveObject() const
{
    QJsonObject obj;
    QJsonArray credentials;
    credentials.append(inputUrl->text());
    credentials.append(inputToken->text());
    credentials.append(autostartBox->isChecked());
    credentials.append(minimizedBox->isChecked());
    credentials.append(startLampBox->isChecked());
    obj.insert("credentials", credentials);
    obj.insert("selected_monitor", monitorCombo->currentData().toString());
    obj.insert("lamps", logoCanvas ? logoCanvas->saveLampPositions(collectAllInputs()) : QJsonObject());
    return obj;
}

void MainWindow::applyLoadedData(const QJsonObject &obj)
{
    const QJsonArray credentials = obj.value("credentials").toArray();
    if (credentials.size() >= 5) {
        inputUrl->setText(credentials.at(0).toString());
        inputToken->setText(credentials.at(1).toString());
        autostartBox->setChecked(credentials.at(2).toBool());
        minimizedBox->setChecked(credentials.at(3).toBool());
        startLampBox->setChecked(credentials.at(4).toBool());
        if (minimizedBox->isChecked()) showMinimized();
        if (startLampBox->isChecked()) {
            toggleButton->setChecked(true);
            toggleButton->click();
        }
    }
    const QString selectedId = obj.value("selected_monitor").toString();
    const int idx = findMonitorByIdentity(selectedId);
    if (idx >= 0) monitorCombo->setCurrentIndex(idx);

    const QJsonObject lamps = obj.value("lamps").toObject();
    for (auto &row : dynamicRows) {
        row.rowWidget->setParent(nullptr);
        row.rowWidget->deleteLater();
    }
    dynamicRows.clear();
    const auto keys = lamps.keys();
    for (const QString &key : keys) {
        addDynamicRowWithText(key);
    }
    if (keys.isEmpty()) addDynamicRowWithText(QString());
    logoCanvas->setLampPositions(lamps);
    refreshLogos();
}

void MainWindow::loadOnStartup()
{
    QFile file(savePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return;
    applyLoadedData(doc.object());
}

bool MainWindow::isInAutostart() const
{
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    return settings.contains("Openhome Sync");
#else
    return QFile::exists(autostartDesktopPath());
#endif
}

QString MainWindow::autostartDesktopPath() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QDir().mkpath(dir);
    return dir + "/open-lightsync.desktop";
}

void MainWindow::addSelfToAutostart()
{
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    settings.setValue("Openhome Sync", '"' + exe + '"');
#else
    QFile file(autostartDesktopPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return;
    QTextStream out(&file);
    out << "[Desktop Entry]\nType=Application\nName=Openhome Sync\nExec=\"" << QCoreApplication::applicationFilePath() << "\"\nTerminal=false\nX-GNOME-Autostart-enabled=true\n";
#endif
}

void MainWindow::removeSelfFromAutostart()
{
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    settings.remove("Openhome Sync");
#else
    QFile::remove(autostartDesktopPath());
#endif
}

