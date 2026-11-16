#include "Message.hpp"
#include <string>
#include <vector>

// ----------------------------------------------------
// CONSTRUCTOR
// ----------------------------------------------------
Message::Message(const std::string& raw_data){
    std::string data = raw_data;

    removeCrLf_(data);
    parsePrefix_(data);
    parseCommand_(data);
    parseParams_(data);
}

// ----------------------------------------------------
// METHOD
// ----------------------------------------------------
//* 1. \r\nを削除
void Message::removeCrLf_(std::string& data){
    size_t pos = data.find("\r\n");
    if(pos != std::string::npos)
        data.erase(pos); //\r\n以降削除
}

//* 2. 特別ルール：先頭が ':' なら、それは Prefix (送信元情報)
void Message::parsePrefix_(std::string& data){
    if(!data.empty() && data[0] == ':'){
        size_t space_pos = data.find(' ');
        if(space_pos != std::string::npos){
            prefix_ = data.substr(1, space_pos - 1);
            data.erase(0, space_pos + 1); //?なんで+1??
        }
    }else{
        prefix_ = "";
    }
}

//* 3. Command の抽出 (最初のスペースまで)
void Message::parseCommand_(std::string& data){
    size_t space_pos = data.find(' ');
    if(space_pos != std::string::npos){
        command_ = data.substr(0, space_pos);
        data.erase(0, space_pos + 1);
    } else {
        command_ = data; //spaceない=全体がcommand
        data.clear(); //?clear()
    }
}

//* 4. パラメータ (Params) の抽出をループで繰り返す
void Message::parseParams_(std::string& data){
    while(!data.empty()){
        // 余分なスペースが連続していたら読み飛ばす
        while(!data.empty() && data[0] == ' ')
            data.erase(0, 1);

        if(data.empty())
            break;

        // Trailing(トレイリング) のルール → ':'から末尾までひとまとまり。
        if(data[0] == ':'){
            params_.push_back(data.substr(1));
            break;
        }

        // 通常のパラメータ(空白で区切る)
        size_t space_pos = data.find(' ');
        if(space_pos != std::string::npos){
            params_.push_back(data.substr(0, space_pos));
            data.erase(0, space_pos + 1);
        }else{
            params_.push_back(data);
            break;
        }
    }
}

// void Server::handleMessage(int client_fd, const std::string& raw_data) {
//     // 1. パース処理（Messageクラスに任せる）
//     Message msg(raw_data);

//     // 2. 振り分け処理
//     if (msg.getCommand() == "PASS") {
//         this->executePass(client_fd, msg.getParams());
//     } else if (msg.getCommand() == "NICK") {
//         this->executeNick(client_fd, msg.getParams());
//     } else if (msg.getCommand() == "USER") {
//         this->executeUser(client_fd, msg.getParams());
//     } else {
//         // 未知のコマンドエラーを返す処理
//     }
// }

/* GETTER */
std::string Message::getPrefix() const{
    return prefix_;
}

std::string Message::getCommand() const{
    return command_;
}

std::vector<std::string> Message::getParams() const{
    return params_;
}

