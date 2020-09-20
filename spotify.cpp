/*
* Classe Spotify serve para realizar a conexão com a API do Spotify.
* Conexão de signal-slot com os handlers deve ser feita no widget principal
*/
#include "spotify.h"
#include <QObject>
#include <QDesktopServices>
#include <QUrlQuery>

#define AUTH_URL    "https://accounts.spotify.com/authorize"    //URL de autorização
#define TOKEN_URL   "https://accounts.spotify.com/api/token"    //URL para conseguir o token de acesso

#define SEARCH_LIMIT 20   //Limitador do numero de resultados da busca

//Construtor/setup necessário
Spotify::Spotify(QObject* main, QString clientId, QString clientSecret)
{
    auto replyHandler = new QOAuthHttpServerReplyHandler(8080, main);
    httpClient.setReplyHandler(replyHandler);
    httpClient.setClientIdentifier(clientId);
    httpClient.setClientIdentifierSharedKey(clientSecret);
    httpClient.setAccessTokenUrl(QUrl(TOKEN_URL));
    httpClient.setAuthorizationUrl(QUrl(AUTH_URL));
    httpClient.setScope("");    //Scope vazio pois não é necessário nenhuma permissão específica para as aplicações desejadas


    isGranted = false;
}

QNetworkReply* Spotify::getUserName()
{
    //Pedido de informações de usuário - ao receber resposta, deve ser passada ao devido handler (handleGetUserName())
    QUrl u ("https://api.spotify.com/v1/me");

    return httpClient.get(u);
}

bool Spotify::handleGetUserName(QNetworkReply *reply, QString &userName, QString &userId)
{
    //Tratador da resposta do pedido de informações de usuario (getUserName())
    if (reply->error() != QNetworkReply::NoError) {
        return false;
    }
    isGranted = true;
    const auto data = reply->readAll();
    //Parsear os dados que vem em formato Json
    const auto document = QJsonDocument::fromJson(data);
    const auto root = document.object();
    userName = root.value("display_name").toString();
    userId = root.value("id").toString();
    reply->deleteLater();
    return true;
}

QNetworkReply *Spotify::search(QString searchItem)
{
    //Pedido de busca a partir da pesquisa acima - ao receber resposta, deve ser passada ao devido handler (handleSearch())
    QUrlQuery queryObject;
    searchItem.replace('+', "%C3");
    queryObject.addQueryItem("q", searchItem);
    queryObject.addQueryItem("type", "track");
    queryObject.addQueryItem("limit", QString::number(SEARCH_LIMIT));
    QUrl u ("https://api.spotify.com/v1/search");
    u.setQuery(queryObject);

    return httpClient.get(u);
}

bool Spotify::handleSearch(QNetworkReply* reply)
{
    //Tratador da resposta do pedido de busca (search())
    if (reply->error() != QNetworkReply::NoError) {
        return false;
    }
    const auto data = reply->readAll();
    reply->deleteLater();
    const auto document = QJsonDocument::fromJson(data);
    auto root = document.object();
    QJsonArray items = root.value("tracks").toObject().value("items").toArray();
    searchResults.clear();
    if (items.isEmpty()){
        return false;
    }
    searchResults.reserve(items.size());
    //Parsear os dados que vem em formato Json, adequando ao formato salvo offline
    for( auto obj : items ) {
        QJsonObject _fromArray = obj.toObject();
        QJsonObject _toVector;
        _toVector["album"] = _fromArray.value("album").toObject().value("name");
        QString artists;
        for( auto artist : _fromArray.value("artists").toArray()) { //Necessario pois uma música pode ter mais de um artista
            artists += artist.toObject().value("name").toString();
            artists += ", ";
        }
        artists.remove((artists.size()-2), 2);    //remover ultimo ' ,'
        _toVector["artist"] = artists;
        _toVector["name"] = _fromArray.value("name");
        _toVector["id"] = _fromArray.value("id");
        searchResults.append(_toVector);
    }
    return true;
}

QNetworkReply *Spotify::getPreview(QString trackId)
{
    //Pedido de URL para reproduzir sample - ao receber resposta, deve ser passada ao devido handler (handleGetPreview())
    QUrl u ("https://api.spotify.com/v1/tracks/" + trackId);

    return httpClient.get(u);
}

QString Spotify::handleGetPreview(QNetworkReply *reply)
{
    //Tratador da resposta do pedido de URL do preview (getPreview())
    if (reply->error() != QNetworkReply::NoError) {
        return "ERROR";
    }
    const auto data = reply->readAll();
    reply->deleteLater();
    const auto document = QJsonDocument::fromJson(data);
    auto root = document.object();
    QString previewUrl = root.value("preview_url").toString();
    return previewUrl;
}

