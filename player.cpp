#include "player.h"
#include "ui_player.h"
#include "clientid.h"

#include <QDesktopServices>
#include <QUrlQuery>
#include <QInputDialog>

player::player( QWidget *parent )
    : QWidget(parent)
    , ui(new Ui::player)
    , _spotify(this, CLIENT_ID, CLIENT_SECRET)  //Inicia objeto Spotify com chaves de desenvolvedor para a API
{
    ui->setupUi(this);

    loginWarning.setText("Faça login para continuar."); //Aviso caso tente realizar operações antes de fazer login.
    _status = PlayerStatus::stopped;
    _songRemoved = false;

    /*--------------------------------- Music Player setup -------------------------------------------*/
    connect(&_musicPlayer, &QMediaPlayer::positionChanged, this, &player::position_follower);
    connect(&_musicPlayer, &QMediaPlayer::mediaStatusChanged, this, &player::endSong);
    _musicPlayer.setVolume(ui->volumeSlider->value());

    /*-------------------------------Spotify API setup------------------------------*/             //Objeto que faz conexão com a API
    connect(&(_spotify.httpClient), &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,           //Sinal de resposta ao pedido de autorização da API
            &QDesktopServices::openUrl);
    connect(&(_spotify.httpClient), &QOAuth2AuthorizationCodeFlow::granted,                        //Sinal de resposta quando a autorização é concedida
            this, &player::granted);

}

player::~player()
{
    delete ui;
}

void player::granted()
{
    //Após autenticação com o servidor
    auto reply = _spotify.getUserName();    //Resposta de pedido de informações de usuario ao servidor
    connect(reply, &QNetworkReply::finished, [=]() {
        if( _spotify.handleGetUserName(reply, _displayName, _userId ) ) {   //Resposta sem erro
            ui->Login->setText("Logout");
            ui->outputLabel->setText("Usuário: " + _displayName);
            if( _playlist.setUser(_userId) ){       //Checa se o arquivo save foi aberto.
                for( auto obj : _playlist.playlistsVector) {
                    ui->selectPlaylist->addItem(obj.value("name").toString());
                }
            }
            else {
                qWarning( "Erro ao abrir o arquivo de save" );
            }
        }
        else {
            QMessageBox msg;
            msg.setText("Erro ao identificar usuário");
            msg.exec();
        }

    });
}


void player::on_Login_clicked()
{
    //Botão para realizar Login/Logout

    if( !_spotify.isGranted ) { //Não está autenticado - botão é pressionado para fazer login
        _spotify.httpClient.grant();
    }
    else {  //Logout
        _spotify.isGranted = false;
        ui->Login->setText("Login");
        ui->outputLabel->setText("Faça login para continuar.");
        ui->selectPlaylist->clear();
        ui->playlistWidget->clear();
        ui->resultsWidget->clear();
        _currentSong = -1;
        _nextSong();
    }
}

void player::on_playlistWidget_itemActivated(QListWidgetItem *item)
{
    //Se clicar duas vezes em um dos items da playlist, começa a tocá-lo
    _playSong(ui->playlistWidget->currentRow());
}

void player::keyPressEvent(QKeyEvent *event)
{
    //Se pressionar Enter após digitar uma busca, faz o equivalente a clicar no botão "buscar"
    if(ui->searchBar->hasFocus()){
        if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            on_searchButton_clicked();
        }
    }
}

void player::on_searchButton_clicked()
{
    // Clicar no botão "Buscar"
    QString text = ui->searchBar->text().toLower();
    ui->searchBar->setText("");
    ui->resultsWidget->clear();
    if( !_spotify.isGranted ) {
        loginWarning.exec();
        return;
    }
    else {
        auto reply = _spotify.search(text); //Resposta ao pedido de busca à API
        connect(reply, &QNetworkReply::finished, [=]() {
            reply->deleteLater();
            if( _spotify.handleSearch(reply) ){
                for ( auto obj : _spotify.searchResults ) { //Resultados da busca parseados em Json
                    QString display = obj["name"].toString() + " - " + obj["artist"].toString() + " (" + obj["album"].toString() + ")";
                    ui->resultsWidget->addItem(display);
                }
            }
            else {
                QMessageBox msg;
                msg.setText("Nenhum resultado encontrado.");
                msg.exec();
            }
        });
    }
}

