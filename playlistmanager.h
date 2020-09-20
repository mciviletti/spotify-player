/* Classe responsável por manejar as playlists - usa formato json para salvar, carregar e interpretar os arquivos*/

#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QFile>
#include <QJsonObject>
#include <QVector>

#define FILE_NAME "save"

class playlistManager
{
public:

    enum SaveFormat {
            Json, Binary
    } saveFormat;


    playlistManager();

    bool setUser( const QString userId );

    void createNewPlaylist( const QString name );

    void switchToPlaylist( const int index, bool newPlaylist = false );

    void removePlaylist( const int index);

    void save();

    QVector<QJsonObject> playlistsVector;

    QVector<QJsonObject> tracksVector;

private:
    QJsonObject _doc;   //Objeto com as informações do arquivo inteiro

    QJsonObject _user;  //Objeto com as informações do usuário atual

    QJsonObject _playlist;  //Objeto com as informações da playlist atual

    int _currentUserPosition;   //Indice do usuário atual no vetor "users"

    int _currentPlaylistPosition;   //Indice da playlist atual no vetor "playlists"

    bool _switchToUser( const QString name, bool create = true );

    QVector<QJsonObject> _userVector;   //Vetor de todos os usuários salvos.

    void _updateTracks();   //Atualiza o array "tracks" no objeto playlist atual

    void _updatePlaylists();    //Atualiza o array "playlists" no objeto user atual

    void _updateUsers();    //Atualiza o array "users" no documento
};

#endif // PLAYLISTMANAGER_H
