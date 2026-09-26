#include "../../TunaSweeper/Source/TunaSweeper/Private/Platform/TunaSweeperStovePolicy.h"
#include "../../TunaSweeper/Source/TunaSweeper/Private/Platform/TunaSweeperStoveCloudPolicy.h"
#include <cstdio>

int main()
{
    struct Case { const wchar_t* Expected; const wchar_t* Actual; unsigned Game; unsigned Ownership; bool Allowed; };
    const Case Cases[] = {
        {L"demo-game", L"demo-game", 4, 1, true},
        {L"full-game", L"full-game", 3, 1, true},
        {L"demo-game", L"another-game", 4, 1, false},
        {L"full-game", L"full-game", 5, 1, false},
        // Launcher developer mode reports BASIC even for this demo's exact game ID.
        {L"demo-game", L"demo-game", 3, 1, true},
        {L"full-game", L"full-game", 4, 1, true},
        {L"demo-game", L"demo-game", 4, 0, false},
        {L"demo-game", L"demo-game", 4, 2, false},
        {L"demo-game", nullptr, 4, 1, false},
        {nullptr, L"demo-game", 4, 1, false},
        {L"", L"", 4, 1, false},
        {L"demo-game", L"demo-game", 0, 1, false},
        {L"demo-game", L"another-game", 3, 1, false},
    };
    int Failures = 0;
    for (const Case& Test : Cases)
    {
        if (TunaSweeperStove::IsOwnedGame(Test.Expected, Test.Actual, Test.Game, Test.Ownership) != Test.Allowed)
        {
            ++Failures;
        }
    }
    std::printf("STOVE ownership policy: %d cases, %d failures\n", int(sizeof(Cases) / sizeof(Cases[0])), Failures);
    struct CloudCase { const wchar_t* Path; bool Allowed; };
    const CloudCase CloudCases[] = {
        {L"C:\\Users\\Player\\Cloud\\12345", true},
        {L"D:/STOVE/Cloud/12345/", true},
        {L"C:/Users/\ud14c\uc2a4\ud2b8/Cloud", true},
        {L"\\\\server\\share\\Cloud", true},
        {nullptr, false}, {L"", false}, {L"Cloud/12345", false},
        {L"C:Cloud", false}, {L"C:/", false}, {L"/Cloud", false},
        {L"C:/Cloud/../Steam", false}, {L"C:/Cloud/./12345", false},
        {L"($APPDATA_LOCAL)/Cloud/($MEMBER_NO)", false},
        {L"C:/Cloud/($MEMBER_NO)", false}, {L"C:/%USERNAME%/Cloud", false},
        {L"\\\\server", false}, {L"\\\\server\\", false},
        {L"C:/Cloud/*.sav", false}, {L"C:/Cloud\nPlayer", false},
    };
    int CloudFailures = 0;
    for (const CloudCase& Test : CloudCases)
    {
        if (TunaSweeperStove::IsUsableCloudSaveRoot(Test.Path) != Test.Allowed) ++CloudFailures;
    }
    std::printf("STOVE cloud path policy: %d cases, %d failures\n", int(sizeof(CloudCases) / sizeof(CloudCases[0])), CloudFailures);
    Failures += CloudFailures;
    return Failures ? 1 : 0;
}
