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
        std::string randid(int size);
    public:
        MusicSession(nlohmann::json connectionInfo);
        //MusicSession();
        void addSong(nlohmann::json songInfo, int playlistPoint);
        void removeSong(int playlistPoint);
        int sendUpdate();
        void setTimestamp();
        int getUpdate();
        nlohmann::json getSessionInfo();
        nlohmann::json getSongInfo();
        std::vector<nlohmann::json> getPlaylist();
        enum playState{
            PLAYING,
            PAUSED,
            NEXT,
            PREVIOUS
        };
};