using System.Windows;
using System.Windows.Controls;

namespace TunaBuildHelper;

public partial class WorldProgressWindow : Window
{
    private readonly MovementCheatTarget target;
    private readonly CancellationTokenSource lifetime = new();
    private GateProgressSnapshot? snapshot;
    private bool busy, closed;
    private long contextRevision;
    private sealed record DisplayRow(GateProgressRow Row, string State);
    internal WorldProgressWindow(MovementCheatTarget target)
    {
        this.target = target;
        InitializeComponent();
        TargetText.Text = string.Format(Text("GateTarget"), target.DisplayName);
        Loaded += (_, _) => _ = MonitorAsync();
    }
    private string Text(string key) => (string)FindResource(key);
    private async Task MonitorAsync()
    {
        try
        {
            while (!lifetime.IsCancellationRequested)
            {
                if (!busy)
                {
                    try
                    {
                        var next = await Task.Run(() => GateProgressProcess.Read(target), lifetime.Token);
                        if (closed) return;
                        if (!busy) Apply(next);
                    }
                    catch (Exception e) when (e is not OperationCanceledException)
                    {
                        if (closed) return;
                        snapshot = null; contextRevision++; GateList.ItemsSource = null;
                        ContextText.Text = Text("GateUnavailable");
                        StatusText.Text = Text("GateUnsupported"); UpdateControls();
                    }
                }
                await Task.Delay(700, lifetime.Token);
            }
        }
        catch (OperationCanceledException) { }
    }
    private void Apply(GateProgressSnapshot next)
    {
        bool changed = snapshot?.Context != next.Context || snapshot?.Map != next.Map || snapshot?.Slot != next.Slot || snapshot?.State != next.State;
        var selected = (GateList.SelectedItem as DisplayRow)?.Row;
        if (changed) { contextRevision++; selected = null; }
        bool rowsChanged = snapshot == null || !snapshot.Rows.SequenceEqual(next.Rows);
        snapshot = next;
        if (changed || rowsChanged)
        {
            var rows = next.Rows.Select(r => new DisplayRow(r, Text(!r.CanReset ? "GateInvalid" : (r.Flags & 1) != 0 ? "GateUnlocked" : "GateLocked"))).ToArray();
            GateList.ItemsSource = rows;
            GateList.SelectedItem = rows.FirstOrDefault(r => r.Row == selected);
        }
        ContextText.Text = next.Ready ? string.Format(Text("GateContext"), next.Map, next.Slot) : Text("GateUnavailable");
        if (!next.Ready || changed || StatusText.Text == Text("GateReading"))
            StatusText.Text = Text(next.State == 3 ? "GateDisabled" : !next.Ready ? "GateUnavailable" : next.Rows.Count == 0 ? "GateEmpty" : "GateReady");
        UpdateControls();
    }
    private void UpdateControls()
    {
        bool ready = !busy && !closed && snapshot?.Ready == true;
        GateList.IsEnabled = ready;
        ResetButton.IsEnabled = ready && GateList.SelectedItem is DisplayRow { Row.CanReset: true };
    }
    private void GateList_SelectionChanged(object sender, SelectionChangedEventArgs e) { if (ResetButton != null) UpdateControls(); }
    private async void Reset_Click(object sender, RoutedEventArgs e)
    {
        if (busy || snapshot?.Ready != true || GateList.SelectedItem is not DisplayRow selected || !selected.Row.CanReset) return;
        var expected = snapshot;
        long revision = contextRevision;
        busy = true; UpdateControls();
        try
        {
            if (MessageBox.Show(this, string.Format(Text("GateConfirm"), selected.Row.GateId, expected.Map, expected.Slot), Text("GateTitle"),
                MessageBoxButton.YesNo, MessageBoxImage.Warning, MessageBoxResult.No) != MessageBoxResult.Yes) return;
            if (closed || revision != contextRevision) return;
            StatusText.Text = Text("GateWorking");
            uint result = await Task.Run(() => GateProgressProcess.Reset(target, expected, selected.Row, lifetime.Token), lifetime.Token);
            if (!closed) StatusText.Text = Text(result == 2 ? "GateSuccess" : result == 3 ? "GateDisabled" : result is 6 ? "GateTimeout" : result is 7 or 8 or 9 ? "GateStale" : "GateFailure");
        }
        catch (OperationCanceledException) { }
        catch (Exception ex)
        {
            if (!closed) StatusText.Text = Text(ex is InvalidOperationException && TryFindResource(ex.Message) is string ? ex.Message : "GateUnsupported");
        }
        finally { busy = false; UpdateControls(); }
    }
    protected override void OnClosed(EventArgs e) { closed = true; lifetime.Cancel(); base.OnClosed(e); }
}
