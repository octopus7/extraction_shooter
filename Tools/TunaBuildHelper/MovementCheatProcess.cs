using Microsoft.Win32.SafeHandles;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace TunaBuildHelper;

internal sealed record MovementCheatTarget(int ProcessId, DateTime StartTime, string DisplayName)
{
    public override string ToString() => DisplayName;
}

internal sealed class MovementCheatTargetExitedException : InvalidOperationException;

internal static class MovementCheatProcess
{
    private const string ExportName = "TunaSweeperMovementCheatControl";
    private const ulong Magic = 0x54534D4F56454348;
    private const uint EnabledToken = 0x54534D43;

    public static IReadOnlyList<MovementCheatTarget> FindTargets()
    {
        List<MovementCheatTarget> targets = [];
        foreach (Process process in Process.GetProcesses())
        {
            using (process)
            {
                try
                {
                    if (!IsGameProcess(process.ProcessName)) continue;
                    using SafeProcessHandle handle = Open(process.Id, false);
                    FindControl(process, handle);
                    targets.Add(new(process.Id, process.StartTime, $"{process.ProcessName} ({process.Id})"));
                }
                catch (Exception exception) when (exception is Win32Exception or InvalidOperationException or ArgumentException)
                {
                    // Exited, inaccessible, or an older game without the versioned interface.
                }
            }
        }
        return targets;
    }

    public static void SetEnabled(MovementCheatTarget target, bool enabled)
    {
        using Process process = GetSelectedProcess(target);
        using SafeProcessHandle handle = Open(process.Id, true);
        nint control = FindControl(process, handle);
        byte[] token = BitConverter.GetBytes(enabled ? EnabledToken : 0u);
        if (!WriteProcessMemory(handle, control + 16, token, (nuint)token.Length, out nuint written) || written != 4)
            throw new Win32Exception(Marshal.GetLastWin32Error());
        byte[] actual = Read(handle, control + 16, 4);
        if (!actual.SequenceEqual(token)) throw new InvalidOperationException("MovementCheatWriteFailed");
    }

    public static bool IsEnabled(MovementCheatTarget target)
    {
        using Process process = GetSelectedProcess(target);
        using SafeProcessHandle handle = Open(process.Id, false);
        nint control = FindControl(process, handle);
        return BitConverter.ToUInt32(Read(handle, control + 16, 4)) == EnabledToken;
    }

    private static Process GetSelectedProcess(MovementCheatTarget target)
    {
        Process process;
        try
        {
            process = Process.GetProcessById(target.ProcessId);
        }
        catch (ArgumentException)
        {
            throw new MovementCheatTargetExitedException();
        }
        try
        {
            if (process.HasExited || process.StartTime != target.StartTime)
                throw new MovementCheatTargetExitedException();
            if (!IsGameProcess(process.ProcessName))
                throw new InvalidOperationException("MovementCheatTargetChanged");
            return process;
        }
        catch
        {
            process.Dispose();
            throw;
        }
    }

    private static bool IsGameProcess(string name)
    {
        if (name.Equals("UnrealEditor", StringComparison.OrdinalIgnoreCase)) return true;
        string[] targets = ["TunaSweeper", "TunaSweeperDemo", "TunaSweeperNoStore", "TunaSweeperNoStoreDemo", "TunaSweeperStove", "TunaSweeperStoveDemo"];
        return targets.Any(target => name.Equals(target, StringComparison.OrdinalIgnoreCase) ||
            name.Equals(target + "-Win64-Shipping", StringComparison.OrdinalIgnoreCase) ||
            name.Equals(target + "-Win64-Test", StringComparison.OrdinalIgnoreCase));
    }

    private static SafeProcessHandle Open(int processId, bool writable)
    {
        const uint queryAndRead = 0x0400 | 0x0010;
        SafeProcessHandle handle = OpenProcess(queryAndRead | (writable ? 0x0020u | 0x0008u : 0), false, processId);
        if (handle.IsInvalid)
        {
            handle.Dispose();
            throw new Win32Exception(Marshal.GetLastWin32Error());
        }
        return handle;
    }