void player::on_addButton_clicked()
{
    //Botão de adicionar música à playlist
    if( !_spotify.isGranted ){
        loginWarning.exec();
        return;

    }
    int currentRow = ui->resultsWidget->currentRow();
    if( currentRow < 0) {   //Quando não há nada selecionado, função currentRow() retorna -1
        QMessageBox msg;
        msg.setText("Selecione uma música para adicionar à playlist!");
        msg.exec();
    }
    else {
        QJsonObject obj = _spotify.searchResults[currentRow];   //Objeto é selecionado antes do pedido de get para, caso o servidor demore a responder, o usuário não clique em outro item e acabe adicionando a música errada
        auto reply = _spotify.getPreview(obj["id"].toString()); //Pedido da URL de Preview do spotify - ja checa se arquivo tem preview
        connect( reply, &QNetworkReply::finished, [=]() {
            reply->deleteLater();
            QString url = _spotify.handleGetPreview(reply);
            if(url == "ERROR") {
                QMessageBox msg;
                msg.setText("Erro ao conectar com o servidor.");
                msg.exec();
            }
            else if (url.isEmpty()) {
                QMessageBox msg;
                msg.setText("A música selecionada não tem preview disponível.");
                msg.exec();
            }
            else {
                QJsonObject song = obj; //obj é const dentro da conexão, para alterar o valor de "preview_url" é necessário novo objeto volátil
                song["preview_url"] = url;
                QString display = song["name"].toString() + " - " + song["artist"].toString() + " (" + song["album"].toString() + ")";
                ui->playlistWidget->addItem(display);
                _playlist.tracksVector.append(song);
            }
        });
    }
}

void player::on_createButton_clicked()
{
    //Botão para criar playlist
    if( !_spotify.isGranted ) {
        loginWarning.exec();
        return;
    }

    //Pedido para usuário digitar nome da playlist
    bool ok;
    QString inputText = QInputDialog::getText(this, tr("Criar nova playlist"),
                                             tr("Digite o nome de sua playlist:"), QLineEdit::Normal,
                                             "", &ok);
    _playlist.createNewPlaylist(inputText);
    ui->selectPlaylist->addItem(inputText);
    ui->selectPlaylist->setCurrentIndex((ui->selectPlaylist->count())-1); //Ao criar nova playlist, seleciona ela como playlist atual
    _currentSong = -1;  //Indicar que a música atual não está na playlist
}

void player::on_selectPlaylist_currentIndexChanged(int index)
{
    //Sinal que indica que a playlist atual foi trocada
    _currentSong = -1;  //Indicar que a música atual não está na playlist
    if( index >= 0 ) {  //Index -1 significa que o widget está vazio
        _playlist.switchToPlaylist(index);
        ui->playlistWidget->clear();
        for ( auto obj : _playlist.tracksVector ) { //Vetor com as músicas da playlist salvas como objetos Json
            QString display = obj["name"].toString() + " - " + obj["artist"].toString() + " (" + obj["album"].toString() + ")";
            ui->playlistWidget->addItem(display);
        }
    }
    else {
        ui->playlistWidget->clear();    //Não há playlists, esvazia o widget.
    }
}

void player::on_saveButton_clicked()
{
    //Botão de salvar as alterações na playlist
    if( !_spotify.isGranted ){
        loginWarning.exec();
        return;
    }
    if( ui->selectPlaylist->currentIndex() < 0 ) {  //Checa se há alguma playlist selecionada
        QMessageBox msg;
        msg.setText("Selecione uma playlist para salvar!");
        msg.exec();
        return;
    }
    _playlist.save();
}

