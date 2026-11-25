using Microsoft.Win32.SafeHandles;
using System.Text;

namespace TunaBuildHelper;

internal sealed record GateProgressRow(ulong Key, uint Flags, string GateId, string Level)
{
    public bool CanReset => Key != 0 && (Flags & ~1u) == 0;
}
internal sealed record GateProgressSnapshot(ulong Ack, ulong Context, long Published, uint Result, uint State, int Slot, string Map, IReadOnlyList<GateProgressRow> Rows, long TotalKills = -1)
{
    public bool Ready => State == 0 && Context != 0 && Slot is >= 1 and <= 3 && !string.IsNullOrEmpty(Map) && Math.Abs(DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() - Published) < 2500;
}
internal static class GateProgressProcess
{
    internal const int Size = 25864;
    private static nint Locate(System.Diagnostics.Process process, SafeProcessHandle handle, bool requireTuna = false, bool requireItem = false, bool requireKills = false)
    {
        nint address = MovementCheatProcess.FindInterface(process, handle, "TunaSweeperGateCheatControl", requireKills ? Size + 8 : Size);
        byte[] h = MovementCheatProcess.Read(handle, address, 24);
        uint version = BitConverter.ToUInt32(h, 8);
        if (BitConverter.ToUInt64(h) != 0x5453474154454348 || version is < 1 or > 4 ||
            BitConverter.ToUInt32(h, 12) != (version == 4 ? Size + 8 : Size) || BitConverter.ToUInt32(h, 16) != process.Id || BitConverter.ToUInt32(h, 20) != 0)
            throw new InvalidOperationException("GateUnsupported");
        if (requireKills && version < 4) throw new InvalidOperationException("GateUnsupported");
        if (requireTuna && version < 2) throw new InvalidOperationException("TunaUnsupported");
        if (requireItem && version < 3) throw new InvalidOperationException("ui.helper_item_grant.unsupported");
        return address;
    }
    public static GateProgressSnapshot ReadTuna(MovementCheatTarget target)
    {
        using var process = MovementCheatProcess.GetSelectedProcess(target);
        using var handle = MovementCheatProcess.Open(process.Id, false);
        var snapshot = ReadSnapshot(handle, Locate(process, handle, requireTuna: true));
        if (snapshot.State == 3) throw new InvalidOperationException("TunaDisabled");
        if (!snapshot.Ready) throw new InvalidOperationException("TunaUnavailable");
        return snapshot;
    }
    public static GateProgressSnapshot Read(MovementCheatTarget target)
    {
        using var process = MovementCheatProcess.GetSelectedProcess(target);
        using var handle = MovementCheatProcess.Open(process.Id, false);
        return ReadSnapshot(handle, Locate(process, handle));
    }
    public static GateProgressSnapshot ReadItemGrant(MovementCheatTarget target)
    {
        using var process = MovementCheatProcess.GetSelectedProcess(target);
        using var handle = MovementCheatProcess.Open(process.Id, false);
        return ReadSnapshot(handle, Locate(process, handle, requireItem: true));
    }
    public static GateProgressSnapshot ReadKills(MovementCheatTarget target)
    {
        using var process = MovementCheatProcess.GetSelectedProcess(target);
        using var handle = MovementCheatProcess.Open(process.Id, false);
        return ReadSnapshot(handle, Locate(process, handle, requireKills: true), true);
    }

    private static GateProgressSnapshot ReadSnapshot(SafeProcessHandle handle, nint address, bool readKills = false)
    {
        for (int attempt = 0; attempt < 8; attempt++)
        {
            byte[] b = MovementCheatProcess.Read(handle, address + 88, Size - 88 + (readKills ? 8 : 0));
            ulong revision = BitConverter.ToUInt64(b);
            if (revision == 0 || (revision & 1) != 0 || revision != BitConverter.ToUInt64(MovementCheatProcess.Read(handle, address + 88, 8))) continue;
            uint count = BitConverter.ToUInt32(b, 44);
            if (count > 64) throw new InvalidOperationException("GateUnsupported");
            string Text(int offset, int length)
            {
                int end = Array.IndexOf(b, (byte)0, offset, length);
                if (end < 0) throw new InvalidOperationException("GateUnsupported");
                return new UTF8Encoding(false, true).GetString(b, offset, end - offset);
            }
            List<GateProgressRow> rows = [];
            for (int i = 0; i < count; i++)
            {
                int o = 176 + i * 400;
                rows.Add(new(BitConverter.ToUInt64(b, o), BitConverter.ToUInt32(b, o + 8), Text(o + 16, 128), Text(o + 144, 256)));
            }
            return new(BitConverter.ToUInt64(b, 8), BitConverter.ToUInt64(b, 16), BitConverter.ToInt64(b, 24),
                BitConverter.ToUInt32(b, 32), BitConverter.ToUInt32(b, 36), BitConverter.ToInt32(b, 40), Text(48, 128), rows,
                readKills ? BitConverter.ToInt64(b, Size - 88) : -1);
        }
        throw new InvalidOperationException("GateUnavailable");
    }
    // Mutex ownership stays on this worker thread across publication and acknowledgement.
    // Status polling never writes the mailbox; only an explicitly confirmed action calls this method.
    public static uint Reset(MovementCheatTarget target, GateProgressSnapshot expected, GateProgressRow row, CancellationToken token)
        => Publish(target, expected, row, 2, token);

