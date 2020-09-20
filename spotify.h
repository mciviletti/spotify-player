/* Classe para interface com a API do Spotify */

#ifndef SPOTIFY_H
#define SPOTIFY_H

#include <QObject>
#include <QtNetworkAuth>
#include <QWidget>

class Spotify
{
public:
    Spotify( QObject* main, QString clientId, QString clientSecretKey );

    bool isGranted;

    QOAuth2AuthorizationCodeFlow httpClient;

    QNetworkReply* getUserName();
    bool handleGetUserName( QNetworkReply* reply, QString &userName, QString &userId );

    QNetworkReply* search( QString searchItem );
    bool handleSearch( QNetworkReply* reply );

    QNetworkReply* getPreview( QString trackId );
    QString handleGetPreview( QNetworkReply* reply );

    QVector<QJsonObject> searchResults;

private:

};

#endif // SPOTIFY_H
