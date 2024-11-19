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

#endif


class MusicSession{
    private:
        nlohmann::json sessionInfo;
        rtc::Configuration conf;
        std::shared_ptr<rtc::WebSocket> ws;
        std::string localId;
        std::unordered_map<std::string, std::shared_ptr<rtc::PeerConnection>> peerConnectionMap;
        std::unordered_map<std::string, std::shared_ptr<rtc::DataChannel>> dataChannelMap;
        std::vector<nlohmann::json> playList;
        std::shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration &config,std::weak_ptr<rtc::WebSocket> wws, std::string id);
        void cleanConnections();
        std::string randid(int size);
        
        bool inPlaylistWriteMode;
        
        
        int interperateIncomming(std::string inp, std::string id, std::shared_ptr<rtc::DataChannel> dc);
        int interperateIncomming(std::string inp, std::string id, std::weak_ptr<rtc::DataChannel> wdc);
        int greetPeer(std::shared_ptr<rtc::DataChannel> dc);
        int greetPeer(std::weak_ptr<rtc::DataChannel> dc);
        int getPlaylistSum();
    public:
        MusicSession(nlohmann::json connectionInfo);
        //MusicSession();
        void addSong(nlohmann::json songInfo, int playlistPoint);
        void removeSong(int playlistPoint);
        std::string getCode();
        int makeConnection(std::string code);
        int sendUpdate();
        void setTimestamp();
        int getUpdate();
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
};