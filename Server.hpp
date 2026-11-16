#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <poll.h>
#include <cstddef> //size_t
#include <map>

class Server{
    public:
        // Constructor / Destructor
        Server(const char* port_str, const char* password);
        ~Server();

        // Main loop
        void run();

    private:
        // Member variables
        int                         port_; //サーバーが待ち受けるポート番号
        std::string                 password_; //クライアント接続時に使用するパスワード
        int                         listener_fd_; //接続要求を待ち受けるソケット(受付用)
        std::vector<struct pollfd>  pfds_; //	poll() で監視するファイルディスクリプタの一覧

        // Client management
        std::map<int, std::string> client_buffers_;

        //validation
        static int          validate_port_(const char *port_str);
        static std::string  validate_password_(const char *password);

        // Listener setup
        void    setup_listener_();
        int     create_listener_socket_();
        void    configure_non_blocking_(int fd);
        void    start_listening_(int fd);

        // Connection handling
        void    process_connections_();
        void    handle_new_connection_();
        void    handle_client_data_(size_t index);
};

#endif
