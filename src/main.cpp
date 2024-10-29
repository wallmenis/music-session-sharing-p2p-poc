#include <rtc/rtc.hpp>
#include <iostream>
#include <rtc/configuration.hpp>
#include <rtc/peerconnection.hpp>
#include <rtc/websocket.hpp>
#include <memory>
#include <string>
#include <nlohmann/json>

bool ss_is_avail; //Flag for if the signalling server is availiable to send/receive;

int main()
{
    ss_is_avail = false;
    rtc::WebSocket ws;
    
    //bool signalling_server_message_availiable = false;
    
    ws.onOpen([&ws]() {
        std::cout << "WebSocket open" << std::endl;
    });
    
    ws.onMessage([](std::variant<rtc::binary, rtc::string> message) {
        if (std::holds_alternative<rtc::string>(message)) {
            std::string msg = std::get<rtc::string>(message);
            std::cout << "WebSocket received: " << msg << std::endl;
            
        }
    });
    
    ws.onAvailable([&ws]()
    {
        ss_is_avail = true;
//         nlohmann::json ids_and_stuff =
//         {
//         {
//             
//         }
//         };
//         ws.send();
    }
    );
    
    ws.open("127.0.0.1:8000");
    
    // while(!ws.isOpen());
    // ws.send("test");
    ws.close();
    
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


// std::shared_ptr<rtc::PeerConnection> makeAPeerConnection(const rtc::Configuration conf)
// {
//     rtc::PeerConnection test;
//     return test;
// }

