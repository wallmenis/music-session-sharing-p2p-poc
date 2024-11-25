// SPDX-License-Identifier: MPL-2.0


/**
 * libdatachannel client example -> music thesis
 * Copyright (c) 2019-2020 Paul-Louis Ageneau
 * Copyright (c) 2019 Murat Dogan
 * Copyright (c) 2020 Will Munn
 * Copyright (c) 2020 Nico Chatzi
 * Copyright (c) 2020 Lara Mackey
 * Copyright (c) 2020 Erik Cota-Robles
 * Copyright (c) 2024 Dimosthenis Krallis
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "musicSession.h"
#include <iostream>
#include <nlohmann/json_fwd.hpp>

MusicSession::MusicSession(nlohmann::json connectionInfo)
//MusicSession::MusicSession()
{
    auto tmp = connectionInfo.find("ice_server");
    auto iceServer = tmp->get<std::string>();
    tmp = connectionInfo.find("signaling_server");
    auto signalingServer = tmp->get<std::string>();
    sessionInfo = 
    {
        {"playState", MusicSession::playState::PAUSED},
        {"timeStamp", 0.0},
        {"playlistPos", 0},
        {"numberOfSongs", 0},
        {"playlistChkSum", 0},
        {"priority" , "none"}
    };
    // tmp = connectionInfo.find("enabledTURN");
    // auto is_turn = tmp->get<std::string>();
    try {
        
        rtc::InitLogger(rtc::LogLevel::Info);
        
        //rtc::Configuration config;
        
        
        
        localId = randid(5);
        std::cout << "The local ID is " << localId << std::endl;
        
        // std::shared_ptr<rtc::WebSocket> ws = std::make_shared<rtc::WebSocket>();
        // std::weak_ptr<rtc::WebSocket> wws = ws;
        ws = std::make_shared<rtc::WebSocket>();
        wws = ws;
        
        std::promise<void> wsPromise;
        auto wsFuture = wsPromise.get_future();
        
        ws->onOpen([&wsPromise, this]() {
            std::cout << "WebSocket connected, signaling ready" << std::endl;
            wsPromise.set_value();
            wsConnected = true;
        });
        
        ws->onError([&wsPromise, this](std::string s) {
            std::cout << "WebSocket error" << std::endl;
            wsPromise.set_exception(std::make_exception_ptr(std::runtime_error(s)));
            wsConnected = false;
        });
        
        ws->onClosed([this]() { 
            std::cout << "WebSocket closed" << std::endl; 
            wsConnected = false;
        });
        
        
        ws->onMessage([this](auto data) {
            // data holds either std::string or rtc::binary
            std::cout << "got data from signaling server\n";
            if (!std::holds_alternative<std::string>(data))
            {
                std::cout << "got binary from signaling server... exiting...\n";
                return;
            }
                
            std::cout << "got string: " << std::get<std::string>(data) << "\n";
            
            nlohmann::json message = nlohmann::json::parse(std::get<std::string>(data));
            
            std::cout << "-----------here----------\n";
            
            auto it = message.find("id");
            if (it == message.end())
                return;
            
            auto id = it->get<std::string>();
            
            it = message.find("type");
            if (it == message.end())
                return;
            
            auto type = it->get<std::string>();
            
            std::shared_ptr<rtc::PeerConnection> pc;
            if (auto jt = peerConnectionMap.find(id); jt != peerConnectionMap.end()) {
                pc = jt->second;
            } else if (type == "offer") {
                std::cout << "Answering to " + id << std::endl;
                pc = createPeerConnection(conf, wws, id);
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
        finalurl << signalingServer << "/" << localId;
        conf.iceServers.emplace_back(iceServer);
        const std::string url = finalurl.str();
        std::cout << "WebSocket URL is " << url << std::endl;
        ws->open(url);
        
        std::cout << "Waiting for signaling to be connected..." << std::endl;
        wsFuture.get();
        
        /*while (true) {
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
            
            std::weak_ptr<rtc::DataChannel> wdc = dc;
            
            dc->onOpen([id, wdc, this]() {
                std::cout << "DataChannel from " << id << " open" << std::endl;
                if (auto dc = wdc.lock())
                {
                    dc->send("Hello from " + localId);
                }
            });
            
            dc->onClosed([id, this]() {
                std::cout << "DataChannel from " << id << " closed" << std::endl;
                dataChannelMap.erase(id);
            });
            
            dc->onMessage([id, wdc](auto data) {
                // data holds either std::string or rtc::binary
                if (std::holds_alternative<std::string>(data))
                    std::cout << "Message from " << id << " received: " << std::get<std::string>(data)
                    << std::endl;
                else
                    std::cout << "Binary message from " << id
                    << " received, size=" << std::get<rtc::binary>(data).size() << std::endl;
            });
            
            dataChannelMap.emplace(id, dc);
        }*/
        
