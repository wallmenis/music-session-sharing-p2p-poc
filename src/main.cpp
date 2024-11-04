#include <cstdlib>
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

std::string randid(int size);
std::shared_ptr<rtc::PeerConnection> createAPeerConnection(rtc::Configuration config);
std::shared_ptr<rtc::DataChannel> createADataChannel();

bool is_offerer;
bool ss_is_avail; //Flag for if the signalling server is availiable to send/receive;
std::string myId;

int main()
{
    myId = randid(5);
    ss_is_avail = false;
    std::shared_ptr<rtc::WebSocket> ws;
    std::promise<void> websocket_promise;
    auto websocket_promise_future = websocket_promise.get_future();
    rtc::Configuration config;
    
    ws = std::make_shared<rtc::WebSocket>();
    //bool signalling_server_message_availiable = false;
    
    ws->onOpen([&websocket_promise]() {
        std::cout << "WebSocket open" << std::endl;
        websocket_promise.set_value();
    }); 
    
    ws->onMessage([](std::variant<rtc::binary, rtc::string> message) {
        if (std::holds_alternative<rtc::string>(message)) {
            std::string msg = std::get<rtc::string>(message);
            std::cout << "WebSocket received: " << msg << std::endl;
            
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
    
    std::cout << "Is this an offerer?[Y/N]\n";
    std::cin >> inp;
    if (inp.c_str()[0] == 'Y')
    {
        is_offerer = true;
    }
    else {
        is_offerer = false;
        std::cout << "Please enter the offerer's ID\n";
        std::cin >> inp;
    }
    std::shared_ptr<rtc::PeerConnection> pc = createAPeerConnection(config);
    pc->onLocalDescription([&ws](rtc::Description sdp){
        nlohmann::json message =
        {{"id", myId},
        {"type", sdp.typeString()},
        {"description", std::string(sdp)}
        };
        if(ss_is_avail)
        {
            ss_is_avail = false;
            ws->send(message.dump());
            ss_is_avail = true;
        }
    });
    pc->onLocalCandidate([&ws](rtc::Candidate candidate){
        nlohmann::json message =
        {{"id", myId},
        {"type", "candidate"},
        {"candidate", std::string(candidate)},
        {"mid", candidate.mid()}
        };
        if(ss_is_avail)
        {
            ss_is_avail = false;
            ws->send(message.dump());
            ss_is_avail = true;
        }
        
    });
    if (is_offerer)
    {
        
    }
    else {

    }
    // while(ws.isOpen())
    // {
    //     if(ss_is_avail)
    //     {
    //     }
    // }
    
    
    // while(!ws.isOpen());
    // ws.send("test");
    ws->close();
    
//     rtc::Configuration config;
//     config.iceServers.emplace_back("stun.l.google.com:19302");
//     rtc::PeerConnection pc(config);
//     
//     //auto weak_ws = std::make_weak<rtc::WebSocket> (ws);
//     
//     pc.onLocalDescription([&ws](rtc::Description sdp) {
//         // Send the SDP to the remote peer
//         ws.send(std::string(sdp));
//         //MY_SEND_DESCRIPTION_TO_REMOTE(std::string(sdp));
//     });
//     
//     pc.onLocalCandidate([&ws](rtc::Candidate candidate) {
//         // Send the candidate to the remote peer
//         ws.send(candidate.candidate());
//         //candidate.mid()
//         //MY_SEND_CANDIDATE_TO_REMOTE(candidate.candidate(), candidate.mid());
//     });
    
    // MY_ON_RECV_DESCRIPTION_FROM_REMOTE([&pc](std::string sdp) {
    //     pc.setRemoteDescription(rtc::Description(sdp));
    // });
    
    // MY_ON_RECV_CANDIDATE_FROM_REMOTE([&pc](std::string candidate, std::string mid) {
    //     pc.addRemoteCandidate(rtc::Candidate(candidate, mid));
    // });
    
//     pc.onStateChange([](rtc::PeerConnection::State state) {
//         std::cout << "State: " << state << std::endl;
//     });
//     
//     pc.onGatheringStateChange([](rtc::PeerConnection::GatheringState state) {
//         std::cout << "Gathering state: " << state << std::endl;
//     });
//     
//     auto dc = pc.createDataChannel("test");
//     
//     dc->onOpen([]() {
//         std::cout << "Open" << std::endl;
//     });
//     
//     dc->onMessage([](std::variant<rtc::binary, rtc::string> message) {
//         if (std::holds_alternative<rtc::string>(message)) {
//             std::cout << "Received: " << get<rtc::string>(message) << std::endl;
//         }
//     });
//     
//     //std::shared_ptr<rtc::DataChannel> dc;
//     pc.onDataChannel([&dc](std::shared_ptr<rtc::DataChannel> incoming) {
//         dc = incoming;
//         dc->send("Hello world!");
//     });
    
    return 0;
}

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


std::shared_ptr<rtc::PeerConnection> createAPeerConnection(rtc::Configuration config)
{
    
    auto pc = std::make_shared<rtc::PeerConnection>(config);
    return pc;
}

std::shared_ptr<rtc::DataChannel> createADataChannel()
{
    return NULL;
}

