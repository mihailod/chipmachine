#pragma once

extern "C" {
    int fapCrunch(int argc, char* argv[]);

    int getBufferSize();
    int getPlayTimeInNops();
    int getRegisterCountToPlay();
    bool isR12Constant();
}