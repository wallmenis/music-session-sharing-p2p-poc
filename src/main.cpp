#include "musicSession.h"
#include <memory>
#include <fstream>
#include <nlohmann/json_fwd.hpp>
#include <unistd.h>

int main(int argc, char *argv[])
{
    std::string otherPear;
    std::stringstream inpStrm;
    
    if(argc > 1)
    {
        for (int i = 0; i < KEYLEN; i++)
        {
            inpStrm << argv[1][i];
        }
        otherPear = inpStrm.str();
    }
    if(argc > 2)
    {
        
    }
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
    
    std::cout << mu->getCode()  << "\n"; //<< "Please enter peer code: ";
    // std::cin >> inp;
    // std::cin.ignore();
    if(!otherPear.empty() && argc > 2)
    {
        mu->makeConnection(otherPear);
        nlohmann::json track;
        
        track = {
            {"track","gamer"},
            {"hash", "gaming"},
            {"uri", "no"}
        };
        
        mu->addSong(track,0);
        
        nlohmann::json sessionInfo = 
        {
            {"playState", MusicSession::playState::PLAYING},
            {"timeStamp",0},
            {"playlistPos", 0},
            {"numberOfSongs", 1},
            {"playlistChkSum", mu->getPlaylistSum()},
            {"priority", mu->getCode()}
        };
        mu->waitUntilCanBeHeard();
        mu->setInfo(sessionInfo);
        //mu->askSync();
        mu->assertState();
    }
    while(true)
    {
        // if(!otherPear.empty())
        // {
        //     nlohmann::json sessionInfo = 
        //     {
        //         {"playState", MusicSession::playState::PLAYING},
        //         {"timeStamp",0},
        //         {"playlistPos", 0},
        //         {"numberOfSongs", 1},
        //         {"playlistChkSum", mu->getPlaylistSum()},
        //         {"priority", mu->getCode()}
        //     };
        //     mu->setInfo(sessionInfo);
        // }
        sleep(1);
        std::cout << "Time: " << std::chrono::system_clock::now().time_since_epoch().count() << " in id: " << mu->getCode() << "\n";
        std::cout << mu->getNumberOfPeers() << std::endl;
        std::cout << mu->getSessionInfo().dump() << std::endl;
        std::vector<nlohmann::json> psnapshot(mu->getPlaylist());
        for ( nlohmann::json song : psnapshot)
        {
            std::cout << song.dump() << "\n";
        }
        std::cout << " arg count: " << argc << std::endl;
        if(otherPear.empty() || argc < 3)
        {
            
            std::cout << "----------------------------------------ASKING TO SYNC!!!!----------------------------------------\n";
            mu->askSync();
        }
        
    }
    //std::cin >> inp;
    mu->endSession();
    return 0;
}