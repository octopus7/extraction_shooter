using System.Diagnostics;
using System.Drawing.Drawing2D;
using System.Numerics;

namespace CombatMovementSimulator;

internal sealed class SimulatorForm : Form
{
	private const int PanelWidth = 300;
	private readonly HashSet<Keys> keysDown = [];
	private readonly Stopwatch stopwatch = Stopwatch.StartNew();
	private readonly System.Windows.Forms.Timer timer = new();
	private readonly Button startButton = new();
	private readonly SimulationWorld world;
	private Vector2 mouseWorld;
	private Point mouseScreen;
	private bool fireHeld;
	private bool isSimulationRunning;
	private bool paused;
	private bool showDebug = true;
	private BuildMode buildMode = BuildMode.None;
	private float previewRotationRadians;
	private Vector2 previewSize = new(430.0f, 120.0f);
	private float renderScale;

	public SimulatorForm()
	{
		Text = "TunaSweeper Combat Movement Simulator";
		ClientSize = new Size(1320, 820);
		MinimumSize = new Size(1024, 680);
		DoubleBuffered = true;
		KeyPreview = true;
		BackColor = Color.FromArgb(18, 20, 23);

		CombatTuning tuning = CombatTuning.LoadOrDefault(AppContext.BaseDirectory);
		renderScale = tuning.World.SimulationScale;
		world = new SimulationWorld(tuning);
		InitializePanelControls();

		timer.Interval = 16;
		timer.Tick += HandleTick;
		timer.Start();
	}

	protected override void OnKeyDown(KeyEventArgs e)
	{
		base.OnKeyDown(e);
		keysDown.Add(e.KeyCode);

		switch (e.KeyCode)
		{
			case Keys.R:
				ResetToEditMode();
				break;
			case Keys.P:
				if (isSimulationRunning)
				{
					paused = !paused;
				}
				break;
			case Keys.T:
				showDebug = !showDebug;
				break;
			case Keys.F:
				if (isSimulationRunning)
				{
					world.FreezeEnemies = !world.FreezeEnemies;
				}
				break;
			case Keys.B:
				if (!isSimulationRunning)
				{
					buildMode = buildMode switch
					{
						BuildMode.None => BuildMode.Destructible,
						BuildMode.Destructible => BuildMode.Indestructible,
						BuildMode.Indestructible => BuildMode.MeleeEnemy,
						BuildMode.MeleeEnemy => BuildMode.RangedEnemy,
						BuildMode.RangedEnemy => BuildMode.FlankerEnemy,
						_ => BuildMode.None
					};
				}
				break;
			case Keys.D1:
				if (!isSimulationRunning)
				{
					buildMode = BuildMode.MeleeEnemy;
				}
				break;
			case Keys.D2:
				if (!isSimulationRunning)
				{
					buildMode = BuildMode.RangedEnemy;
				}
				break;
			case Keys.D3:
				if (!isSimulationRunning)
				{
					buildMode = BuildMode.FlankerEnemy;
				}
				break;
			case Keys.D4:
				if (!isSimulationRunning)
				{
					buildMode = BuildMode.Destructible;
				}
				break;
			case Keys.D5:
				if (!isSimulationRunning)
				{
					buildMode = BuildMode.Indestructible;
				}
				break;
			case Keys.Delete:
				if (!isSimulationRunning)
				{
					world.RemoveObstacleAt(mouseWorld);
				}
				break;
			case Keys.Escape:
				if (!isSimulationRunning)
				{
					buildMode = BuildMode.None;
				}
				break;
			case Keys.OemOpenBrackets:
				if (!isSimulationRunning)
				{
					previewSize.Y = MathF.Max(60.0f, previewSize.Y - 20.0f);
				}
				break;
			case Keys.OemCloseBrackets:
				if (!isSimulationRunning)
				{
					previewSize.Y += 20.0f;
				}
				break;
			case Keys.OemMinus:
				if (!isSimulationRunning)
				{
					previewSize.X = MathF.Max(80.0f, previewSize.X - 30.0f);
				}
				break;
			case Keys.Oemplus:
				if (!isSimulationRunning)
				{
					previewSize.X += 30.0f;
				}
				break;
		}
	}

