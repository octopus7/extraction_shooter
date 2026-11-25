using System.ComponentModel;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;

namespace TunaBuildHelper;

public partial class KillCountWindow : Window
{
    private readonly MovementCheatTarget target;
    private GateProgressSnapshot? snapshot;
    private bool saving;

    internal KillCountWindow(MovementCheatTarget target)
    {
        this.target = target;
        foreach (var key in new[] { "ui.helper_kills.title", "ui.helper_kills.label", "ui.quest_cheat.fix", "ui.coop.close" })
            Resources[key] = ItemGrantCatalog.Text(key);
        InitializeComponent();
        TargetText.Text = target.DisplayName;
        Loaded += async (_, _) => await LoadCount();
    }

    private async Task LoadCount()
    {
        try
        {
            var current = await Task.Run(() => GateProgressProcess.ReadKills(target));
            if (!IsVisible) return;
            if (!current.Ready || current.TotalKills < 0)
            {
                StatusText.Text = ItemGrantCatalog.Text(current.State == 3 ? "ui.helper_item_grant.disabled" : "ui.helper_item_grant.waiting");
                return;
            }
            snapshot = current;
            CountBox.IsEnabled = true;
            CountBox.Text = current.TotalKills.ToString(CultureInfo.InvariantCulture);
            CountBox.Focus();
            CountBox.SelectAll();
        }
        catch (Exception) { StatusText.Text = ItemGrantCatalog.Text("ui.helper_kills.unavailable"); }
    }

    internal static bool TryParseCount(string text, out long value) =>
        long.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out value) && value >= 0;

    private void InputChanged(object sender, TextChangedEventArgs e)
    {
        if (SaveButton != null)
            SaveButton.IsEnabled = !saving && snapshot != null && TryParseCount(CountBox.Text, out var count) && count != snapshot.TotalKills;
    }

    private async void Save_Click(object sender, RoutedEventArgs e)
    {
        if (saving || snapshot is not { } expected || !TryParseCount(CountBox.Text, out long count)) return;
        saving = true;
        SaveButton.IsEnabled = CloseButton.IsEnabled = CountBox.IsEnabled = false;
        bool saved = false;
        try
        {
            uint result = await Task.Run(() => GateProgressProcess.SaveKills(target, expected, count));
            saved = result == 17;
        }
        catch (Exception) { /* Refresh the snapshot before permitting another edit. */ }
        finally { saving = false; CloseButton.IsEnabled = true; }
        if (saved) { DialogResult = true; return; }
        snapshot = null;
        CountBox.Clear();
        StatusText.Text = ItemGrantCatalog.Text("ui.helper_kills.retry");
        await LoadCount();
    }

    protected override void OnClosing(CancelEventArgs e)
    {
        base.OnClosing(e);
        if (saving) e.Cancel = true;
    }
}
