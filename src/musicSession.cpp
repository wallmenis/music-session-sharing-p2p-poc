#include "musicSession.h"

MusicSession::MusicSession(nlohmann::json connectionInfo)
{
    auto iceServer =connectionInfo.find("ice");
    auto signalingServer = connectionInfo.find("signaling");
    connectionInfo.find("enabledTURN");
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