void player::on_removeButton_clicked()
{
    //Botão para remover música selecionada da playlist
    if( !_spotify.isGranted ) {
        loginWarning.exec();
        return;
    }
    int index = ui->playlistWidget->currentRow();   //Musica selecionada
    if (index < 0) {    //Index -1 representa nenhuma seleção
        QMessageBox msg;
        msg.setText("Escolha uma música para remover!");
        msg.exec();
        return;
    }
    _playlist.tracksVector.remove(index);   //Remove do vetor de musicas
    ui->playlistWidget->removeItemWidget(ui->playlistWidget->currentItem()); //remove do widget

    if( index <= _currentSong ) {   //Lógica para que o player vá para a próxima música/música anterior da lista ignorando a música removida
        if( _currentSong == 0 ) {
            _songRemoved = true;
        }
        else {
            _currentSong--;
        }
    }

    delete ui->playlistWidget->currentItem();
}

void player::on_volumeSlider_valueChanged(int value)
{
    // Slider de volume, ja configurado com valor de 0 a 100
    _musicPlayer.setVolume(value);
    if(value == 0) {
        ui->volumeLabel->setStyleSheet("image: url(:/images/images/mute.PNG)");
        return;
    }
    else if (value < 30) {
        ui->volumeLabel->setStyleSheet("image: url(:/images/images/volumeLow.PNG)");
        return;
    }

    else if (value < 60) {
        ui->volumeLabel->setStyleSheet("image: url(:/images/images/volumeMedium.PNG)");
        return;
    }

    else {
        ui->volumeLabel->setStyleSheet("image: url(:/images/images/volumeHigh.PNG)");
        return;
    }
}

void player::on_playButton_clicked()
{
    //Botão para tocar/pausar música
    if( !_spotify.isGranted ) {
        loginWarning.exec();
        return;
    }
    switch( _status ) {
        case(PlayerStatus::playing):
            //Pausar
            _musicPlayer.pause();
            ui->playButton->setStyleSheet("image: url(:/images/images/play.PNG)");
            _status = PlayerStatus::paused;
            break;
        case(PlayerStatus::paused):
            //Resumir
            _musicPlayer.play();
            ui->playButton->setStyleSheet("image: url(:/images/images/pause.PNG)");
            _status = PlayerStatus::playing;
            break;
        default:
            //Começar a tocar
            int playlist = ui->selectPlaylist->currentIndex();
            if( playlist < 0 ) { //Checa se há playlist selecionada
                QMessageBox msg;
                msg.setText("Selecione uma playlist para tocar.");
                msg.exec();
            }
            else if ( !_playlist.tracksVector.isEmpty() ){ //Checa se há músicas na playlist
                int index = ui->playlistWidget->currentRow();
                if(index < 0) { //Se não houver música da playlist selecionada, começa a tocar do início
                    _playSong(0);
                }
                else {
                    _playSong(index);
                }
            }
            break;
    }
}

void player::on_deleteButton_clicked()
{
    //Botão para remover playlist
    if( !_spotify.isGranted ) {
        loginWarning.exec();
        return;
    }
    int index = ui->selectPlaylist->currentIndex();
    if (index < 0) {    //Checa se há playlist selecionada
        QMessageBox msg;
        msg.setText("Selecione uma playlist para deletar.");
        msg.exec();
    }
    else {
       QMessageBox msg; //Pede confirmação do usuário para deletar
       msg.setText("A playlist atual será deletada.");
       msg.setInformativeText("Tem certeza que deseja continuar?");
       msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
       msg.setDefaultButton(QMessageBox::No);
       int ret = msg.exec();
       if( ret == QMessageBox::Yes ) {
           ui->selectPlaylist->removeItem(index);
           _playlist.removePlaylist(index);
           on_selectPlaylist_currentIndexChanged(ui->selectPlaylist->currentIndex()); //Troca para a playlist indicada no widget
       }
    }
}

