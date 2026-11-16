#include "Server.hpp"
#include <iostream>
#include <stdexcept> //std::invalid_argument
#include <cstdlib> //std::strtol
#include <cerrno> // errno, ERANGE
#include <cstring> // std::strerror()
#include <sstream>

#include <netdb.h> //addrinfo()
#include <sys/socket.h> //socket()
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h> //fcntl()
#include <poll.h>

// ----------------------------------------------------
// Constructor / Destructor
// ----------------------------------------------------
Server::Server(const char * port_str, const char *password)
:   port_(validate_port_(port_str)),
    password_(validate_password_(password)),
    listener_fd_(-1),
    pfds_()
{
    setup_listener_();
}


Server::~Server(){}

// ----------------------------------------------------
// Validation
// ----------------------------------------------------
int Server::validate_port_(const char *port_str){
    if(port_str == NULL || *port_str == '\0')
        throw std::invalid_argument("Port is empty");

    //portが数値か
    for (int i = 0; port_str[i] != '\0'; ++i){
        if(port_str[i] < '0' || port_str[i] > '9')
            throw std::invalid_argument("Invalid port number");
    }

    errno = 0;
    char *end;
    long port = std::strtol(port_str, &end, 10);

    if(errno == ERANGE || end == port_str || *end != '\0')
    throw std::invalid_argument("Invalid port number");
    
    //TCP/UDPのポート番号は16ビットなので、範囲は1~65535(intの範囲より狭い)。
    if(port < 1 || port > 65535)
        throw std::invalid_argument("Port must be between 1 and 65535");
    
    return static_cast<int>(port);
}

//!passwordの文字数は指定しなくていいのか?
std::string Server::validate_password_(const char *password){
    if(password == NULL || *password == '\0')
        throw std::invalid_argument("Password is empty");

    //TODO::whitespaceのみの場合の処理
    //whitespaceだけのpasswordは受け付けるのか確認が必要

    return std::string(password);
}

// ============================================================
// Socket setup
// ============================================================
void Server::setup_listener_() {
    listener_fd_ = create_listener_socket_();
    try {
        configure_non_blocking_(listener_fd_);
        start_listening_(listener_fd_);

        //poll()監視対象に登録
        struct pollfd pfd;
        pfd.fd = listener_fd_;
        pfd.events = POLLIN;
        pfd.revents = 0;
        pfds_.push_back(pfd);
    }
    catch (...)
    {
        close(listener_fd_);
        listener_fd_ = -1;
        throw;
    }
}

int Server::create_listener_socket_(){
    struct addrinfo hints, *ai, *p;
    int listener = -1;
    int rv;
    int yes = 1;

    //アドレス情報取得
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; //!現在IPv4のみ。IPv6にも対応するならAF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    //int to string
    std::ostringstream oss;
    oss << port_;
    std::string service = oss.str();
    
    if((rv = getaddrinfo(NULL, service.c_str(), &hints, &ai)) != 0)
        throw std::runtime_error(gai_strerror(rv));

    //候補探し
    for (p = ai; p != NULL; p = p->ai_next){
        //socket引数①IPv4 / IPv6 などのアドレスファミリー ②TCP/UDP などの通信方式 ③使用するプロトコル(プロトコルを自動選択する)
        if((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;

        //* errno使うと、closeのerrnoが入ってしまうかも→errno使わない方がいいかも
        if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){
            close(listener);
            throw std::runtime_error(std::string("setsockopt failed: ") + std::strerror(errno));
        }

        //bind引数①紐づけるソケットのID②紐づけるIPアドレス・ポート番号の情報③アドレス情報のサイズ
        if(bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        { 
            close(listener);
            continue;
        }
        break;
    }

    if(p == NULL){
        freeaddrinfo(ai);
        throw std::runtime_error("Failed to create and bind listener socket");
    }

    //アドレス情報解放（リストの解放）
    freeaddrinfo(ai);

    return listener;
}

void Server::configure_non_blocking_(int fd)
{
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
    {
        throw std::runtime_error(
            std::string("fcntl O_NONBLOCK failed: ") + std::strerror(errno)
        );
    }
}


void Server::start_listening_(int fd){
    if(listen(fd, 10) == -1){ 
        throw std::runtime_error("listen failed.");
    }
}


// ============================================================
// Main loop
// ============================================================
void Server::run(){
    while(true){
        //引数①pfds_配列の先頭 ②pfds_のfdの数 ③イベント発生まで無期限に待つ
        int poll_count = poll(&pfds_[0], pfds_.size(), -1);
        if(poll_count == -1){
            if(errno == EINTR) //!EINTR→シグナルで中断するとerrnoにEINTRがセット
                continue;
            throw std::runtime_error(std::string("poll failed: ") + std::strerror(errno));
        }

        process_connections_();
    }
}

// ============================================================
// Connection handling
// ============================================================
void Server::process_connections_(){
    for (size_t i = 0; i < pfds_.size(); ++i){
        short revents = pfds_[i].revents;
        if (revents == 0)
            continue;

        // 無効なFD
        if (revents & POLLNVAL)
            throw std::runtime_error("poll: invalid file descriptor");

        // ソケットエラー
        if (revents & POLLERR)
            throw std::runtime_error("poll: socket error");

        // 通常のイベント処理
        if (pfds_[i].fd == listener_fd_)
        {
            if (revents & POLLHUP)
                throw std::runtime_error("Listener socket error");
            if (revents & POLLIN)
                handle_new_connection_();
        } else {
            if (revents & (POLLIN | POLLHUP))
                handle_client_data_(i);
        }
    }
}

void Server::handle_new_connection_() {
    //Ipv4とIpv6両対応のためsockaddr_storageをつかう
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen = sizeof(remoteaddr);

    //1回の accept()で(現在の実装)1人だけ受け付ける方法と、接続がなくなるまで繰り返し accept() する方法あり。
    int client_fd = accept(
        listener_fd_,
        reinterpret_cast<struct sockaddr *>(&remoteaddr),
        &addrlen
    );

    if(client_fd == -1){
        if(errno == EWOULDBLOCK || errno == EAGAIN)
            return;

        if(errno == EINTR)
            return;
        
        throw std::runtime_error(std::string("accept failed: ") + std::strerror(errno));
    }

    try{
        configure_non_blocking_(client_fd);
        struct pollfd pfd;
        pfd.fd = client_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        pfds_.push_back(pfd);
    } catch (...){
        close(client_fd);
        throw;
    }
}

void handle_client_data_(size_t index){
    //バッファ作る、初期化
    //変数 送り主のfdを保存
    //メッセージ受信
    //○エラー処理
    // - ノンブロッキングの時の対応
    // - 通信切断、致命的エラー
    //○常にデータ受信した時の処理
    // -とりあえずバッファに詰め込む
    // -バッファから\nの場所探して部分文字列切り出す
    // -最後の文字\rだったら消す
    // -受付と送り主以外に送る
    //sendしたメッセージは消す

}
