#pragma once

namespace TunaSweeperStove
{
    // Independent of SDK/UE types so authorization can be tested offline.
    inline bool IsOwnedGame(const wchar_t* ExpectedGameId, const wchar_t* GameId,
        unsigned int GameCode, unsigned int OwnershipCode)
    {
        if (!ExpectedGameId || !*ExpectedGameId || !GameId || OwnershipCode != 1
            || (GameCode != 3u && GameCode != 4u))
        {
            return false;
        }
        while (*ExpectedGameId && *ExpectedGameId == *GameId)
        {
            ++ExpectedGameId;
            ++GameId;
        }
        return *ExpectedGameId == *GameId;
    }
}
