#include "musicSession.h"
#include <memory>
#include <fstream>
#include <nlohmann/json_fwd.hpp>

int main()
{
    nlohmann::json configs ={{
        "signaling_server", "127.0.0.1:8000"},
        {"ice_server" , "127.0.0.1:3479"
    }};
    // nlohmann::json tst= {
    //     {"playState", MusicSession::playState::PAUSED},
    //     {"timeStamp", "aaaaaa"},
    //     {"playlistPos", 0}
    // };
    std::cout << configs.dump() << std::endl;
    std::string tmp;
    std::stringstream json_string;
    std::ifstream file("config.json");
    if (file.is_open())
    {
        while(std::getline(file,tmp))
        {
            json_string << tmp;
        }
        configs = nlohmann::json::parse(json_string.str());
    }
    else {
        std::cerr << "Faied to read config file, using default values.\n";
    }
    // nlohmann::json test = {
    //     {"test" , "aaa"}
    // };
    // std::cout << test.find("test").value() << "\n";
    //std::cout << configs.find("signaling_server").value() << "\n";
    //std::cout << MusicSession::playState::PLAYING << std::endl;
    
    
    
    
    std::unique_ptr<MusicSession> mu = std::make_unique<MusicSession>(configs);
    
    std::string inp;
    
    std::cout << mu->getCode() << "\n Please enter peer code:";
    std::cin >> inp;
    std::cin.ignore();
    mu->makeConnection(inp);
    
    
    std::cin >> inp;
    
    mu->endSession();
    return 0;
}