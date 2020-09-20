//-------------------------------------Widget principal do programa --------------------------

#ifndef PLAYER_H
#define PLAYER_H

#include <QWidget>
#include <QtWidgets>
#include "spotify.h"
#include "playlistmanager.h"
#include <QListWidget>
#include <QKeyEvent>
#include <QtMultimedia>

QT_BEGIN_NAMESPACE
namespace Ui { class player; }
QT_END_NAMESPACE


enum PlayerStatus : byte {
    playing,
    paused,
    stopped
};

class player : public QWidget
{
    Q_OBJECT

public:
    player(QWidget *parent = nullptr);
    ~player();

private slots:
    void granted();

    void on_Login_clicked();

    void on_playlistWidget_itemActivated( QListWidgetItem *item );

    void keyPressEvent( QKeyEvent *event );

    void on_searchButton_clicked();

    void on_addButton_clicked();

    void on_createButton_clicked();

    void on_selectPlaylist_currentIndexChanged( int index );

    void on_saveButton_clicked();

    void on_removeButton_clicked();

    void on_volumeSlider_valueChanged( int value );

    void on_playButton_clicked();

    void on_deleteButton_clicked();

    void position_follower();

    void on_forwardButton_clicked();

    void on_progressSlider_sliderMoved( int position );

    void on_rewindButton_clicked();

    void endSong();

private:
    Ui::player *ui;

    Spotify _spotify;

    playlistManager _playlist;

    QString _displayName;   //Nome de display do usuario logado

    QString _userId;        //Id do usuário logado

    QMessageBox loginWarning;   //Aviso para tentativa de ações antes de fazer login

    QMediaPlayer _musicPlayer;  //Objeto que reproduz o sample

    PlayerStatus _status;       //Indicador se música está parada, pausada ou tocando

    int _currentSong;           //Indice da música atual

    bool _songRemoved;          //Indicador de música removida da primeira posição

    void _playSong( const int index );

    void _nextSong();

    void _previousSong();
};

#endif // PLAYER_H