	protected override void OnKeyUp(KeyEventArgs e)
	{
		base.OnKeyUp(e);
		keysDown.Remove(e.KeyCode);
	}

	protected override void OnMouseDown(MouseEventArgs e)
	{
		base.OnMouseDown(e);
		if (e.Button == MouseButtons.Left && isSimulationRunning)
		{
			fireHeld = true;
		}
		else if (e.Button == MouseButtons.Right && !isSimulationRunning && buildMode != BuildMode.None)
		{
			PlaceCurrentPreview();
		}
	}

	protected override void OnMouseUp(MouseEventArgs e)
	{
		base.OnMouseUp(e);
		if (e.Button == MouseButtons.Left)
		{
			fireHeld = false;
		}
	}

	protected override void OnMouseMove(MouseEventArgs e)
	{
		base.OnMouseMove(e);
		mouseScreen = e.Location;
		mouseWorld = ScreenToWorld(e.Location);
	}

	protected override void OnMouseWheel(MouseEventArgs e)
	{
		base.OnMouseWheel(e);
		if (!isSimulationRunning && buildMode != BuildMode.None)
		{
			previewRotationRadians += e.Delta > 0 ? MathF.PI / 36.0f : -MathF.PI / 36.0f;
		}
		else
		{
			renderScale = Math.Clamp(renderScale + (e.Delta > 0 ? 0.01f : -0.01f), 0.08f, 0.32f);
		}
	}

	protected override void OnPaint(PaintEventArgs e)
	{
		base.OnPaint(e);
		Graphics graphics = e.Graphics;
		graphics.SmoothingMode = SmoothingMode.AntiAlias;
		graphics.Clear(Color.FromArgb(18, 20, 23));

		Rectangle viewport = GetViewport();
		using SolidBrush viewportBrush = new(Color.FromArgb(24, 29, 34));
		graphics.FillRectangle(viewportBrush, viewport);
		DrawGrid(graphics, viewport);
		DrawObstacles(graphics);
		DrawProjectiles(graphics);
		DrawAgents(graphics);
		DrawBuildPreview(graphics);
		DrawPanel(graphics);
	}

	private void HandleTick(object? sender, EventArgs e)
	{
		float dt = (float)stopwatch.Elapsed.TotalSeconds;
		stopwatch.Restart();

		if (!isSimulationRunning && keysDown.Contains(Keys.Q) && buildMode != BuildMode.None)
		{
			previewRotationRadians -= dt * 2.4f;
		}

		if (!isSimulationRunning && keysDown.Contains(Keys.E) && buildMode != BuildMode.None)
		{
			previewRotationRadians += dt * 2.4f;
		}

		if (isSimulationRunning && !paused)
		{
			world.Update(new SimulationInput
			{
				MoveDirection = ReadMoveDirection(),
				AimWorld = mouseWorld,
				FireHeld = fireHeld,
				SprintHeld = IsSprintHeld()
			}, dt);
		}

		Invalidate();
	}

	private void InitializePanelControls()
	{
		startButton.Text = "Start";
		startButton.Font = new Font("Segoe UI", 10.0f, FontStyle.Bold);
		startButton.FlatStyle = FlatStyle.Flat;
		startButton.BackColor = Color.FromArgb(64, 122, 214);
		startButton.ForeColor = Color.White;
		startButton.FlatAppearance.BorderColor = Color.FromArgb(114, 164, 238);
		startButton.Size = new Size(264, 34);
		startButton.Click += (_, _) => StartSimulation();
		Controls.Add(startButton);
		PositionPanelControls();
	}

	protected override void OnResize(EventArgs e)
	{
		base.OnResize(e);
		PositionPanelControls();
	}

	private void PositionPanelControls()
	{
		int panelLeft = Math.Max(0, ClientSize.Width - PanelWidth);
		startButton.Location = new Point(panelLeft + 18, 138);
	}

