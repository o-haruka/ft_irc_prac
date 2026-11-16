#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>

class Message {
private:
    std::string prefix_;
    std::string command_;
    std::vector<std::string> params_;

    void removeCrLf_(std::string& data);
    void parsePrefix_(std::string& data);
    void parseCommand_(std::string& data);
    void parseParams_(std::string& data);

public:
    // コンストラクタ（ここで raw_data を受け取ってパースする）
    Message(const std::string& raw_data); 
    
    // GETTER
    std::string getPrefix() const;
    std::string getCommand() const;
    std::vector<std::string> getParams() const;
};

#endif