//         std::cout << "Cleaning up..." << std::endl;
//         
//         dataChannelMap.clear();
//         peerConnectionMap.clear();
        
    } catch (const std::exception &e) {
        std::cout << "Error: " << e.what() << std::endl;
        cleanConnections();
    }
}

MusicSession::~MusicSession()
{
    endSession();
}

void MusicSession::endSession()
{
    if(ws.unique())
    {
        if(ws->isOpen())
        {
            ws->close();
        }
    }
    cleanConnections();
}

void MusicSession::cleanConnections()
{
    std::cout << "Cleaning up..." << std::endl;
    dataChannelMap.clear();
    peerConnectionMap.clear();
}

std::string MusicSession::getCode()
{
    return localId;
}

int MusicSession::makeConnection(std::string code)
{
    
    if (code.empty())
    {
        std::cerr << "Empty Id" << std::endl;
        return -1;
    }
        
    
    if (code == localId) {
        std::cerr << "Invalid code (This is the local ID)" << std::endl;
        return -2;
    }
    
    std::cout << "Offering to " + code<< std::endl;
    auto pc = createPeerConnection(conf, ws, code);
    
    // We are the offerer, so create a data channel to initiate the process
    const std::string label = "GAMER";
    std::cout << "Creating DataChannel with label \"" << label << "\"" << std::endl;
    auto dc = pc->createDataChannel(label);
    
    std::weak_ptr<rtc::DataChannel> wdc = dc;
    
    dc->onOpen([code, wdc, this]() {
        std::cout << "DataChannel from " << code << " open" << std::endl;
        /*if (greetPeer(wdc))
        {
            std::cout << "failed to greet peer...\n";
        }*/
        dcConnected = true;
    });
    
    dc->onClosed([code, this]() {
        std::cout << "DataChannel from " << code << " closed" << std::endl;
    });
    
    dc->onMessage([code, wdc, this](auto data) {
        // data holds either std::string or rtc::binary
        std::cout << "Got from " << code << std::endl;
        if (std::holds_alternative<std::string>(data))
        {
            std::cout << "Message from " << code << " received: " << std::get<std::string>(data) << std::endl;
            if(interperateIncomming(std::get<std::string>(data),code, wdc))
            {
                std::cerr << "failed to interperate message\n";
            }
            
        }
        else
        {
            std::cout << "Binary message from " << code << " received, size=" << std::get<rtc::binary>(data).size() << "Ignoring.." << std::endl;
            
        }
    });
    
    dataChannelMap.emplace(code, dc);
    
    return 0;
}




