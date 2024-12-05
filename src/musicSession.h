// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: Dimosthenis Krallis

#ifndef MUSICSESSION_H
#define MUSICSESSION_H

#include "rtc/rtc.hpp"

#include <nlohmann/json_fwd.hpp>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <rtc/configuration.hpp>
#include <stdexcept>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <atomic>

#define KEYLEN 5


class MusicSession{
    
    private:
        bool wsConnected;
        bool dcConnected;
        bool pcConnected;
        std::atomic<int> lockPlayList;
        std::atomic<int> lockSession;
        std::atomic<bool> inPlaylistWriteMode;
    public:
        MusicSession(nlohmann::json connectionInfo);
        //MusicSession();
        int addSong(nlohmann::json track);
        int addSong(nlohmann::json track, int pos);
        int addSong(nlohmann::json track, int pos, bool force, bool updateSession);
        void removeSong(int playlistPoint);
        std::string getCode();
        int makeConnection(std::string code);

        void endSession();
        nlohmann::json getSessionInfo();
        nlohmann::json getSongInfo();
        std::vector<nlohmann::json> getPlaylist();
        enum playState{
            PLAYING,
            PAUSED,
            NEXT,
            PREVIOUS
        };
        ~MusicSession();
    private:
        nlohmann::json sessionInfo;
        nlohmann::json sessionInfoBuffer;
        rtc::Configuration conf;
        std::shared_ptr<rtc::WebSocket> ws;
        std::weak_ptr<rtc::WebSocket> wws;
        std::string localId;
        std::unordered_map<std::string, bool> lackingPeers;
        std::unordered_map<std::string, std::shared_ptr<rtc::PeerConnection>> peerConnectionMap;
        std::unordered_map<std::string, std::shared_ptr<rtc::DataChannel>> dataChannelMap;
        std::vector<nlohmann::json> playList;
        std::vector<nlohmann::json> playListBuffer;
        std::shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration &config,std::weak_ptr<rtc::WebSocket> wws, std::string id);
        void cleanConnections();
        std::string randid(int size);
        
        
        
        
        //std::string priorityMessage;
        
        int setInfoUpdate(nlohmann::json info);
        int setInfoUpdate(nlohmann::json info, bool setPriority);
        int interperateIncomming(std::string inp, std::string id, std::shared_ptr<rtc::DataChannel> dc);
        int interperateIncomming(std::string inp, std::string id, std::weak_ptr<rtc::DataChannel> wdc);
        int greetPeer(std::shared_ptr<rtc::DataChannel> dc);
        int greetPeer(std::weak_ptr<rtc::DataChannel> dc);
        
        int sendUpdate();
        int getUpdate();
        bool getIfFieldIsInteger(nlohmann::json message, std::string field);
        bool getIfFieldIsString(nlohmann::json message, std::string field);
        
        int safeCheckIntEq(nlohmann::json message,std::string field, int inp);
        int sessionIntegrityCheck(nlohmann::json inp);
        bool checkIfIsSessionPacket(nlohmann::json inp);
        bool checkIfIsSongPacket(nlohmann::json inp);
        
    public:
        int getPlaylistSum();
        int setInfo(nlohmann::json info);
        bool getIfCanBeHeard();
        void waitUntilCanBeHeard();
        int getNumberOfPeers();
        int sendSync();
        int askSync();
        int assertState();
};

#endif