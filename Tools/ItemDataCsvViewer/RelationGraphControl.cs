using System.Drawing.Drawing2D;

namespace ItemDataCsvViewer;

internal sealed class RelationGraphControl : Control
{
	private readonly Font titleFont = new(FontFamily.GenericSansSerif, 9.5f, FontStyle.Bold);
	private readonly Font subtitleFont = new(FontFamily.GenericSansSerif, 8.0f, FontStyle.Regular);
	private readonly Font edgeFont = new(FontFamily.GenericSansSerif, 7.5f, FontStyle.Regular);
	private RelationGraph graph = RelationGraph.Empty;

	public RelationGraphControl()
	{
		DoubleBuffered = true;
		BackColor = Color.FromArgb(22, 24, 28);
	}

	public void SetGraph(RelationGraph nextGraph)
	{
		graph = nextGraph;
		Invalidate();
	}

	protected override void OnPaint(PaintEventArgs e)
	{
		base.OnPaint(e);
		Graphics graphics = e.Graphics;
		graphics.SmoothingMode = SmoothingMode.AntiAlias;
		graphics.Clear(BackColor);

		if (graph.Nodes.Count <= 0)
		{
			DrawEmptyState(graphics);
			return;
		}

		Dictionary<string, RectangleF> rectanglesByNodeId = LayoutNodes(ClientRectangle);
		DrawEdges(graphics, rectanglesByNodeId);
		DrawNodes(graphics, rectanglesByNodeId);
		DrawLegend(graphics);
	}

	private Dictionary<string, RectangleF> LayoutNodes(Rectangle bounds)
	{
		Dictionary<string, RectangleF> rectangles = [];
		float nodeWidth = Math.Clamp(bounds.Width * 0.18f, 160.0f, 230.0f);
		float nodeHeight = 58.0f;
		float centerX = bounds.Left + bounds.Width * 0.5f;
		float centerY = bounds.Top + bounds.Height * 0.48f;
		rectangles[graph.CenterNodeId] = new RectangleF(centerX - nodeWidth * 0.5f, centerY - nodeHeight * 0.5f, nodeWidth, nodeHeight);

		List<RelationGraphNode> outerNodes = graph.Nodes.Where(node => node.Id != graph.CenterNodeId).ToList();
		if (outerNodes.Count <= 0)
		{
			return rectangles;
		}

		float radiusX = Math.Max(220.0f, bounds.Width * 0.34f);
		float radiusY = Math.Max(170.0f, bounds.Height * 0.34f);
		for (int index = 0; index < outerNodes.Count; ++index)
		{
			double angle = -Math.PI / 2.0 + index * (Math.PI * 2.0 / outerNodes.Count);
			float x = centerX + (float)Math.Cos(angle) * radiusX;
			float y = centerY + (float)Math.Sin(angle) * radiusY;
			x = Math.Clamp(x, bounds.Left + nodeWidth * 0.55f, bounds.Right - nodeWidth * 0.55f);
			y = Math.Clamp(y, bounds.Top + nodeHeight * 0.9f, bounds.Bottom - nodeHeight * 0.9f);
			rectangles[outerNodes[index].Id] = new RectangleF(x - nodeWidth * 0.5f, y - nodeHeight * 0.5f, nodeWidth, nodeHeight);
		}

		return rectangles;
	}

	private void DrawEdges(Graphics graphics, IReadOnlyDictionary<string, RectangleF> rectanglesByNodeId)
	{
		using Pen edgePen = new(Color.FromArgb(118, 134, 150), 1.5f);
		using SolidBrush labelBackgroundBrush = new(Color.FromArgb(210, 22, 24, 28));
		using SolidBrush labelBrush = new(Color.FromArgb(220, 230, 235, 240));
		using StringFormat labelFormat = new()
		{
			Alignment = StringAlignment.Center,
			LineAlignment = StringAlignment.Center,
			Trimming = StringTrimming.EllipsisCharacter
		};

		foreach (RelationGraphEdge edge in graph.Edges)
		{
			if (!rectanglesByNodeId.TryGetValue(edge.FromId, out RectangleF fromRectangle) ||
				!rectanglesByNodeId.TryGetValue(edge.ToId, out RectangleF toRectangle))
			{
				continue;
			}

			PointF from = Center(fromRectangle);
			PointF to = Center(toRectangle);
			graphics.DrawLine(edgePen, from, to);
			PointF middle = new((from.X + to.X) * 0.5f, (from.Y + to.Y) * 0.5f);
			SizeF labelSize = graphics.MeasureString(edge.Label, edgeFont, 140);
			RectangleF labelRectangle = new(middle.X - labelSize.Width * 0.5f - 4.0f, middle.Y - labelSize.Height * 0.5f - 2.0f, labelSize.Width + 8.0f, labelSize.Height + 4.0f);
			graphics.FillRectangle(labelBackgroundBrush, labelRectangle);
			graphics.DrawString(edge.Label, edgeFont, labelBrush, labelRectangle, labelFormat);
		}
	}

