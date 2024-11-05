#include "musicSession.h"
#include <memory>

int main()
{
    std::unique_ptr<MusicSession> mu = std::make_unique<MusicSession>();
    return 0;
}