	private void StartSimulation()
	{
		isSimulationRunning = true;
		paused = false;
		fireHeld = false;
		buildMode = BuildMode.None;
		startButton.Enabled = false;
		startButton.Text = "Running";
		startButton.BackColor = Color.FromArgb(75, 90, 105);
		stopwatch.Restart();
		Invalidate();
	}

	private void ResetToEditMode()
	{
		world.ResetScenario();
		isSimulationRunning = false;
		paused = false;
		fireHeld = false;
		buildMode = BuildMode.None;
		keysDown.Clear();
		startButton.Enabled = true;
		startButton.Text = "Start";
		startButton.BackColor = Color.FromArgb(64, 122, 214);
		stopwatch.Restart();
		Invalidate();
	}

	private Vector2 ReadMoveDirection()
	{
		Vector2 move = Vector2.Zero;
		if (keysDown.Contains(Keys.W))
		{
			move.X += 1.0f;
		}

		if (keysDown.Contains(Keys.S))
		{
			move.X -= 1.0f;
		}

		if (keysDown.Contains(Keys.D))
		{
			move.Y += 1.0f;
		}

		if (keysDown.Contains(Keys.A))
		{
			move.Y -= 1.0f;
		}

		return Geometry.NormalizeOrZero(move);
	}

	private bool IsSprintHeld()
	{
		return keysDown.Contains(Keys.ShiftKey) ||
			keysDown.Contains(Keys.LShiftKey) ||
			keysDown.Contains(Keys.RShiftKey);
	}

	private Rectangle GetViewport()
	{
		return new Rectangle(0, 0, Math.Max(1, ClientSize.Width - PanelWidth), ClientSize.Height);
	}

	private PointF WorldToScreen(Vector2 worldPoint)
	{
		Rectangle viewport = GetViewport();
		float centerX = viewport.Left + viewport.Width * 0.5f;
		float centerY = viewport.Top + viewport.Height * 0.5f;
		return new PointF(centerX + worldPoint.Y * renderScale, centerY - worldPoint.X * renderScale);
	}

	private Vector2 ScreenToWorld(Point screenPoint)
	{
		Rectangle viewport = GetViewport();
		float centerX = viewport.Left + viewport.Width * 0.5f;
		float centerY = viewport.Top + viewport.Height * 0.5f;
		return new Vector2((centerY - screenPoint.Y) / renderScale, (screenPoint.X - centerX) / renderScale);
	}

	private float ToPixels(float centimeters)
	{
		return centimeters * renderScale;
	}

	private void DrawGrid(Graphics graphics, Rectangle viewport)
	{
		using Pen minorPen = new(Color.FromArgb(44, 51, 58));
		using Pen majorPen = new(Color.FromArgb(58, 68, 76));
		for (int world = -3000; world <= 3000; world += 250)
		{
			PointF a = WorldToScreen(new Vector2(world, -3000.0f));
			PointF b = WorldToScreen(new Vector2(world, 3000.0f));
			graphics.DrawLine(world % 1000 == 0 ? majorPen : minorPen, a, b);

			PointF c = WorldToScreen(new Vector2(-3000.0f, world));
			PointF d = WorldToScreen(new Vector2(3000.0f, world));
			graphics.DrawLine(world % 1000 == 0 ? majorPen : minorPen, c, d);
		}

		using Pen northPen = new(Color.FromArgb(72, 147, 244), 2.0f);
		using Pen eastPen = new(Color.FromArgb(92, 200, 153), 2.0f);
		graphics.DrawLine(northPen, WorldToScreen(new Vector2(-3000.0f, 0.0f)), WorldToScreen(new Vector2(3000.0f, 0.0f)));
		graphics.DrawLine(eastPen, WorldToScreen(new Vector2(0.0f, -3000.0f)), WorldToScreen(new Vector2(0.0f, 3000.0f)));
	}