    public static uint FillTuna(MovementCheatTarget target, GateProgressSnapshot expected, CancellationToken token)
        => Publish(target, expected, null, 3, token);

    public static uint GrantItem(MovementCheatTarget target, GateProgressSnapshot expected, int itemId, int quantity, CancellationToken token)
    {
        if (itemId <= 0 || quantity is < 1 or > 1000000) throw new ArgumentOutOfRangeException(nameof(quantity));
        return Publish(target, expected, null, 4, token, ((ulong)(uint)quantity << 32) | (uint)itemId);
    }

    public static uint SaveKills(MovementCheatTarget target, GateProgressSnapshot expected, long count)
        => Publish(target, expected, null, 5, CancellationToken.None, checked((ulong)count));

    private static uint Publish(MovementCheatTarget target, GateProgressSnapshot expected, GateProgressRow? row, uint operation, CancellationToken token, ulong payload = 0)
    {
        using var mutex = new Mutex(false, $@"Local\TunaGateReset_{target.ProcessId}_{target.StartTime.ToUniversalTime().Ticks}");
        bool acquired = false;
        try
        {
            try { acquired = mutex.WaitOne(0); } catch (AbandonedMutexException) { acquired = true; }
            if (!acquired) throw new InvalidOperationException("GateBusy");
            token.ThrowIfCancellationRequested();
            using var process = MovementCheatProcess.GetSelectedProcess(target);
            using var handle = MovementCheatProcess.Open(process.Id, true);
            nint address = Locate(process, handle, requireTuna: operation == 3, requireItem: operation == 4, requireKills: operation == 5);
            var current = ReadSnapshot(handle, address);
            if (operation == 3 && current.State == 3) throw new InvalidOperationException("TunaDisabled");
            if (!current.Ready || current.Context != expected.Context || current.Map != expected.Map || current.Slot != expected.Slot ||
                (operation == 2 && !current.Rows.Any(r => r == row && r.CanReset))) throw new InvalidOperationException("GateStale");
            byte[] previous = MovementCheatProcess.Read(handle, address + 24, 64);
            ulong oldSequence = BitConverter.ToUInt64(previous);
            if (oldSequence != 0 && oldSequence != current.Ack && BitConverter.ToInt64(previous, 8) >= DateTimeOffset.UtcNow.ToUnixTimeMilliseconds())
                throw new InvalidOperationException("GateBusy");
            ulong sequence = checked(Math.Max(Math.Max(oldSequence, current.Ack), (ulong)DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() * 1000) + 1);
            long deadline = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() + 3000;
            byte[] body = new byte[56];
            BitConverter.GetBytes(deadline).CopyTo(body, 0);
            BitConverter.GetBytes(expected.Context).CopyTo(body, 8);
            BitConverter.GetBytes(operation is 4 or 5 ? payload : row?.Key ?? 0UL).CopyTo(body, 16);
            BitConverter.GetBytes(operation).CopyTo(body, 24);
            BitConverter.GetBytes(target.ProcessId).CopyTo(body, 28);
            if (operation == 5) BitConverter.GetBytes(expected.TotalKills).CopyTo(body, 32);
            token.ThrowIfCancellationRequested();
            try
            {
                MovementCheatProcess.Write(handle, address + 24, new byte[8]);
                MovementCheatProcess.Write(handle, address + 32, body);
                token.ThrowIfCancellationRequested();
                MovementCheatProcess.Write(handle, address + 24, BitConverter.GetBytes(sequence));
                while (DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() <= deadline)
                {
                    if (token.WaitHandle.WaitOne(40)) throw new OperationCanceledException(token);
                    var response = ReadSnapshot(handle, address);
                    if (response.Ack == sequence) return response.Result;
                }
                throw new InvalidOperationException("GateTimeout");
            }
            finally
            {
                // Still owning the interprocess mutex: do not leave a canceled request queued.
                // A transaction already accepted by the game thread cannot be undone here.
                try
                {
                    MovementCheatProcess.Write(handle, address + 32, new byte[8]);
                    MovementCheatProcess.Write(handle, address + 24, new byte[8]);
                }
                catch (System.ComponentModel.Win32Exception) { }
            }
        }
        finally { if (acquired) mutex.ReleaseMutex(); }
    }
}
