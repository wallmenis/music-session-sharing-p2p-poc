#include "rtc/rtc.hpp"


int main()
{
    rtc::Configuration config;
    config.iceServers.emplace_back("stun.l.google.com:19302");
    
    rtc::PeerConnection pc(config);
    
    pc.onLocalDescription([](rtc::Description sdp) {
        // Send the SDP to the remote peer
        MY_SEND_DESCRIPTION_TO_REMOTE(std::string(sdp));
    });
    
    pc.onLocalCandidate([](rtc::Candidate candidate) {
        // Send the candidate to the remote peer
        MY_SEND_CANDIDATE_TO_REMOTE(candidate.candidate(), candidate.mid());
    });
    
    MY_ON_RECV_DESCRIPTION_FROM_REMOTE([&pc](std::string sdp) {
        pc.setRemoteDescription(rtc::Description(sdp));
    });
    
    MY_ON_RECV_CANDIDATE_FROM_REMOTE([&pc](std::string candidate, std::string mid) {
        pc.addRemoteCandidate(rtc::Candidate(candidate, mid));
    });
    
    pc.onStateChange([](rtc::PeerConnection::State state) {
        std::cout << "State: " << state << std::endl;
    });
    
    pc.onGatheringStateChange([](rtc::PeerConnection::GatheringState state) {
        std::cout << "Gathering state: " << state << std::endl;
    });
    
    auto dc = pc.createDataChannel("test");
    
    dc->onOpen([]() {
        std::cout << "Open" << std::endl;
    });
    
    dc->onMessage([](std::variant<rtc::binary, rtc::string> message) {
        if (std::holds_alternative<rtc::string>(message)) {
            std::cout << "Received: " << get<rtc::string>(message) << std::endl;
        }
    });
    
    // std::shared_ptr<rtc::DataChannel> dc;
    // pc.onDataChannel([&dc](std::shared_ptr<rtc::DataChannel> incoming) {
    //     dc = incoming;
    //     dc->send("Hello world!");
    // });
    
    return 0;
}


