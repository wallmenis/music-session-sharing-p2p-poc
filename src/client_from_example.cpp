/**
 * libdatachannel client example
 * Copyright (c) 2019-2020 Paul-Louis Ageneau
 * Copyright (c) 2019 Murat Dogan
 * Copyright (c) 2020 Will Munn
 * Copyright (c) 2020 Nico Chatzi
 * Copyright (c) 2020 Lara Mackey
 * Copyright (c) 2020 Erik Cota-Robles
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "rtc/rtc.hpp"


#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <nlohmann/json.hpp>

using namespace std::chrono_literals;
using std::shared_ptr;
using std::weak_ptr;
template <class T> weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }


std::string localId;
std::unordered_map<std::string, shared_ptr<rtc::PeerConnection>> peerConnectionMap;
std::unordered_map<std::string, shared_ptr<rtc::DataChannel>> dataChannelMap;

shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration &config,weak_ptr<rtc::WebSocket> wws, std::string id);
std::string randid(int size);

int main(int argc, char **argv) try {
    
    rtc::InitLogger(rtc::LogLevel::Info);
    
    rtc::Configuration config;
    std::string stunServer = "";
    
    localId = randid(5);
    std::cout << "The local ID is " << localId << std::endl;
    
    auto ws = std::make_shared<rtc::WebSocket>();
    
    std::promise<void> wsPromise;
    auto wsFuture = wsPromise.get_future();
    
    ws->onOpen([&wsPromise]() {
        std::cout << "WebSocket connected, signaling ready" << std::endl;
        wsPromise.set_value();
    });
    
    ws->onError([&wsPromise](std::string s) {
        std::cout << "WebSocket error" << std::endl;
        wsPromise.set_exception(std::make_exception_ptr(std::runtime_error(s)));
    });
    
    ws->onClosed([]() { std::cout << "WebSocket closed" << std::endl; });
    
    ws->onMessage([&config, wws = make_weak_ptr(ws)](auto data) {
        // data holds either std::string or rtc::binary
        if (!std::holds_alternative<std::string>(data))
            return;
        
        nlohmann::json message = nlohmann::json::parse(std::get<std::string>(data));
        
        auto it = message.find("id");
        if (it == message.end())
            return;
        
        auto id = it->get<std::string>();
        
        it = message.find("type");
        if (it == message.end())
            return;
        
        auto type = it->get<std::string>();
        
        shared_ptr<rtc::PeerConnection> pc;
        if (auto jt = peerConnectionMap.find(id); jt != peerConnectionMap.end()) {
            pc = jt->second;
        } else if (type == "offer") {
            std::cout << "Answering to " + id << std::endl;
            pc = createPeerConnection(config, wws, id);
        } else {
            return;
        }
        
        if (type == "offer" || type == "answer") {
            auto sdp = message["description"].get<std::string>();
            pc->setRemoteDescription(rtc::Description(sdp, type));
        } else if (type == "candidate") {
            auto sdp = message["candidate"].get<std::string>();
            auto mid = message["mid"].get<std::string>();
            pc->addRemoteCandidate(rtc::Candidate(sdp, mid));
        }
    });
    
    std::stringstream finalurl;
    finalurl << "127.0.0.1:8000/" << localId;
    config.iceServers.emplace_back("127.0.0.1:3479");
    const std::string url = finalurl.str();
    std::cout << "WebSocket URL is " << url << std::endl;
    ws->open(url);
    
    std::cout << "Waiting for signaling to be connected..." << std::endl;
    wsFuture.get();
    
    while (true) {
        std::string id;
        std::cout << "Enter a remote ID to send an offer:" << std::endl;
        std::cin >> id;
        std::cin.ignore();
        
        if (id.empty())
            break;
        
        if (id == localId) {
            std::cout << "Invalid remote ID (This is the local ID)" << std::endl;
            continue;
        }
        
        std::cout << "Offering to " + id << std::endl;
        auto pc = createPeerConnection(config, ws, id);
        
        // We are the offerer, so create a data channel to initiate the process
        const std::string label = "test";
        std::cout << "Creating DataChannel with label \"" << label << "\"" << std::endl;
        auto dc = pc->createDataChannel(label);
        
        dc->onOpen([id, wdc = make_weak_ptr(dc)]() {
            std::cout << "DataChannel from " << id << " open" << std::endl;
            if (auto dc = wdc.lock())
                dc->send("Hello from " + localId);
        });
        
        dc->onClosed([id]() { std::cout << "DataChannel from " << id << " closed" << std::endl; });
        
        dc->onMessage([id, wdc = make_weak_ptr(dc)](auto data) {
            // data holds either std::string or rtc::binary
            if (std::holds_alternative<std::string>(data))
                std::cout << "Message from " << id << " received: " << std::get<std::string>(data)
                << std::endl;
            else
                std::cout << "Binary message from " << id
                << " received, size=" << std::get<rtc::binary>(data).size() << std::endl;
        });
        
        dataChannelMap.emplace(id, dc);
    }
    
    std::cout << "Cleaning up..." << std::endl;
    
    dataChannelMap.clear();
    peerConnectionMap.clear();
    return 0;
    
} catch (const std::exception &e) {
    std::cout << "Error: " << e.what() << std::endl;
    dataChannelMap.clear();
    peerConnectionMap.clear();
    return -1;
}

// Create and setup a PeerConnection
shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration &config,
                                                     weak_ptr<rtc::WebSocket> wws, std::string id) {
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
    
    pc->onDataChannel([id](shared_ptr<rtc::DataChannel> dc) {
        std::cout << "DataChannel from " << id << " received with label \"" << dc->label() << "\""
        << std::endl;
        
        dc->onOpen([wdc = make_weak_ptr(dc)]() {
            if (auto dc = wdc.lock())
                dc->send("Hello from " + localId);
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
        
        dataChannelMap.emplace(id, dc);
    });
    
    peerConnectionMap.emplace(id, pc);
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
                                                     