    private static nint FindControl(Process process, SafeProcessHandle handle)
    {
        foreach (ProcessModule module in process.Modules)
        {
            bool editorModule = module.ModuleName.Equals("UnrealEditor-TunaSweeper.dll", StringComparison.OrdinalIgnoreCase);
            bool gameModule = module.BaseAddress == process.MainModule?.BaseAddress &&
                !process.ProcessName.Equals("UnrealEditor", StringComparison.OrdinalIgnoreCase);
            if (!editorModule && !gameModule) continue;
            nint address = FindExport(handle, module);
            if (address == 0) continue;
            byte[] header = Read(handle, address, 32);
            if (BitConverter.ToUInt64(header, 0) != Magic || BitConverter.ToUInt32(header, 8) != 1 ||
                BitConverter.ToUInt32(header, 12) != 32 || BitConverter.ToUInt32(header, 20) != (uint)process.Id ||
                BitConverter.ToUInt64(header, 24) != 0)
                throw new InvalidOperationException("MovementCheatProtocolMismatch");
            return address;
        }
        throw new InvalidOperationException("MovementCheatInterfaceMissing");
    }

    private static nint FindExport(SafeProcessHandle handle, ProcessModule module)
    {
        byte[] ReadRva(uint rva, int size)
        {
            if ((ulong)rva + (ulong)size > (ulong)module.ModuleMemorySize)
                throw new InvalidOperationException("MovementCheatInvalidImage");
            return Read(handle, module.BaseAddress + (nint)rva, size);
        }

        byte[] dos = ReadRva(0, 64);
        if (BitConverter.ToUInt16(dos) != 0x5A4D) return 0;
        byte[] pe = ReadRva(BitConverter.ToUInt32(dos, 60), 144);
        if (BitConverter.ToUInt32(pe) != 0x4550 || BitConverter.ToUInt16(pe, 24) != 0x20B) return 0;
        uint exportRva = BitConverter.ToUInt32(pe, 136);
        uint exportSize = BitConverter.ToUInt32(pe, 140);
        if (exportRva == 0 || exportSize < 40) return 0;
        byte[] directory = ReadRva(exportRva, 40);
        uint functionCount = BitConverter.ToUInt32(directory, 20);
        uint nameCount = BitConverter.ToUInt32(directory, 24);
        if (nameCount > 1_000_000 || functionCount > 1_000_000)
            throw new InvalidOperationException("MovementCheatInvalidImage");
        byte[] names = ReadRva(BitConverter.ToUInt32(directory, 32), checked((int)nameCount * 4));
        byte[] ordinals = ReadRva(BitConverter.ToUInt32(directory, 36), checked((int)nameCount * 2));
        for (int i = 0; i < nameCount; i++)
        {
            uint nameRva = BitConverter.ToUInt32(names, i * 4);
            byte[] name = ReadRva(nameRva, ExportName.Length + 1);
            if (name[^1] != 0 || Encoding.ASCII.GetString(name, 0, name.Length - 1) != ExportName) continue;
            ushort ordinal = BitConverter.ToUInt16(ordinals, i * 2);
            if (ordinal >= functionCount) throw new InvalidOperationException("MovementCheatInvalidImage");
            uint functionRva = BitConverter.ToUInt32(ReadRva(BitConverter.ToUInt32(directory, 28) + ordinal * 4u, 4));
            if (functionRva >= exportRva && (ulong)functionRva < (ulong)exportRva + exportSize)
                throw new InvalidOperationException("MovementCheatInvalidImage");
            ReadRva(functionRva, 32);
            return module.BaseAddress + (nint)functionRva;
        }
        return 0;
    }

    private static byte[] Read(SafeProcessHandle handle, nint address, int size)
    {
        byte[] result = new byte[size];
        if (size == 0) return result;
        if (!ReadProcessMemory(handle, address, result, (nuint)size, out nuint read) || read != (nuint)size)
            throw new Win32Exception(Marshal.GetLastWin32Error());
        return result;
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern SafeProcessHandle OpenProcess(uint access, bool inheritHandle, int processId);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool ReadProcessMemory(SafeProcessHandle process, nint address, byte[] buffer, nuint size, out nuint read);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool WriteProcessMemory(SafeProcessHandle process, nint address, byte[] buffer, nuint size, out nuint written);
}
