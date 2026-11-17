using System.Globalization;
using System.Windows;
using System.Windows.Controls;

namespace TunaBuildHelper;

public partial class ItemGrantWindow : Window
{
    private readonly MovementCheatTarget target;
    private readonly CancellationTokenSource lifetime = new();
    private GateProgressSnapshot? snapshot;
    private bool busy, closed;
    private static string Text(string key) => ItemGrantCatalog.Text(key);
    internal ItemGrantWindow(MovementCheatTarget target)
    {
        this.target = target;
        foreach (var key in new[] { "ui.item_grant.title", "ui.item_grant.id", "ui.item_grant.quantity", "ui.item_grant.grant", "ui.item_grant.picker", "ui.helper_item_grant.scope" }) Resources[key] = Text(key);
        InitializeComponent();
        FeaturedList.ItemsSource = ItemGrantCatalog.Featured;
        TargetText.Text = target.DisplayName;
        StatusText.Text = Text("ui.helper_item_grant.waiting");
        UpdateControls();
        Loaded += (_, _) => _ = MonitorAsync();
    }
    private bool Input(out int id, out int quantity)
    {
        int.TryParse(ItemIdBox?.Text, NumberStyles.None, CultureInfo.InvariantCulture, out id);
        int.TryParse(QuantityBox?.Text, NumberStyles.None, CultureInfo.InvariantCulture, out quantity);
        int selectedId = id;
        return quantity is >= 1 and <= 1000000 && ItemGrantCatalog.Items.Any(i => i.Id == selectedId);
    }
    private void UpdateControls()
    {
        if (GrantButton == null) return;
        bool valid = Input(out int id, out _);
        ItemNameText.Text = ItemGrantCatalog.Items.FirstOrDefault(i => i.Id == id)?.Name ?? Text("ui.item_grant.unknown");
        GrantButton.IsEnabled = !busy && !closed && snapshot?.Ready == true && valid;
        ItemIdBox.IsEnabled = QuantityBox.IsEnabled = FeaturedList.IsEnabled = !busy;
    }
    private void InputChanged(object sender, TextChangedEventArgs e) => UpdateControls();
    private void Featured_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (FeaturedList.SelectedItem is GrantItem item) ItemIdBox.Text = item.Id.ToString(CultureInfo.InvariantCulture);
    }
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
                        var next = await Task.Run(() => GateProgressProcess.ReadItemGrant(target), lifetime.Token);
                        if (closed) return;
                        if (!busy)
                        {
                            bool changed = snapshot?.Context != next.Context || snapshot?.State != next.State;
                            snapshot = next;
                            TargetText.Text = string.Format(Text("ui.helper_item_grant.target"), target.DisplayName, next.Map, next.Slot);
                            if (changed || !next.Ready) StatusText.Text = Text(next.State == 3 ? "ui.helper_item_grant.disabled" : next.Ready ? "ui.helper_item_grant.ready" : "ui.helper_item_grant.waiting");
                        }
                    }
                    catch (Exception ex) when (ex is not OperationCanceledException)
                    {
                        if (closed) return;
                        snapshot = null; StatusText.Text = Text("ui.helper_item_grant.unsupported");
                    }
                    UpdateControls();
                }
                await Task.Delay(700, lifetime.Token);
            }
        }
        catch (OperationCanceledException) { }
    }
    private async void Grant_Click(object sender, RoutedEventArgs e)
    {
        if (busy || snapshot?.Ready != true || !Input(out int id, out int quantity)) return;
        var expected = snapshot;
        busy = true; UpdateControls(); StatusText.Text = Text("ui.helper_item_grant.working");
        try
        {
            uint result = await Task.Run(() => GateProgressProcess.GrantItem(target, expected, id, quantity, lifetime.Token), lifetime.Token);
            if (closed) return;
            StatusText.Text = result == 15 ? string.Format(Text("ui.item_grant.success"), ItemGrantCatalog.Items.First(i => i.Id == id).Name, quantity)
                : Text(result == 16 ? "ui.item_grant.full" : result == 3 ? "ui.helper_item_grant.disabled" : result is 6 or 7 ? "ui.helper_item_grant.stale" : "ui.item_grant.failed");
        }
        catch (OperationCanceledException) { }
        catch (Exception ex)
        {
            if (!closed) StatusText.Text = Text(ex.Message == "ui.helper_item_grant.unsupported" ? ex.Message : ex.Message == "GateTimeout" ? "ui.helper_item_grant.timeout" : ex.Message == "GateStale" ? "ui.helper_item_grant.stale" : "ui.item_grant.failed");
        }
        finally { busy = false; if (!closed) UpdateControls(); }
    }
    protected override void OnClosed(EventArgs e) { closed = true; lifetime.Cancel(); base.OnClosed(e); }
}