std::shared_ptr<rtc::PeerConnection> MusicSession::createPeerConnection(const rtc::Configuration &config, std::weak_ptr<rtc::WebSocket> wws, std::string id) {
    auto pc = std::make_shared<rtc::PeerConnection>(config);
    std::cout << "created peer connection\n";
    pc->onStateChange(
        [](rtc::PeerConnection::State state) { std::cout << "State: " << state << std::endl; });
    
    pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state) {
        std::cout << "Gathering State: " << state << std::endl;
    });
    
    pc->onLocalDescription([wws, id](rtc::Description description) {
        std::cout << "sending local description\n";
        nlohmann::json message = {{"id", id},
        {"type", description.typeString()},
        {"description", std::string(description)}};
        if (auto ws = wws.lock())
        {
            ws->send(message.dump());
            std::cout << "sent local description\n";
        }
    });
    
    pc->onLocalCandidate([wws, id](rtc::Candidate candidate) {
        std::cout << "sending local candidate\n";
        nlohmann::json message = {{"id", id},
        {"type", "candidate"},
        {"candidate", std::string(candidate)},
        {"mid", candidate.mid()}};
        if (auto ws = wws.lock())
        {
            ws->send(message.dump());
            std::cout << "sent local candidate\n";
        }
        
    });
    
    pc->onDataChannel([id,this](std::shared_ptr<rtc::DataChannel> dc) {
        std::cout << "DataChannel from " << id << " received with label \"" << dc->label() << "\""<< std::endl;
        std::weak_ptr<rtc::DataChannel> wdc = dc;
        dc->onOpen([wdc, this]() {
            std::cout << "datachannel open\n";
            /*if (greetPeer(wdc))
            {
                std::cout << "failed to greet peer...\n";
            }*/
        });
        
        dc->onClosed([id]() { std::cout << "DataChannel from " << id << " closed" << std::endl; });
        
        dc->onMessage([id,wdc, this](auto data) {
            // data holds either std::string or rtc::binary
            std::cout << "Got from " << id << std::endl;
            if (std::holds_alternative<std::string>(data))
            {
                std::cout << "Message from " << id << " received: " << std::get<std::string>(data) << std::endl;
                if(interperateIncomming(std::get<std::string>(data),id, wdc))
                {
                    std::cerr << "failed to interperate message\n";
                }
            }
            else
            {
                std::cout << "Binary message from " << id << " received, size=" << std::get<rtc::binary>(data).size() << "ignoring..." << std::endl;
            }
        });
        
        dataChannelMap.emplace(id, dc);
    });
    
    peerConnectionMap.emplace(id, pc);
    return pc;
};


int MusicSession::getPlaylistSum()
{
    int i,j;
    int sum=0;
    for (i = 0; i < playList.size(); i++)
    {
        for (j = 0; j < playList[i].dump().size(); j++)
        {
            sum += playList[i].dump().c_str()[j];
        }
    }
    return sum;
}

int MusicSession::interperateIncomming(std::string inp, std::string id, std::weak_ptr<rtc::DataChannel> wdc)
{
    if (std::shared_ptr<rtc::DataChannel> dc = wdc.lock())
    {
        return interperateIncomming(inp,id,dc);
    }
    return 1;
}

int MusicSession::interperateIncomming(std::string inp, std::string id, std::shared_ptr<rtc::DataChannel> dc)
{
    std::cout << "interperating\n";
    nlohmann::json message = nlohmann::json::parse(inp);
    
    int psum = getPlaylistSum();
    if (message.find("playlistChkSum").value().is_number_integer() || message.find("ok").value().is_number_integer())
    {
        if (message.find("playlistChkSum").value().get<int>() != psum || message.find("ok").value().get<int>() != psum)
        {
            nlohmann::json tmp =
            {
                {"badSum", psum}
            };
            dc->send(tmp.dump());
            if(!inPlaylistWriteMode)
                playList.clear();
            inPlaylistWriteMode = true;
        }
        else
        {
            inPlaylistWriteMode = false;
        }
    }
    
    if (message.find("badSum").value().is_number_integer())
    {
        nlohmann::json tmp =
        {
            {"ok", psum}
        };
        if (message.find("badSum").value().get<int>() != psum)
        {
            int i;
            for (i = 0; i < playList.size(); i++)
            {
                if(dc->isOpen())
                    dc->send(playList[i].dump());
            }
        }
        dc->send(tmp.dump());
    }
    
    if(!inPlaylistWriteMode)
    {
        return setInfoUpdate(message);
    }
    else {
        return addSong(message, playList.size());
    }
    
    return 0;
}

int MusicSession::addSong(nlohmann::json track, int pos)
{
    std::string tck = "";
    std::string uri = "";
    std::string hash = "";
    if(track.empty())
        return 1;
    if(track.find("track").value().is_string())
    {
        tck = track.find("track").value().get<std::string>();
    }
    if(track.find("uri").value().is_string())
    {
        uri = track.find("uri").value().get<std::string>();
    }
    if(track.find("hash").value().is_string())
    {
        hash = track.find("hash").value().get<std::string>();
    }
    nlohmann::json templateTrack=
    {
        {"track", tck},
        {"uri", uri},
        {"hash", hash}
    };
    if(inPlaylistWriteMode)
    {
        return 1;
    }
    playList.insert(playList.begin() + pos, templateTrack);
    return 0;
}