	private void DrawObstacles(Graphics graphics)
	{
		foreach (Obstacle obstacle in world.Obstacles)
		{
			PointF[] points = Geometry.GetRectangleVertices(obstacle).Select(WorldToScreen).ToArray();
			Color fill = obstacle.Kind == ObstacleKind.Destructible
				? Color.FromArgb(120, 185, 115, 70)
				: Color.FromArgb(160, 90, 105, 121);
			Color stroke = obstacle.Kind == ObstacleKind.Destructible
				? Color.FromArgb(235, 224, 143, 91)
				: Color.FromArgb(245, 172, 190, 212);

			using SolidBrush brush = new(fill);
			using Pen pen = new(stroke, 2.0f);
			graphics.FillPolygon(brush, points);
			graphics.DrawPolygon(pen, points);

			if (obstacle.Kind == ObstacleKind.Destructible)
			{
				DrawCenteredText(graphics, $"{MathF.Ceiling(obstacle.Health)}", obstacle.Center, Color.FromArgb(240, 255, 220, 170));
			}
		}
	}

	private void DrawProjectiles(Graphics graphics)
	{
		foreach (Projectile projectile in world.Projectiles)
		{
			PointF point = WorldToScreen(projectile.Position);
			float radius = ToPixels(projectile.Radius);
			Color color = projectile.Owner == ProjectileOwner.Player ? Color.White : Color.FromArgb(255, 255, 96, 82);
			using SolidBrush brush = new(color);
			graphics.FillEllipse(brush, point.X - radius, point.Y - radius, radius * 2.0f, radius * 2.0f);
		}
	}

	private void DrawAgents(Graphics graphics)
	{
		if (showDebug)
		{
			DrawCombatDisengageLines(graphics);
		}

		DrawPlayer(graphics);
		foreach (EnemyAgent enemy in world.Enemies)
		{
			DrawEnemy(graphics, enemy);
		}
	}

	private void DrawCombatDisengageLines(Graphics graphics)
	{
		foreach (EnemyAgent enemy in world.Enemies)
		{
			float trackingRange = world.ResolveTrackingRange(enemy);
			float disengageRange = MathF.Max(trackingRange, world.Tuning.Enemy.CombatDisengageRange);
			if (trackingRange <= 0.0f && disengageRange <= 0.0f)
			{
				continue;
			}

			Vector2 toPlayer = world.Player.Position - enemy.Position;
			float distanceToPlayer = toPlayer.Length();
			if (distanceToPlayer <= 1.0f)
			{
				continue;
			}

			Vector2 directionToPlayer = toPlayer / distanceToPlayer;
			PointF playerPoint = WorldToScreen(world.Player.Position);

			if (!enemy.IsCombatEngaged)
			{
				DrawCombatVisionConeOutline(graphics, enemy, trackingRange);
				continue;
			}

			if (distanceToPlayer < disengageRange)
			{
				Vector2 disengageEnd = enemy.Position + directionToPlayer * disengageRange;
				using Pen remainingPen = new(Color.FromArgb(64, 255, 255, 255), 2.0f)
				{
					DashStyle = DashStyle.Dash
				};
				graphics.DrawLine(remainingPen, playerPoint, WorldToScreen(disengageEnd));
			}
		}
	}

	private void DrawCombatVisionConeOutline(Graphics graphics, EnemyAgent enemy, float range)
	{
		Vector2 facing = Geometry.NormalizeOrZero(enemy.Facing);
		if (facing == Vector2.Zero || range <= 0.0f)
		{
			return;
		}

		float angleDegrees = Math.Clamp(world.Tuning.Enemy.CombatVisionAngleDegrees, 0.0f, 360.0f);
		float halfAngleRadians = MathF.Min(MathF.PI, angleDegrees * MathF.PI / 360.0f);
		const int ArcSegments = 24;

		using Pen conePen = new(Color.FromArgb(64, 255, 255, 255), 1.0f)
		{
			DashStyle = DashStyle.Dash
		};

		PointF origin = WorldToScreen(enemy.Position);
		Vector2 leftDirection = Geometry.Rotate(facing, -halfAngleRadians);
		Vector2 rightDirection = Geometry.Rotate(facing, halfAngleRadians);
		graphics.DrawLine(conePen, origin, WorldToScreen(enemy.Position + leftDirection * range));
		graphics.DrawLine(conePen, origin, WorldToScreen(enemy.Position + rightDirection * range));

		PointF previousPoint = WorldToScreen(enemy.Position + leftDirection * range);
		for (int i = 1; i <= ArcSegments; ++i)
		{
			float t = i / (float)ArcSegments;
			float angle = -halfAngleRadians + halfAngleRadians * 2.0f * t;
			PointF nextPoint = WorldToScreen(enemy.Position + Geometry.Rotate(facing, angle) * range);
			graphics.DrawLine(conePen, previousPoint, nextPoint);
			previousPoint = nextPoint;
		}
	}

