#pragma once

#include <string_view>

namespace TunaSweeperStove
{
    // SDK-independent validation: the launcher must return a resolved absolute directory.
    inline bool IsUsableCloudSaveRoot(const wchar_t* Path)
    {
        if (!Path) return false;
        const std::wstring_view Value(Path);
        const auto IsSeparator = [](wchar_t C) { return C == L'/' || C == L'\\'; };
        const bool bDrive = Value.size() > 3
            && ((Value[0] >= L'A' && Value[0] <= L'Z') || (Value[0] >= L'a' && Value[0] <= L'z'))
            && Value[1] == L':' && IsSeparator(Value[2]);
        const bool bUNC = Value.size() > 4 && IsSeparator(Value[0]) && IsSeparator(Value[1]);
        if (!bDrive && !bUNC) return false;
        const size_t Start = bDrive ? 3 : 2;
        unsigned int Components = 0;
        size_t ComponentStart = Start;
        for (size_t Index = Start; Index <= Value.size(); ++Index)
        {
            if (Index != Value.size() && !IsSeparator(Value[Index]))
            {
                const wchar_t C = Value[Index];
                if (C < 32 || C == L':' || C == L'"' || C == L'<' || C == L'>'
                    || C == L'|' || C == L'?' || C == L'*' || C == L'$' || C == L'%') return false;
                continue;
            }
            const std::wstring_view Component = Value.substr(ComponentStart, Index - ComponentStart);
            if (Component == L"." || Component == L"..") return false;
            if (!Component.empty()) ++Components;
            else if (Index != Value.size()) return false;
            ComponentStart = Index + 1;
        }
        return Components >= (bUNC ? 2u : 1u);
    }
}