int MusicSession::setInfoUpdate(nlohmann::json info)
{
    int returnval = 1;
    bool isDiff = false;
    // sessionInfo = 
    // {
    //     {"playState", MusicSession::playState::PAUSED},
    //     {"timeStamp", 0.0},
    //     {"playlistPos", 0},
    //     {"numberOfSongs", 0},
    //     {"playlistChkSum", 0}
    // };
    std::cout << "setting info update\n";
    MusicSession::playState ps = sessionInfo.find("playState").value().get<MusicSession::playState>();
    float ts = sessionInfo.find("timeStamp").value().get<float>();
    int plp = sessionInfo.find("playlistPos").value().get<int>();
    int nos = sessionInfo.find("numberOfSongs").value().get<int>();
    int plcs = sessionInfo.find("playlistChkSum").value().get<int>();
    int prm = sessionInfo.find("priority").value().get<int>();
    if (!info.empty())
    {
        MusicSession::playState tmpst;
        int tmpint;
        if(info.find("playState").value().is_number_integer())
        {
            tmpst= info.find("playState").value().get<MusicSession::playState>();
            if (ps != tmpst)
            {
                ps = tmpst;
                isDiff = true;
            }
        }
        if(info.find("playlistPos").value().is_number_integer())
        {
            tmpint = info.find("playlistPos").value().get<int>();
            if(tmpint != plp)
            {
                plp = tmpint;
                isDiff = true;
            }
        }
        if(info.find("numberOfSongs").value().is_number_integer())
        {
            tmpint = info.find("numberOfSongs").value().get<int>();
            if (tmpint != nos)
            {
                nos = tmpint;
                isDiff = true;
            }
        }
        if(info.find("playlistChkSum").value().is_number_integer())
        {
            tmpint = info.find("playlistChkSum").value().get<int>();
            if (tmpint != plcs)
            {
                plcs = tmpint;
                isDiff = true;
            }
        }
        if(info.find("timeStamp").value().is_number_float())
        {
            float tmp = info.find("timeStamp").value().get<float>();
            if (tmp - ts + 1.0 > 0.0 || isDiff) // that extra 1 is fot the max latency
            {
                ts = tmp;
            }
        }
        if(info.find("priority").value().is_string())
        {
            std::string tmp = info.find("priority").value().get<std::string>();
            priorityMessage = tmp;
        }
        sessionInfo = 
        {
            {"playState", ps},
            {"timeStamp", ts},
            {"playlistPos", plp},
            {"numberOfSongs", nos},
            {"playlistChkSum", plcs},
            {"priority", priorityMessage}
        };
        returnval = 0;
    }
    return returnval;
}

int MusicSession::greetPeer(std::weak_ptr<rtc::DataChannel> wdc)
{
    if (std::shared_ptr<rtc::DataChannel> dc = wdc.lock())
    {
        return greetPeer(dc);
    }
    return 1;
}

int MusicSession::greetPeer(std::shared_ptr<rtc::DataChannel> dc)
{
    int i;
    // nlohmann::json hello={
    // {"hello", 0}
    // };
    // dc->send(hello.dump());
    dc->send(sessionInfo.dump());
    // for (i = 0; i < playList.size(); i++)
    // {
    //     dc->send(playList[i].dump());
    // }
    return 0;
}

std::string MusicSession::randid(int size)
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

int MusicSession::setInfo(nlohmann::json info)
{
    std::cout << "setting info\n";
    MusicSession::playState ps = info.find("playState").value().get<MusicSession::playState>();
    float ts = info.find("timeStamp").value().get<float>();
    int plp = info.find("playlistPos").value().get<int>();
    int nos = info.find("numberOfSongs").value().get<int>();
    int plcs = info.find("playlistChkSum").value().get<int>();
    std::string prm = info.find("priority").value().get<std::string>();
    nlohmann::json askForUser=
    {
        {"playState", ps},
        {"timeStamp", ts},
        {"playlistPos", plp},
        {"numberOfSongs", nos},
        {"playlistChkSum", plcs},
        {"priority" , localId}
    };
    for (auto conne : dataChannelMap)
    {
        if(conne.second->isOpen())
        {
            conne.second->send(askForUser.dump());
        }
    }
    setInfoUpdate(info);
    return 0;
}


