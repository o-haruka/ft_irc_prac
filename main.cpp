#include "Message.hpp"
#include "Server.hpp"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    try {
        // 1. 引数からポート番号とパスワードを受け取り、Serverオブジェクト作成
        Server irc_server(argv[1], argv[2]);

        // 2. サーバーを起動する（この中でずーっと通信を待ち受けるループが回る）
        irc_server.run();
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
