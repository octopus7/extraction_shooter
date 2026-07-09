namespace ItemDataCsvViewer;

internal sealed class RelationGraph
{
	public RelationGraph(string centerNodeId, IReadOnlyList<RelationGraphNode> nodes, IReadOnlyList<RelationGraphEdge> edges)
	{
		CenterNodeId = centerNodeId;
		Nodes = nodes;
		Edges = edges;
	}

	public string CenterNodeId { get; }

	public IReadOnlyList<RelationGraphNode> Nodes { get; }

	public IReadOnlyList<RelationGraphEdge> Edges { get; }

	public static RelationGraph Empty { get; } = new(string.Empty, Array.Empty<RelationGraphNode>(), Array.Empty<RelationGraphEdge>());
}

internal sealed class RelationGraphNode
{
	public RelationGraphNode(string id, string title, string subtitle, string group)
	{
		Id = id;
		Title = title;
		Subtitle = subtitle;
		Group = group;
	}

	public string Id { get; }

	public string Title { get; }

	public string Subtitle { get; }

	public string Group { get; }
}

internal sealed class RelationGraphEdge
{
	public RelationGraphEdge(string fromId, string toId, string label)
	{
		FromId = fromId;
		ToId = toId;
		Label = label;
	}

	public string FromId { get; }

	public string ToId { get; }

	public string Label { get; }
}