	private void DrawPlayer(Graphics graphics)
	{
		PointF playerPoint = WorldToScreen(world.Player.Position);
		float radius = ToPixels(world.Tuning.Player.Radius);
		using SolidBrush brush = new(Color.FromArgb(240, 56, 170, 255));
		using Pen outline = new(Color.White, 2.0f);
		graphics.FillEllipse(brush, playerPoint.X - radius, playerPoint.Y - radius, radius * 2.0f, radius * 2.0f);
		graphics.DrawEllipse(outline, playerPoint.X - radius, playerPoint.Y - radius, radius * 2.0f, radius * 2.0f);

		PointF aimEnd = WorldToScreen(world.Player.Position + world.Player.AimDirection * 260.0f);
		using Pen aimPen = new(Color.FromArgb(230, 255, 255, 255), 2.0f);
		graphics.DrawLine(aimPen, playerPoint, aimEnd);
		DrawHealthBar(graphics, playerPoint, radius, world.Player.Health, world.Tuning.Player.Health, Color.FromArgb(68, 209, 126));
	}

	private void DrawEnemy(Graphics graphics, EnemyAgent enemy)
	{
		PointF enemyPoint = WorldToScreen(enemy.Position);
		float radius = ToPixels(world.Tuning.Enemy.Radius);
		Color fill = enemy.Kind switch
		{
			EnemyKind.Melee => Color.FromArgb(232, 218, 79, 74),
			EnemyKind.Flanker => Color.FromArgb(232, 74, 205, 142),
			_ => Color.FromArgb(232, 184, 99, 224)
		};
		Color outlineColor = enemy.State switch
		{
			EnemyState.SeekLineOfFire => Color.FromArgb(255, 255, 204, 95),
			EnemyState.Strafe => Color.FromArgb(255, 118, 242, 174),
			EnemyState.Retreat => Color.FromArgb(255, 93, 202, 255),
			EnemyState.AttackCommit => Color.FromArgb(255, 255, 255, 255),
			_ => Color.FromArgb(255, 35, 35, 40)
		};

		using SolidBrush brush = new(fill);
		using Pen outline = new(outlineColor, 2.0f);
		graphics.FillEllipse(brush, enemyPoint.X - radius, enemyPoint.Y - radius, radius * 2.0f, radius * 2.0f);
		graphics.DrawEllipse(outline, enemyPoint.X - radius, enemyPoint.Y - radius, radius * 2.0f, radius * 2.0f);

		PointF facingEnd = WorldToScreen(enemy.Position + enemy.Facing * 165.0f);
		using Pen facingPen = new(outlineColor, 2.0f);
		graphics.DrawLine(facingPen, enemyPoint, facingEnd);

		if (showDebug)
		{
			using Pen rangePen = new(Color.FromArgb(60, outlineColor), 1.0f);
			float range = enemy.Kind switch
			{
				EnemyKind.Melee => EnemyCombatConstants.MeleeAttackRange,
				EnemyKind.Flanker => world.Tuning.Enemy.FlankerPreferredMax,
				_ => world.Tuning.Enemy.RangedPreferredMax
			};
			graphics.DrawEllipse(rangePen, enemyPoint.X - ToPixels(range), enemyPoint.Y - ToPixels(range), ToPixels(range) * 2.0f, ToPixels(range) * 2.0f);
			DrawCenteredText(graphics, enemy.State.ToString(), enemy.Position + new Vector2(-145.0f, 0.0f), Color.FromArgb(225, 235, 238, 244));
		}

		DrawHealthBar(graphics, enemyPoint, radius, enemy.Health, world.Tuning.Enemy.Health, Color.FromArgb(228, 88, 84));
	}

