/* Essa classe maneja as playlists utilizando formato Json. Todas as informações são salvas em apenas um arquivo.
 * O arquivo contém um vetor "users", onde cada entrada é um objeto Json com um valor "id"(String) e um vetor "playlists", representando um usuário.
 * Cada entrada do vetor "playlists" é um objeto Json com um valor "name" (String) e um vetor "tracks", representando uma playlist.
 * Cada entrada do vetor "tracks" é um objeto Json com um valor "name"(String), "artist"(String), "Album"(String), "preview_url"(String) e
 * "id"(String), representando uma música.
 *
 *
 * A biblioteca de Json trata os vetores de objeto como QJsonArrays. Para ser possível alterar os valores dentro das entradas do vetor, é ncessário
 * converter o objeto para um QVector de QJsonObject e então alterar o valor do QJsonObject desejado. As informações são guardadas, então, em QVectors
 * para que possam ser atualizadas futuramente.
 */

#include "playlistmanager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QCborValue>
#include <QCborMap>

playlistManager::playlistManager()
{
    //Ao ser iniciado, abre o arquivo onde as informações estão salvas
    saveFormat = Binary; //Selecionar Json ou Binary (CBOR) - CBOR ocupa menos espaço (menos da metade) mas o arquivo não é legível para pessoas
    QFile _file(saveFormat == Json ? (QString(FILE_NAME) + ".json") : (QString(FILE_NAME) + ".dat"));
    if (!_file.open(QIODevice::ReadOnly)) {
            qWarning("Couldn't open user files.");
    }
    QByteArray data = _file.readAll();
    QJsonDocument loadDoc(saveFormat == Json
                          ? QJsonDocument::fromJson(data)
                          : QJsonDocument(QCborValue::fromCbor(data).toMap().toJsonObject()));
    _doc = loadDoc.object();
    _file.close();
}

bool playlistManager::setUser(QString userId)
{
    //Função para selecionar um usuário
    QJsonArray usersArray = _doc["users"].toArray();
    _userVector.clear();
    _userVector.reserve(usersArray.size());
    for (int index = 0; index < usersArray.size(); ++index) {
        QJsonObject userObject = usersArray[index].toObject();
        _userVector.append(userObject);
    }
    _switchToUser(userId);
    _updateUsers();

    return true;
}

void playlistManager::createNewPlaylist(const QString name)
{
    //Função para criar nova playlist
    QJsonObject newPlaylist;
    newPlaylist["name"] = name;
    playlistsVector.append(newPlaylist);
    switchToPlaylist((playlistsVector.size()-1), true);
    save();
}

void playlistManager::switchToPlaylist(const int index, bool newPlaylist)
{
    //Função para trocar para a playlist indicada pelo indice, valor newPlaylist serve para indicar se a playlist acaba de ser criada (por padrão, false)
    _currentPlaylistPosition = index;
    tracksVector.clear();
    if(newPlaylist)
        return;
    QJsonArray tracksArray = playlistsVector[index]["tracks"].toArray();
    tracksVector.reserve(tracksArray.size());
    for( auto obj : tracksArray) {
        tracksVector.append(obj.toObject());
    }
}

void playlistManager::removePlaylist(const int index)
{
    //Remove a playlist indicada pelo indice
    playlistsVector.remove(index);
    tracksVector.clear();
    _currentPlaylistPosition = -1;
    save();
}

void playlistManager::save()
{
    //Atualiza os objetos necessários e salva no arquivo
    _updateTracks();
    _updatePlaylists();
    _updateUsers();
}

bool playlistManager::_switchToUser(const QString name, bool create)
{
    //Trocar de usuário, com possibilidade de criar novo usuário caso não exista. Por padrão, create = true
    int i;
    for (i = 0; i < _userVector.size(); i++) {
        if(_userVector[i].value("id").toString()== name){
            _currentUserPosition = i;
            QJsonArray playlistsArray = _userVector[i]["playlists"].toArray();
            playlistsVector.clear();
            playlistsVector.reserve(playlistsArray.size());
            for(int index = 0; index < playlistsArray.size(); ++index) {
                playlistsVector.append(playlistsArray[index].toObject());
            }
            _currentPlaylistPosition = -1;
            return true;
        }
    }
    if(create){
        QJsonObject newObject;
        newObject["id"] = name;
        _userVector.append(newObject);
        _currentUserPosition = _userVector.size() - 1;
        playlistsVector.clear();
        _currentPlaylistPosition = -1;
    }
    return false;
}

void playlistManager::_updateTracks()
{
    //Atualiza as músicas da playlist atual
    QJsonArray tracks;
    for(auto obj : tracksVector) {
        tracks.append(obj);
    }
    if(_currentPlaylistPosition != -1) {
        playlistsVector[_currentPlaylistPosition]["tracks"] = tracks;
    }
}

void playlistManager::_updatePlaylists()
{
    //Atualiza as playlists do usuário atual
    QJsonArray playlists;
    for(auto obj : playlistsVector) {
        playlists.append(obj);
    }
    if(_currentUserPosition != -1) {
        _userVector[_currentUserPosition]["playlists"] = playlists;
    }
}

void playlistManager::_updateUsers()
{
    //Atualiza os usuários do documento e salva.
    QFile _file(saveFormat == Json ? (QString(FILE_NAME) + ".json") : (QString(FILE_NAME) + ".dat"));
    if (!_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning("Couldn't open user files.");
    }
    QJsonArray users;
    for(auto obj : _userVector){
        users.append(obj);
    }
    _doc["users"] = users;
    _file.write(saveFormat == Json
                ? QJsonDocument(_doc).toJson()
                : QCborValue::fromJsonValue(_doc).toCbor());
    _file.close();
}
