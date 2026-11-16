#include <iostream>
#include "Message.hpp"

void printMessage(const Message& msg){
    std::cout << "Prefix : [" << msg.getPrefix() << "]" << std::endl;
    std::cout << "Command: [" << msg.getCommand() << "]" << std::endl;
    
    std::vector<std::string> params = msg.getParams();
    for (size_t i = 0; i < params.size(); ++i) {
        std::cout << "Param[" << i << "]: [" << params[i] << "]" << std::endl;
    }
    std::cout << "-----------------------" << std::endl;
}

int main(){
    // テストケース1：通常のUSERコマンド（Trailingあり）
    std::string raw_data1 = "USER haruka 0 * :Haruka Omura\r\n";
    std::cout << "【Test 1】 " << raw_data1;
    Message msg1(raw_data1);
    printMessage(msg1);

    // テストケース2：Prefixが含まれるコマンド（JOINなど）
    std::string raw_data2 = ":haruka!user@localhost JOIN :#42tokyo\r\n";
    std::cout << "【Test 2】 " << raw_data2;
    Message msg2(raw_data2);
    printMessage(msg2);
}