	private void DrawHealthBar(Graphics graphics, PointF center, float radius, float health, float maxHealth, Color fillColor)
	{
		float width = MathF.Max(34.0f, radius * 2.0f);
		float height = 5.0f;
		float y = center.Y - radius - 12.0f;
		RectangleF backRect = new(center.X - width * 0.5f, y, width, height);
		RectangleF fillRect = backRect;
		fillRect.Width *= Math.Clamp(health / MathF.Max(1.0f, maxHealth), 0.0f, 1.0f);
		using SolidBrush backBrush = new(Color.FromArgb(160, 15, 15, 18));
		using SolidBrush fillBrush = new(fillColor);
		graphics.FillRectangle(backBrush, backRect);
		graphics.FillRectangle(fillBrush, fillRect);
	}

	private void DrawBuildPreview(Graphics graphics)
	{
		if (isSimulationRunning || buildMode == BuildMode.None)
		{
			return;
		}

		if (buildMode is BuildMode.MeleeEnemy or BuildMode.RangedEnemy or BuildMode.FlankerEnemy)
		{
			DrawEnemyPlacementPreview(graphics);
			return;
		}

		Obstacle preview = new()
		{
			Kind = buildMode == BuildMode.Destructible ? ObstacleKind.Destructible : ObstacleKind.Indestructible,
			Center = mouseWorld,
			Size = previewSize,
			RotationRadians = previewRotationRadians,
			Health = 1.0f
		};

		PointF[] points = Geometry.GetRectangleVertices(preview).Select(WorldToScreen).ToArray();
		using SolidBrush brush = new(Color.FromArgb(52, 255, 255, 255));
		using Pen pen = new(Color.FromArgb(230, 255, 255, 255), 2.0f) { DashStyle = DashStyle.Dash };
		graphics.FillPolygon(brush, points);
		graphics.DrawPolygon(pen, points);
	}

	private void DrawEnemyPlacementPreview(Graphics graphics)
	{
		PointF point = WorldToScreen(mouseWorld);
		float radius = ToPixels(world.Tuning.Enemy.Radius);
		Color color = buildMode switch
		{
			BuildMode.MeleeEnemy => Color.FromArgb(150, 218, 79, 74),
			BuildMode.FlankerEnemy => Color.FromArgb(150, 74, 205, 142),
			_ => Color.FromArgb(150, 184, 99, 224)
		};

		using SolidBrush brush = new(color);
		using Pen pen = new(Color.FromArgb(230, 255, 255, 255), 2.0f) { DashStyle = DashStyle.Dash };
		graphics.FillEllipse(brush, point.X - radius, point.Y - radius, radius * 2.0f, radius * 2.0f);
		graphics.DrawEllipse(pen, point.X - radius, point.Y - radius, radius * 2.0f, radius * 2.0f);
		string label = buildMode switch
		{
			BuildMode.MeleeEnemy => "Melee",
			BuildMode.FlankerEnemy => "Flanker",
			_ => "Ranged"
		};
		DrawCenteredText(graphics, label, mouseWorld + new Vector2(-120.0f, 0.0f), Color.White);
	}

	private void PlaceCurrentPreview()
	{
		switch (buildMode)
		{
			case BuildMode.Destructible:
				world.AddObstacle(ObstacleKind.Destructible, mouseWorld, previewSize, previewRotationRadians);
				break;
			case BuildMode.Indestructible:
				world.AddObstacle(ObstacleKind.Indestructible, mouseWorld, previewSize, previewRotationRadians);
				break;
			case BuildMode.MeleeEnemy:
				world.SpawnEnemy(EnemyKind.Melee, mouseWorld);
				break;
			case BuildMode.RangedEnemy:
				world.SpawnEnemy(EnemyKind.Ranged, mouseWorld);
				break;
			case BuildMode.FlankerEnemy:
				world.SpawnEnemy(EnemyKind.Flanker, mouseWorld);
				break;
		}
	}

