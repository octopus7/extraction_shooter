using System.Windows;
using System.Windows.Controls;
using TunaBuildHelper;

internal static class Program
{
    [STAThread]
    private static int Main()
    {
        void Check(bool condition, string message) { if (!condition) throw new Exception(message); }
        Check(ItemGrantCatalog.Featured.Count == 41, "Featured item count");
        Check(ItemGrantCatalog.Featured[0].Id == 6019, "Rocket first");
        Check(ItemGrantCatalog.Items.All(i => !i.Name.StartsWith("item.")), "Resolved item names");
        var window = new ItemGrantWindow(new(123, DateTime.Now, "synthetic test target"));
        var id = (TextBox)window.FindName("ItemIdBox");
        var quantity = (TextBox)window.FindName("QuantityBox");
        var list = (ListBox)window.FindName("FeaturedList");
        Check(id.Text == "6019" && quantity.Text == "1", "Defaults");
        Check(Grid.GetColumn((UIElement)list.Parent) == 2, "List in right column");
        Check(ScrollViewer.GetVerticalScrollBarVisibility(list) == ScrollBarVisibility.Auto, "Vertical scrolling");
        list.SelectedItem = ItemGrantCatalog.Featured.First(i => i.Id == 6033);
        Check(id.Text == "6033" && quantity.Text == "1", "Selection fills ID and retains quantity");
        Check(!((Button)window.FindName("GrantButton")).IsEnabled, "No grant without live context");
        Check(window.Title == ItemGrantCatalog.Text("ui.item_grant.title"), "Localized window title");
        window.Close();
        Console.WriteLine("Item grant helper tests passed (catalog, layout, selection, defaults, context guard, localization).");
        return 0;
    }
}