	private void DrawNodes(Graphics graphics, IReadOnlyDictionary<string, RectangleF> rectanglesByNodeId)
	{
		using Pen borderPen = new(Color.FromArgb(82, 96, 112), 1.0f);
		using StringFormat titleFormat = new()
		{
			Alignment = StringAlignment.Center,
			LineAlignment = StringAlignment.Center,
			Trimming = StringTrimming.EllipsisCharacter
		};
		using StringFormat subtitleFormat = new()
		{
			Alignment = StringAlignment.Center,
			LineAlignment = StringAlignment.Center,
			Trimming = StringTrimming.EllipsisCharacter
		};

		foreach (RelationGraphNode node in graph.Nodes)
		{
			if (!rectanglesByNodeId.TryGetValue(node.Id, out RectangleF rectangle))
			{
				continue;
			}

			Color fillColor = NodeFillColor(node);
			using SolidBrush fillBrush = new(fillColor);
			using SolidBrush titleBrush = new(Color.FromArgb(245, 248, 251));
			using SolidBrush subtitleBrush = new(Color.FromArgb(198, 207, 216));
			graphics.FillRectangle(fillBrush, rectangle);
			graphics.DrawRectangle(borderPen, rectangle.X, rectangle.Y, rectangle.Width, rectangle.Height);

			RectangleF titleRectangle = new(rectangle.Left + 8.0f, rectangle.Top + 7.0f, rectangle.Width - 16.0f, 23.0f);
			RectangleF subtitleRectangle = new(rectangle.Left + 8.0f, rectangle.Top + 31.0f, rectangle.Width - 16.0f, 20.0f);
			graphics.DrawString(node.Title, titleFont, titleBrush, titleRectangle, titleFormat);
			graphics.DrawString(node.Subtitle, subtitleFont, subtitleBrush, subtitleRectangle, subtitleFormat);
		}
	}

	private void DrawLegend(Graphics graphics)
	{
		string text = "Select an item row to redraw the relation graph.";
		using SolidBrush brush = new(Color.FromArgb(185, 195, 205));
		graphics.DrawString(text, subtitleFont, brush, new PointF(12.0f, 10.0f));
	}

	private void DrawEmptyState(Graphics graphics)
	{
		using SolidBrush brush = new(Color.FromArgb(190, 200, 210));
		using StringFormat format = new()
		{
			Alignment = StringAlignment.Center,
			LineAlignment = StringAlignment.Center
		};
		graphics.DrawString("No item selected.", titleFont, brush, ClientRectangle, format);
	}

	private static Color NodeFillColor(RelationGraphNode node)
	{
		return node.Group switch
		{
			"Item" => Color.FromArgb(48, 82, 118),
			"Tag" => Color.FromArgb(60, 76, 72),
			"Shop" => Color.FromArgb(86, 70, 42),
			"Loot" => Color.FromArgb(80, 58, 76),
			"Craft" => Color.FromArgb(52, 78, 92),
			"Dismantle" => Color.FromArgb(90, 58, 54),
			_ => Color.FromArgb(52, 58, 66)
		};
	}

	private static PointF Center(RectangleF rectangle)
	{
		return new PointF(rectangle.Left + rectangle.Width * 0.5f, rectangle.Top + rectangle.Height * 0.5f);
	}

	protected override void Dispose(bool disposing)
	{
		if (disposing)
		{
			titleFont.Dispose();
			subtitleFont.Dispose();
			edgeFont.Dispose();
		}

		base.Dispose(disposing);
	}
}
