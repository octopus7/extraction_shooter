#include "../../TunaSweeper/Source/TunaSweeper/Private/Platform/TunaSweeperStovePolicy.h"
#include <cstdio>

int main()
{
    struct Case { const wchar_t* Expected; const wchar_t* Actual; unsigned Game; unsigned Ownership; bool Demo; bool Allowed; };
    const Case Cases[] = {
        {L"demo-game", L"demo-game", 4, 1, true, true},
        {L"full-game", L"full-game", 3, 1, false, true},
        {L"demo-game", L"another-game", 4, 1, true, false},
        {L"full-game", L"full-game", 5, 1, false, false},
        {L"demo-game", L"demo-game", 3, 1, true, false},
        {L"full-game", L"full-game", 4, 1, false, false},
        {L"demo-game", L"demo-game", 4, 0, true, false},
        {L"demo-game", L"demo-game", 4, 2, true, false},
        {L"demo-game", nullptr, 4, 1, true, false},
        {nullptr, L"demo-game", 4, 1, true, false},
        {L"", L"", 4, 1, true, false},
    };
    int Failures = 0;
    for (const Case& Test : Cases)
    {
        if (TunaSweeperStove::IsOwnedGame(Test.Expected, Test.Actual, Test.Game, Test.Ownership, Test.Demo) != Test.Allowed)
        {
            ++Failures;
        }
    }
    std::printf("STOVE ownership policy: %d cases, %d failures\n", int(sizeof(Cases) / sizeof(Cases[0])), Failures);
    return Failures ? 1 : 0;
}
