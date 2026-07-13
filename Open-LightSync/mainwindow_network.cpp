#include "mainwindow.h"

#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

void MainWindow::sendTurnOn(const QString &entityId, const QColor &color, double transitionSeconds)
{
    if (inputUrl->text().trimmed().isEmpty() || inputToken->text().trimmed().isEmpty() || entityId.trimmed().isEmpty())
        return;

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
    if (entities.isEmpty())
        return;

    QJsonObject payload;
    QJsonArray entityArray;
    for (const QString &entity : entities)
    {
        entityArray.append(entity);
    }
    payload.insert("entity_id", entityArray);

    postJson(QUrl(inputUrl->text().trimmed() + "/api/services/light/turn_off"), payload);
}

void MainWindow::postJson(const QUrl &url, const QJsonObject &payload)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", QByteArray("Bearer ") + inputToken->text().trimmed().toUtf8());

    auto *reply = networkManager->post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}

void MainWindow::helpClick()
{
    QDesktopServices::openUrl(QUrl("https://github.com/Butter-mit-Brot/Openhome-Sync"));
}