	private void DrawPanel(Graphics graphics)
	{
		Rectangle panel = new(ClientSize.Width - PanelWidth, 0, PanelWidth, ClientSize.Height);
		using SolidBrush panelBrush = new(Color.FromArgb(28, 31, 36));
		graphics.FillRectangle(panelBrush, panel);

		int x = panel.Left + 18;
		int y = 18;
		DrawText(graphics, "Combat Movement Simulator", x, y, 14.0f, Color.White, FontStyle.Bold);
		y += 34;
		DrawText(graphics, $"Time {world.ElapsedSeconds:0.0}s  Scale {renderScale:0.00}", x, y, 9.5f, Color.FromArgb(210, 230, 235, 241));
		y += 20;
		DrawText(graphics, $"Player HP {world.Player.Health:0}/{world.Tuning.Player.Health:0}", x, y, 9.5f, Color.FromArgb(210, 120, 220, 170));
		y += 20;
		DrawText(graphics, $"Enemies {world.Enemies.Count}  Obstacles {world.Obstacles.Count}", x, y, 9.5f, Color.FromArgb(210, 230, 235, 241));
		y += 20;
		string runMode = isSimulationRunning ? (paused ? "Paused" : "Simulation") : "Edit";
		DrawText(graphics, $"Run {runMode}  Placement {buildMode}", x, y, 9.5f, Color.FromArgb(210, 230, 235, 241));
		y += 72;

		DrawText(graphics, "Controls", x, y, 11.0f, Color.White, FontStyle.Bold);
		y += 24;
		foreach (string line in new[]
		{
			"Start button: enter simulation",
			"WASD: move after start",
			"Shift: sprint x2 after start",
			"Mouse: aim after start",
			"LMB: fire after start",
			"R: reset scenario",
			"P: pause",
			"T: debug overlay",
			"F: freeze enemies",
			"B: cycle placement mode",
			"1/2/3: melee/ranged/flanker",
			"4/5: destructible/indestructible",
			"RMB: place current preview",
			"Wheel or Q/E: rotate obstacle",
			"-/+: obstacle width",
			"[/]: obstacle height",
			"Delete: remove enemy/obstacle"
		})
		{
			DrawText(graphics, line, x, y, 9.0f, Color.FromArgb(210, 222, 227, 233));
			y += 19;
		}

		y += 16;
		DrawText(graphics, "Line of fire", x, y, 11.0f, Color.White, FontStyle.Bold);
		y += 24;
		LineOfFire playerLine = world.EvaluateLineOfFire(world.Player.Position, mouseWorld);
		DrawText(graphics, $"Player aim: {playerLine}", x, y, 9.0f, playerLine == LineOfFire.Clear ? Color.FromArgb(140, 230, 165) : Color.FromArgb(255, 205, 118));
		y += 19;
		DrawText(graphics, $"Mouse world X {mouseWorld.X:0}  Y {mouseWorld.Y:0}", x, y, 9.0f, Color.FromArgb(210, 222, 227, 233));

		if (world.Player.Health <= 0.0f)
		{
			using SolidBrush overlay = new(Color.FromArgb(180, 0, 0, 0));
			graphics.FillRectangle(overlay, GetViewport());
			DrawText(graphics, "PLAYER DOWN - PRESS R", 80, ClientSize.Height / 2 - 28, 24.0f, Color.White, FontStyle.Bold);
		}
	}

	private void DrawText(Graphics graphics, string text, float x, float y, float size, Color color, FontStyle style = FontStyle.Regular)
	{
		using Font font = new("Segoe UI", size, style);
		using SolidBrush brush = new(color);
		graphics.DrawString(text, font, brush, x, y);
	}

	private void DrawCenteredText(Graphics graphics, string text, Vector2 worldPosition, Color color)
	{
		PointF point = WorldToScreen(worldPosition);
		using Font font = new("Segoe UI", 8.5f, FontStyle.Bold);
		using SolidBrush brush = new(color);
		SizeF size = graphics.MeasureString(text, font);
		graphics.DrawString(text, font, brush, point.X - size.Width * 0.5f, point.Y - size.Height * 0.5f);
	}
}