void player::position_follower()
{
    //Slot que acompanha o sinal de tempo da música para atualizar barra de progresso, por default enviado a cada 1000ms (1 segundo)
    int currentSeconds = _musicPlayer.position()/1000;  //Posição é dada em ms
    ui->progressSlider->setValue(currentSeconds);   //Slider configurado no intervalo 0 a 30
    int minutes = currentSeconds/60;    //Para uso futuro caso a música tenha mais de 1 minuto
    int seconds = currentSeconds%60;
    ui->timeLabel->setText(QString::number(minutes).rightJustified(2, '0') + ":" + QString::number(seconds).rightJustified(2, '0')); //Preenche com 0s
}

void player::_playSong(const int index)
{
    //Comando para tocar música
    _musicPlayer.stop();

    QJsonObject song = _playlist.tracksVector[index]; //Objeto com as informações da música
    QString url = song.value("preview_url").toString();
    QString title = song.value("name").toString();
    QString artist = song.value("artist").toString();

    _musicPlayer.setMedia(QUrl(url));
    //Atualiza as informações do player sobre música atual
    ui->artistLabel->setText(artist);
    ui->titleLabel->setText(title);
    ui->timeLabel->setText("00:00");

    _currentSong = index;   //Indica a posição da música sendo tocada atualmente
    _status = PlayerStatus::playing;
    ui->playButton->setStyleSheet("image: url(:/images/images/pause.PNG)");
    ui->playlistWidget->item(_currentSong)->setSelected(true);  //Indica no widget a música tocada, selecionando-a
    ui->playlistWidget->setFocus();
    ui->progressSlider->setSliderPosition(0);
    _musicPlayer.play();
}

void player::_nextSong()
{
    //Função a ser chamada quando a música acaba de tocar ou botão "forward" é pressionado
    _musicPlayer.stop();

    /* Condicional para verificar se a música sendo tocada previamente era a primeira da lista e foi removida,
     * garantindo que a próxima a ser tocada será a nova primeira da lista */
    if( _currentSong == 0 && _songRemoved && !_playlist.tracksVector.isEmpty() ) {
        _playSong(_currentSong);
    }
    else if(_currentSong == -1 || _currentSong >= (_playlist.tracksVector.size() - 1)) {
        //Caso o vetor seja esvaziado ou a playlist trocada, para de tocar
        ui->progressSlider->setValue(0);
        ui->timeLabel->setText("");
        ui->artistLabel->setText("");
        ui->titleLabel->setText("");
        ui->playButton->setStyleSheet("image: url(:/images/images/play.PNG)");
        _status = PlayerStatus::stopped;
    }
    else {
        _currentSong++;
        _playSong(_currentSong);
    }
    _songRemoved = false;
}

void player::on_forwardButton_clicked()
{
    //Botão para ir para próxima música
    if( !_spotify.isGranted ){
        loginWarning.exec();
        return;
    }
    _nextSong();
}

void player::on_progressSlider_sliderMoved(int position)
{
    //Caso o usuário queira avançar a música, apenas precisa arrastar o slider
    if(_status == PlayerStatus::stopped) {
        ui->progressSlider->setSliderPosition(0);
    }
    else {
        _musicPlayer.setPosition(position*1000);
    }
}

void player::on_rewindButton_clicked()
{
    //Botão para voltar à música anterior

    if( !_spotify.isGranted ){
        loginWarning.exec();
        return;
    }

    if(_status == PlayerStatus::stopped) {
        return;
    }

    else if(_currentSong == 0 && !_playlist.tracksVector.isEmpty()) { //Se for a primeira da lista, apenas reinicia a música atual
        _playSong(0);
    }
    else if (_currentSong > 0) {
        _currentSong--;
        _playSong(_currentSong);
    }
}

void player::endSong()
{
    //Sinal que indica que a música acabou.
    if(_musicPlayer.mediaStatus() == QMediaPlayer::EndOfMedia) {
        _nextSong();
        return;
    }
}
