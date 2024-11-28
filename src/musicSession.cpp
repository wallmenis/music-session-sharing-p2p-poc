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
        {"timeStamp", 0},
        {"playlistPos", 0},
        {"numberOfSongs", 0},
        {"playlistChkSum", 0},
        {"priority" , "none"}
    };
    lockPlayList = 0;
    lockSession = 0;
    lackingPeers.clear();
    // tmp = connectionInfo.find("enabledTURN");
    // auto is_turn = tmp->get<std::string>();
    wsConnected = false;
    dcConnected = false;
    pcConnected = false;
    try {
        
        rtc::InitLogger(rtc::LogLevel::Info);
        
        //rtc::Configuration config;
        

        
        localId = randid(KEYLEN);
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
            std::cout << std::chrono::system_clock::now().time_since_epoch().count() << "got data from signaling server\n";
            if (!std::holds_alternative<std::string>(data))
            {
                std::cout << "got binary from signaling server... exiting...\n";
                return;
            }
                
            std::cout << " got string from signaling server: " << std::get<std::string>(data) << "\n";
            
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
    lackingPeers.clear();
    playList.clear();
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
            dcConnected = true;
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
    pcConnected = true;
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
    std::cout << std::chrono::system_clock::now().time_since_epoch().count()  << " received from " << id << " : " << message << "\n";
    std::vector<nlohmann::json> plv;
    plv.clear();
    
    int psum = getPlaylistSum();
    int playlistLength = getPlaylist().size();
    // if (message.find("playlistChkSum").value().is_number_integer() 
    //     || message.find("ok").value().is_number_integer() 
    //     || message.find("numberOfSongs").value().is_number_integer())
    /*
    if (getIfFieldIsInteger(message, "playlistChkSum")
        || getIfFieldIsInteger(message, "ok")
        || getIfFieldIsInteger(message, "numberOfSongs"))
    {
        // if (message.find("playlistChkSum").value().get<int>() != psum 
        //     || message.find("ok").value().get<int>() != psum 
        //     || message.find("numberOfSongs").value().get<int>() != playlistLength)
        if (safeCheckIntEq(message, "playlistChkSum", psum) == 1
            || safeCheckIntEq(message, "ok", psum) == 1 
            || safeCheckIntEq(message, "numberOfSongs", playlistLength) == 1)
        {
            nlohmann::json tmp =
            {
                {"badSum", psum}
            };
            std::string tstr = tmp.dump();
            std::cout << std::chrono::system_clock::now().time_since_epoch().count()  << " sending to " << id << " : " << tstr << "\n";
            dc->send(tstr);
            if(!inPlaylistWriteMode || message.find("numberOfSongs").value().get<int>() < playlistLength)
                playList.clear();
            inPlaylistWriteMode = true;
        }
        else
        {
            inPlaylistWriteMode = false;
        }
    }
    */
    
    if (safeCheckIntEq(message, "playlistChkSum", psum) == 1                        // 1 is for not equal
        || safeCheckIntEq(message, "ok", psum) == 1 
        || safeCheckIntEq(message, "numberOfSongs", playlistLength) == 1)
    {
        nlohmann::json tmp =
        {
            {"badSum", psum}
        };
        std::string tstr = tmp.dump();
        //std::cout << std::chrono::system_clock::now().time_since_epoch().count()  << " sending to " << id << " : " << tstr << "\n";
        dc->send(tstr);
        if(!inPlaylistWriteMode)
            playList.clear();
        if(getIfFieldIsInteger(message, "numberOfSongs"))
        {
            if(message.find("numberOfSongs").value().get<int>() < playlistLength)
                playList.clear();
        }
        if(!inPlaylistWriteMode)
            inPlaylistWriteMode = true;
    }
    else
    {
        if(safeCheckIntEq(message, "playlistChkSum", psum)==2 || safeCheckIntEq(message, "ok", psum)==2)
        {
            inPlaylistWriteMode = false;
        }
    }
    
    // if (message.find("badSum").value().is_number_integer())
    if (getIfFieldIsInteger(message, "badSum"))
    {
        if(lackingPeers.find(id) == lackingPeers.end())
        {
            lackingPeers.emplace(id,true);
        // }
        // if(lackingPeers){
            // nlohmann::json tmp =
            // {
            //     {"ok", psum}
            // };
            if (message.find("badSum").value().get<int>() != psum) // || )
            {
                plv = getPlaylist();
                for (nlohmann::json song : plv)
                {
                    if(dc->isOpen())
                    {
                        std::cout << std::chrono::system_clock::now().time_since_epoch().count()  << " sending to " << id << " : " << song.dump() << "\n";
                        dc->send(song.dump());
                    }
                        
                }
                // int i;
                // for (i = 0; i < playList.size(); i++)
                // {
                //     if(dc->isOpen())
                //         dc->send(playList[i].dump());
                // }
            }
            lackingPeers.erase(id);
            //std::cout << std::chrono::system_clock::now().time_since_epoch().count()  << " sending to " << id << " : " << tmp.dump() << "\n";
            //dc->send(tmp.dump());
            dc->send(getSessionInfo().dump());
        }
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
    //if(track.find("track").value().is_string())
    if(getIfFieldIsString(track, "track"))
    {
        tck = track.find("track").value().get<std::string>();
    }
    //if(track.find("uri").value().is_string())
    if(getIfFieldIsString(track, "uri"))
    {
        uri = track.find("uri").value().get<std::string>();
    }
    //if(track.find("hash").value().is_string())
    if(getIfFieldIsString(track, "hash"))
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
    if(lockPlayList!=0)
    {
        return 2;
    }
    lockPlayList++;
    std::cout << "Added " << templateTrack << "\n";
    playList.insert(playList.begin() + pos, templateTrack);
    lockPlayList--;
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
    int ts = sessionInfo.find("timeStamp").value().get<int>();
    int plp = sessionInfo.find("playlistPos").value().get<int>();
    int nos = sessionInfo.find("numberOfSongs").value().get<int>();
    int plcs = sessionInfo.find("playlistChkSum").value().get<int>();
    std::string prm = sessionInfo.find("priority").value().get<std::string>();
    if (!info.empty())
    {
        MusicSession::playState tmpst;
        int tmpint;
        //if(info.find("playState").value().is_number_integer())
        if(getIfFieldIsInteger(info, "playState"))
        {
            tmpst= info.find("playState").value().get<MusicSession::playState>();
            if (ps != tmpst)
            {
                ps = tmpst;
                isDiff = true;
            }
        }
        //if(info.find("playlistPos").value().is_number_integer())
        if(getIfFieldIsInteger(info, "playlistPos"))
        {
            tmpint = info.find("playlistPos").value().get<int>();
            if(tmpint != plp)
            {
                plp = tmpint;
                isDiff = true;
            }
        }
        //if(info.find("numberOfSongs").value().is_number_integer())
        if(getIfFieldIsInteger(info, "numberOfSongs"))
        {
            tmpint = info.find("numberOfSongs").value().get<int>();
            if (tmpint != nos)
            {
                nos = tmpint;
                isDiff = true;
            }
        }
        //if(info.find("playlistChkSum").value().is_number_integer())
        if(getIfFieldIsInteger(info, "playlistChkSum"))
        {
            tmpint = info.find("playlistChkSum").value().get<int>();
            if (tmpint != plcs)
            {
                plcs = tmpint;
                isDiff = true;
            }
        }
        //if(info.find("timeStamp").value().is_number_float())
        if(getIfFieldIsInteger(info, "timeStamp"))
        {
            int tmp = info.find("timeStamp").value().get<int>();
            if (tmp - ts + 1000 > 0 || isDiff) // that extra 1 is fot the max latency
            {
                ts = tmp;
            }
        }
        //if(info.find("priority").value().is_string())
        if(getIfFieldIsString(info, "priority"))
        {
            std::string tmp = info.find("priority").value().get<std::string>();
            priorityMessage = tmp;
        }
        std::cout << "session lock : " << lockSession << "\n";
        std::cout << "playlist lock : " << lockPlayList << "\n";
        if(lockSession==0)
        {
            lockSession++;
            sessionInfo = 
            {
                {"playState", ps},
                {"timeStamp", ts},
                {"playlistPos", plp},
                {"numberOfSongs", nos},
                {"playlistChkSum", plcs},
                {"priority", priorityMessage}
            };
            lockSession--;
            returnval = 0;
        }
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
    int ts = info.find("timeStamp").value().get<int>();
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
    if(inPlaylistWriteMode)
    {
        return 3;
    }
    for (auto conne : dataChannelMap)
    {
        if(conne.second->isOpen())
        {
            //std::cout << std::chrono::system_clock::now().time_since_epoch().count()  << " sending to " << conne.first << " : " << askForUser.dump() << "\n";
            conne.second->send(askForUser.dump());
        }
    }
    return setInfoUpdate(info);
}

nlohmann::json MusicSession::getSessionInfo()
{
    nlohmann::json returnVal;
    if(lockSession == 0)
    {
        lockSession++;
        sessionInfoBuffer = nlohmann::json(sessionInfo);
        lockSession--;
    }
    returnVal = nlohmann::json(sessionInfoBuffer);
    return returnVal;
}

std::vector<nlohmann::json> MusicSession::getPlaylist()
{
    std::vector<nlohmann::json> returnVal;
    if(lockPlayList == 0)
    {
        lockPlayList++;
        playListBuffer.assign(playList.begin(), playList.end());
        lockPlayList--;
    }
    returnVal.assign(playListBuffer.begin(), playListBuffer.end());
    return returnVal;
}

bool MusicSession::getIfFieldIsInteger(nlohmann::json message, std::string field)
{
    nlohmann::json msg = nlohmann::json(message);
    //std::cout << "input: " << msg << " | " << field <<"\n";
    if(msg.empty()) //just realized i could use case... oh well... now i wrote it...
    {
        return false;
    }
    //std::cout << "not empty ";
    auto it = msg.find(field);
    //std::cout << "got iterator " << " ";
    // if(it != msg.end())
    // {}else{
    if(it == msg.end())
    {
        //std::cout << "int doesn't exist" << std::endl;
        return false;
    }
    //std::cout << "exists ";
    if(!msg.find(field).value().is_number_integer())
    {
        //std::cout << "is not int" << std::endl;
        return false;
    }
    //std::cout << "is int\n";
    return true;
}

bool MusicSession::getIfFieldIsString(nlohmann::json message, std::string field)
{
    nlohmann::json msg = nlohmann::json(message);
    //std::cout << "input: " << msg << " | " << field <<"\n";
    if(msg.empty())
    {
        return false;
    }
    //std::cout << "not empty ";
    auto it = msg.find(field);
    //std::cout << "got iterator " << " ";
    // if(it != msg.end())
    // {}else{
    if(it == msg.end())
    {
        //std::cout << "string doesn't exist" << std::endl;
        return false;
    }
    //std::cout << "exists ";
    if(!msg.find(field).value().is_string())
    {
        std::cout << "is not string" << std::endl;
        return false;
    }
    //std::cout << "is string\n";
    return true;
}

int MusicSession::safeCheckIntEq(nlohmann::json message,std::string field, int inp)
{
    nlohmann::json msg = nlohmann::json(message);
    //std::cout << "input: " << msg << " | " << field << " | " << inp <<"\n";
    if(!getIfFieldIsInteger(msg, field))
    {
        //std::cout << "input: " << msg << " | " << field << " | " << inp <<" | returned 0\n";
        return 0;
    }
    if(msg.find(field).value().get<int>() != inp)
    {
        //std::cout << "input: " << msg << " | " << field << " | " << inp <<" | returned 1\n";
        return 1;
    }
    //std::cout << "input: " << msg << " | " << field << " | " << inp <<" | returned 2\n";
    return 2;
}

int MusicSession::getNumberOfPeers()
{
    return peerConnectionMap.size();
}

bool MusicSession::getIfCanBeHeard()
{
    return wsConnected && pcConnected && dcConnected;
}

void MusicSession::waitUntilCanBeHeard()
{
    while(!getIfCanBeHeard());
}