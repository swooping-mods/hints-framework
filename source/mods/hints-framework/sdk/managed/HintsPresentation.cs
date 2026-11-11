using System;
using System.Runtime.InteropServices;
using MoonSweepers.Api;

namespace MoonSweepers.Hints;

/// <summary>A provider-admitted callback immediately before native legend
/// composition. Active subscriptions root themselves until explicit Dispose;
/// this prevents native code calling a collected delegate. No hot unload.</summary>
public sealed class HintsPresentation : IDisposable
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate uint SubscribeCall(IntPtr key, IntPtr callback, IntPtr user);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate int UnsubscribeCall(uint handle);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)] private delegate void PrepareCall(IntPtr walker, IntPtr user);
    private readonly SubscribeCall subscribe;
    private readonly UnsubscribeCall unsubscribe;
    private readonly PrepareCall prepare;
    private readonly Action<IntPtr> action;
    private readonly IntPtr callback;
    private readonly int thread = Environment.CurrentManagedThreadId;
    private IntPtr key;
    private uint handle;
    private GCHandle root;
    private bool invoking, enabled = true;

    private HintsPresentation(IntPtr table, string identity, Action<IntPtr> action)
    {
        subscribe = Marshal.GetDelegateForFunctionPointer<SubscribeCall>(Marshal.ReadIntPtr(table, 8));
        unsubscribe = Marshal.GetDelegateForFunctionPointer<UnsubscribeCall>(Marshal.ReadIntPtr(table, 16));
        this.action = action;
        prepare = Invoke;
        callback = Marshal.GetFunctionPointerForDelegate(prepare);
        key = Marshal.StringToCoTaskMemUTF8(identity);
    }

    public bool IsSubscribed => handle != 0;
    public bool IsEnabled => enabled;
    public Exception? LastError { get; private set; }

    /// <summary>Bind on the Unity component thread after native Ready. This does
    /// not subscribe until TrySubscribe admits that thread. Keep one object for
    /// retries, rather than creating an object every Update.</summary>
    public static bool TryBind(RuntimeClient runtime, string owner, string slot,
                               Action<IntPtr> action, out HintsPresentation? client)
    {
        if (action == null) throw new ArgumentNullException(nameof(action));
        string identity = HintsClient.Identity(owner, slot);
        client = null;
        if (!runtime.TryFind("hints-framework", "presentation", 1, 24, out IntPtr table)) return false;
        if (Marshal.ReadInt32(table) != 1 || (uint)Marshal.ReadInt32(table, 4) < 24 ||
            Marshal.ReadIntPtr(table, 8) == IntPtr.Zero || Marshal.ReadIntPtr(table, 16) == IntPtr.Zero)
            return false;
        client = new HintsPresentation(table, identity, action);
        return true;
    }

    /// <summary>False until Hints has observed the Unity thread, or when the
    /// key/capacity is unavailable. Native callbacks never run inside subscribe.
    /// Successful registration is resident until Dispose on this thread.</summary>
    public bool TrySubscribe()
    {
        CheckThread();
        if (handle != 0) return true;
        GCHandle pending = GCHandle.Alloc(this);
        try { handle = subscribe(key, callback, IntPtr.Zero); }
        catch { pending.Free(); throw; }
        if (handle == 0) { pending.Free(); return false; }
        root = pending;
        return true;
    }

    public void SetEnabled(bool value)
    {
        CheckThread();
        enabled = value;
    }

    private void Invoke(IntPtr walker, IntPtr unused)
    {
        if (!enabled || key == IntPtr.Zero) return;
        invoking = true;
        try
        {
            CheckThread();
            if (walker != IntPtr.Zero) action(walker);
        }
        catch (Exception error)
        {
            // Exceptions must not cross the unmanaged callback boundary.
            LastError = error;
            enabled = false;
        }
        finally { invoking = false; GC.KeepAlive(this); }
    }

    public void Dispose()
    {
        if (key == IntPtr.Zero) return;
        CheckThread();
        if (invoking) throw new InvalidOperationException("Dispose outside the presentation callback");
        if (handle != 0 && unsubscribe(handle) == 0)
            throw new InvalidOperationException("Native unsubscribe refused; subscription remains rooted");
        handle = 0;
        if (root.IsAllocated) root.Free();
        FreeKey();
        GC.SuppressFinalize(this);
    }

    // An active self-root prevents finalization. Unsubscribed abandoned objects
    // may free their own key here; no Unity or provider call is made.
    ~HintsPresentation() { FreeKey(); }
    private void FreeKey()
    {
        if (key != IntPtr.Zero) { Marshal.FreeCoTaskMem(key); key = IntPtr.Zero; }
    }
    private void CheckThread()
    {
        if (key == IntPtr.Zero) throw new ObjectDisposedException(nameof(HintsPresentation));
        if (thread != Environment.CurrentManagedThreadId)
            throw new InvalidOperationException("Hints presentation requires the binding thread");
    }
}
