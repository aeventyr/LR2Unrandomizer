
#include <map>
#include <vector>
#include <winbase.h>

#include "logger.h"
#include "lr2.h"

extern std::map<int, int> seedmap;
extern int toIdx(std::vector<int>&);

const char ConfigPath[] = ".\\LR2Unrandomizer.ini";
const unsigned int BufLen = 10;

using namespace LR2;

// This function verifies the input in field Lane
bool isValidLane(char* buf)
{
    if (strlen(buf) == 7)
    {
        unsigned int mask = 0;
        for (int i = 0; i < 7; i++)
        {
            if (!(buf[i] >= '0' && buf[i] <= '9'))
                return false;
            mask |= 1 << (buf[i] - '0');
        }
        if (mask == 0b1111111)
            return true;
        if (mask == 0b11111110)
        {
            for (int i = 0; i < 7; i++)
                buf[i] -= 1;
            return true;
        }
    }
    return false;
}

bool setRandLane(std::vector<int>& lane)
{
    // calculate the mapping from vector lane to int 
    int seqid = toIdx(lane);
    int seed;
    // configuration valid
    if (seqid != -1 && seedmap.find(seqid) != seedmap.end())
    {
        std::vector<int> lane2;
        lane2.reserve(7);
        seed = seedmap[seqid];
        console_log("seed = %d\n", seed);
        *playRandseed = seed;

        // convert to lr2 lane configuration
        for (int i = 0; i < 7; i++)
            lane2[lane[i]] = i;

        playRandLanes[0] = 0;
        for (int i = 0; i < 7; i++)
            playRandLanes[i + 1] = lane2[i] + 1;

        if (*replayStatus == 1)
            for (int i = 0; i < *replayIdx; i++)
                if ((*replayData)[i].type == 200)
                {
                    (*replayData)[i].value = seed;
                    break;
                }
        return true;
    }
    else
    {
        return false;
    }
}

// This function is hooked into the executable
void onSetupLanePerm()
{
    if (*replayStatus == 2)
        return;

    char buf[BufLen];
    GetPrivateProfileString("Config", "Use Unrandomizer", "true", buf, BufLen - 1, ConfigPath);
    _strlwr_s(buf);
    if (!strcmp(buf, "true") || !strcmp(buf, "on") || !strcmp(buf, "yes"))
    {
        if (*randomType1p == LANE_RANDOM && *entryKeyCount == 7)
        {
            GetPrivateProfileString("Config", "Lane", "", buf, BufLen - 1, ConfigPath);
            console_log("Configuration input: %s\n", buf);
            if (isValidLane(buf))
            {
                std::vector<int> lane;
                lane.reserve(7);
                for (int i = 0; i < 7; i++)
                    lane.push_back(buf[i] - '0');

                bool result = setRandLane(lane);
                if (!result)
                    console_log("Configuration %s cannot be generated\n", buf);
            }
        }
    }
    else
    {
        GetPrivateProfileString("Config", "Use R-Random", "false", buf, BufLen - 1, ConfigPath);
        _strlwr_s(buf);
        if (!strcmp(buf, "true") || !strcmp(buf, "on") || !strcmp(buf, "yes"))
        {
            if (*randomType1p == LANE_RANDOM && *entryKeyCount == 7)
            {
                int rranConf = (*playRandseed) % 12;
                console_log("Using R-Random configuration %d\n", rranConf);
                std::vector<int> lane;
                lane.reserve(7);

                for (int i = 0; i < 7; i++)
                    lane.push_back((rranConf & 1) ? i : 7 - 1 - i);
                rranConf >>= 1;
                rranConf += 1;
                int temp;
                for (int i = 0; i < rranConf; i++)
                {
                    temp = lane[7 - 1];
                    for (int j = 5; j >= 0; j--)
                        lane[j + 1] = lane[j];
                    lane[0] = temp;
                }

                bool result = setRandLane(lane);
                if (!result)
                    console_log("Configuration cannot be generated\n");
            }
        }
    }

}
