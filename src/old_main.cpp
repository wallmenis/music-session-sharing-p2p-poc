#include <cstdlib>
#include <nlohmann/json_fwd.hpp>
#include <rtc/datachannel.hpp>
#include <rtc/rtc.hpp>
#include <iostream>
#include <sstream>
#include <rtc/configuration.hpp>
#include <rtc/peerconnection.hpp>
#include <rtc/websocket.hpp>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include <chrono>
#include <future>
#include <vector>

class MusicSession{
public:
    MusicSession()
    {
        
    }
    //ok i have tried to make this as much different as the client... I just can't... this is a very good idea from the example client...
    //don't know if i should change the license to mpl for compatibility... I really want to keep gpl for kde but i am not sure...
    //paul if you see this... can you please... let me have it as gpl... i really need that.
    std::unordered_map<std::string, std::shared_ptr<rtc::PeerConnection>> pcons;
    std::unordered_map<std::string, std::shared_ptr<rtc::DataChannel>> dcons;
};
MusicSession mu;

std::string randid(int size);
std::shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration &config,std::weak_ptr<rtc::WebSocket> wws, std::string id);
bool is_offerer;
bool ss_is_avail; //Flag for if the signalling server is availiable to send/receive;
std::string myId;


int main() try
{
    rtc::InitLogger(rtc::LogLevel::Info);
    myId = randid(5);
    ss_is_avail = false;
    std::shared_ptr<rtc::WebSocket> ws;
    std::weak_ptr<rtc::WebSocket> wws = ws;
    std::promise<void> websocket_promise;
    std::promise<void> descision_if_offerer;
    auto websocket_promise_future = websocket_promise.get_future();
    auto descision_if_offerer_future = descision_if_offerer.get_future();
    rtc::Configuration config;
    
    
    ws = std::make_shared<rtc::WebSocket>();
    //bool signalling_server_message_availiable = false;
    
    ws->onOpen([&websocket_promise]() {
        std::cout << "WebSocket open" << std::endl;
        websocket_promise.set_value();
    }); 
    
    ws->onMessage([&descision_if_offerer_future, wws, &config](std::variant<rtc::binary, rtc::string> message) {
        std::string msg;
        bool is_safe_to_read = false;
        std::cout << "WebSocket received: ";
        if (std::holds_alternative<rtc::string>(message)) {
            is_safe_to_read = true;
            msg = std::get<rtc::string>(message);
            std::cout << msg << std::endl;
            descision_if_offerer_future.get();
        }
        else {
            std::cout << "Binary data received, ignoring";
            is_safe_to_read = false;
        }
        if(is_safe_to_read)
        {
            nlohmann::json msg_json = nlohmann::json::parse(msg);
            auto it = msg_json.find("id");
            if (it == msg_json.end())
                return;
            
            auto id = it->get<std::string>();
            
            it = msg_json.find("type");
            if (it == msg_json.end())
                return;
            
            auto type = it->get<std::string>();
            
            std::shared_ptr<rtc::PeerConnection> pc;
            if (auto jt = mu.pcons.find(id); jt != mu.pcons.end()) {
                pc = jt->second;
            } else if (type == "offer") {
                std::cout << "Answering to " + id << std::endl;
                pc = createPeerConnection(config, wws, id);
            } else {
                return;
            }
            
            if (type == "offer" || type == "answer") {
                auto sdp = msg_json["description"].get<std::string>();
                pc->setRemoteDescription(rtc::Description(sdp, type));
            } else if (type == "candidate") {
                auto sdp = msg_json["candidate"].get<std::string>();
                auto mid = msg_json["mid"].get<std::string>();
                pc->addRemoteCandidate(rtc::Candidate(sdp, mid));
            }
        }
    });
    
//     ws.onAvailable([]()
//     {
//         
//         
//     }
//     );
    
    std::cout << "ID = " << myId << "\n";
    std::stringstream finalurl;
    finalurl << "127.0.0.1:8000/" << myId;
    //std::cout << finalurl.str();
    ws->open(finalurl.str());
    websocket_promise_future.get();
    ss_is_avail = true;
    std::cout << "Connected to signaling server with id " << myId << "\n";
    config.iceServers.emplace_back("127.0.0.1:3479");
    std::string inp;
    
    //std::cout << "Is this an offerer?[Y/N]\n";
    //std::cin >> inp;
    descision_if_offerer.set_value();
    if (inp.c_str()[0] == 'Y')
    {
        is_offerer = true;
        descision_if_offerer.set_value();
    }
    else {
        is_offerer = false;
        std::cout << "Please enter the offerer's ID\n";
        std::cin >> inp;
    }
    std::shared_ptr<rtc::PeerConnection> pc = createPeerConnection(config,wws,myId);
    
    while(true)
    {
        std::cout << "Please enter the offerer's ID\n";
        std::cin >> inp;
    }
    ws->close();
    
    return 0;
} catch(const std::exception &e) {
    std::cout << e.what() << "\n";
    mu.dcons.clear();
    mu.pcons.clear();
};

// Create and setup a PeerConnection
std::shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration &config, std::weak_ptr<rtc::WebSocket> wws, std::string id) {
    auto pc = std::make_shared<rtc::PeerConnection>(config);
    
    pc->onStateChange(
        [](rtc::PeerConnection::State state) { std::cout << "State: " << state << std::endl; });
    
    pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state) {
        std::cout << "Gathering State: " << state << std::endl;
    });
    
    pc->onLocalDescription([wws, id](rtc::Description description) {
        nlohmann::json message = {{"id", id},
        {"type", description.typeString()},
                           {"description", std::string(description)}};
                           
                           if (auto ws = wws.lock())
                               ws->send(message.dump());
    });
    
    pc->onLocalCandidate([wws, id](rtc::Candidate candidate) {
        nlohmann::json message = {{"id", id},
        {"type", "candidate"},
        {"candidate", std::string(candidate)},
                         {"mid", candidate.mid()}};
                         
                         if (auto ws = wws.lock())
                             ws->send(message.dump());
    });
    
    pc->onDataChannel([id](std::shared_ptr<rtc::DataChannel> dc) {
        std::cout << "DataChannel from " << id << " received with label \"" << dc->label() << "\""
        << std::endl;
        
        std::weak_ptr<rtc::DataChannel> wdc;
        wdc = dc;
        
        dc->onOpen([wdc]() {
            if (auto dc = wdc.lock())
                dc->send("Hello from " + myId);
        });
        
        dc->onClosed([id]() { std::cout << "DataChannel from " << id << " closed" << std::endl; });
        
        dc->onMessage([id](auto data) {
            // data holds either std::string or rtc::binary
            if (std::holds_alternative<std::string>(data))
                std::cout << "Message from " << id << " received: " << std::get<std::string>(data)
                << std::endl;
            else
                std::cout << "Binary message from " << id
                << " received, size=" << std::get<rtc::binary>(data).size() << std::endl;
        });
        
        mu.dcons.emplace(id, dc);
    });
    
    mu.pcons.emplace(id, pc);
    return pc;
                                                     };

std::string randid(int size)
{
    int seed = std::chrono::system_clock::now().time_since_epoch().count();
    srand(seed);
    std::stringstream outp;
    const char maptonum[] = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i = 0; i<size; i++)
    {
        outp << maptonum[rand()%36];
    }
    return outp.str();
}

