using System;
using System.Runtime.InteropServices;
using System.Text;
using MoonSweepers.Api;

namespace MoonSweepers.Hints;

/// <summary>One stable plugin-owned hint context. Publish on the admitted native
/// Update thread in the same frame as the legend, with a live borrowed walker.</summary>
public sealed class HintsClient : IDisposable
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate int PublishCall(IntPtr key, IntPtr walker, IntPtr controls);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void ClearCall(IntPtr key);
    private static readonly UTF8Encoding Utf8 = new(false, true);
    private readonly PublishCall publish;
    private readonly ClearCall clear;
    private readonly int thread = Environment.CurrentManagedThreadId;
    private IntPtr key, controls;
    private string? text;

    private HintsClient(IntPtr table, string identity)
    {
        publish = Marshal.GetDelegateForFunctionPointer<PublishCall>(Marshal.ReadIntPtr(table, 8));
        clear = Marshal.GetDelegateForFunctionPointer<ClearCall>(Marshal.ReadIntPtr(table, 16));
        byte[] name = Utf8.GetBytes(identity + "\0");
        try
        {
            key = Marshal.AllocHGlobal(name.Length);
            Marshal.Copy(name, 0, key, name.Length);
            controls = Marshal.AllocHGlobal(768);
            Marshal.WriteByte(controls, 0);
        }
        catch { FreeBuffers(); throw; }
    }

    /// <summary>Bind once after native Ready. Each instance owns only owner.slot;
    /// use finitely many stable slots, never per-entity IDs. Clear does not reclaim
    /// the provider's 32 key slots. The caller supplies its actual package owner.</summary>
    public static bool TryBind(RuntimeClient runtime, string owner, string slot, out HintsClient? client)
    {
        string identity = Identity(owner, slot);
        client = null;
        if (!runtime.TryFind("hints-framework", "hints", 1, 24, out IntPtr table)) return false;
        if (Marshal.ReadInt32(table) != 1 || (uint)Marshal.ReadInt32(table, 4) < 24 ||
            Marshal.ReadIntPtr(table, 8) == IntPtr.Zero || Marshal.ReadIntPtr(table, 16) == IntPtr.Zero)
            return false;
        client = new HintsClient(table, identity);
        return true;
    }

    /// <summary>Encode only when controls change. The provider parses its own
    /// key/label grammar; it copies this bounded UTF-8 input before returning.</summary>
    public void SetControls(string value)
    {
        CheckThread();
        if (string.IsNullOrEmpty(value) || value.IndexOf('\0') >= 0)
            throw new ArgumentException("Hint controls must be nonempty and contain no NUL");
        if (value == text) return;
        if (value.Length > 767 || Utf8.GetByteCount(value) > 767)
            throw new ArgumentException("Hint controls exceed 767 UTF-8 bytes");
        int count = 1;
        foreach (char c in value) if (c == '|' || c == '\n') count++;
        if (count > 8) throw new ArgumentException("Hints support at most eight controls");
        byte[] bytes = Utf8.GetBytes(value);
        Marshal.Copy(bytes, 0, controls, bytes.Length);
        Marshal.WriteByte(controls, bytes.Length, 0);
        text = value;
        GC.KeepAlive(this);
    }

    /// <summary>Zero walker or native focus/world/thread gates return false.
    /// No encoding, lookup, callback registration or managed allocation per tick.</summary>
    public bool Publish(IntPtr borrowedWalker)
    {
        CheckThread();
        if (borrowedWalker == IntPtr.Zero) return false;
        if (text == null) throw new InvalidOperationException("Set controls before publishing");
        bool accepted = publish(key, borrowedWalker, controls) != 0;
        GC.KeepAlive(this);
        return accepted;
    }

    public void Clear()
    {
        CheckThread();
        clear(key);
        GC.KeepAlive(this);
    }

    public void Dispose()
    {
        if (key == IntPtr.Zero) return;
        CheckThread();
        clear(key);
        FreeBuffers();
        GC.SuppressFinalize(this);
    }

    // Finalization only frees our buffers. Calling Unity/provider code here
    // would violate the admitted thread; unpublished native contexts expire.
    ~HintsClient() { FreeBuffers(); }
    private void FreeBuffers()
    {
        if (key != IntPtr.Zero) { Marshal.FreeHGlobal(key); key = IntPtr.Zero; }
        if (controls != IntPtr.Zero) { Marshal.FreeHGlobal(controls); controls = IntPtr.Zero; }
    }
    private void CheckThread()
    {
        if (key == IntPtr.Zero) throw new ObjectDisposedException(nameof(HintsClient));
        if (thread != Environment.CurrentManagedThreadId)
            throw new InvalidOperationException("Hints calls require the binding thread");
    }
    private static void ValidatePart(string value, bool letterFirst)
    {
        if (string.IsNullOrEmpty(value)) throw new ArgumentException("Missing hint owner/slot");
        if (letterFirst && !(value[0] >= 'a' && value[0] <= 'z'))
            throw new ArgumentException("Hint slot must begin with a letter");
        bool start = true;
        foreach (char c in value)
        {
            bool letter = c >= 'a' && c <= 'z', digit = c >= '0' && c <= '9';
            if (start && !(letter || digit)) throw new ArgumentException("Invalid hint owner/slot");
            if (letter || digit) start = false;
            else if (c == '-') start = true;
            else throw new ArgumentException("Invalid hint owner/slot");
        }
        if (start) throw new ArgumentException("Invalid hint owner/slot");
    }

    internal static string Identity(string owner, string slot)
    {
        ValidatePart(owner, false);
        ValidatePart(slot, true);
        string identity = owner + "." + slot;
        if (identity.Length > 63) throw new ArgumentException("Hint key exceeds 63 ASCII bytes");
        return identity;
    }
}
