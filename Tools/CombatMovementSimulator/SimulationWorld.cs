using System.Numerics;

namespace CombatMovementSimulator;

internal sealed class SimulationWorld
{
	private readonly Random random = new(1337);

	public SimulationWorld(CombatTuning tuning)
	{
		Tuning = tuning;
		ResetScenario();
	}

	public CombatTuning Tuning { get; }
	public PlayerAgent Player { get; } = new();
	public List<EnemyAgent> Enemies { get; } = [];
	public List<Obstacle> Obstacles { get; } = [];
	public List<Projectile> Projectiles { get; } = [];
	public bool FreezeEnemies { get; set; }
	public float ElapsedSeconds { get; private set; }

	public void ResetScenario()
	{
		ElapsedSeconds = 0.0f;
		Enemies.Clear();
		Obstacles.Clear();
		Projectiles.Clear();

		Player.Position = Vector2.Zero;
		Player.AimDirection = new Vector2(1.0f, 0.0f);
		Player.Health = Tuning.Player.Health;
		Player.FireCooldownRemaining = 0.0f;

		AddObstacle(ObstacleKind.Indestructible, new Vector2(520.0f, -360.0f), new Vector2(520.0f, 130.0f), DegreesToRadians(28.0f));
		AddObstacle(ObstacleKind.Destructible, new Vector2(850.0f, 310.0f), new Vector2(430.0f, 110.0f), DegreesToRadians(-18.0f));
		AddObstacle(ObstacleKind.Indestructible, new Vector2(-300.0f, 520.0f), new Vector2(560.0f, 120.0f), DegreesToRadians(55.0f));
		AddObstacle(ObstacleKind.Destructible, new Vector2(-690.0f, -260.0f), new Vector2(380.0f, 130.0f), DegreesToRadians(8.0f));
		AddObstacle(ObstacleKind.Indestructible, new Vector2(1450.0f, -40.0f), new Vector2(180.0f, 650.0f), DegreesToRadians(0.0f));

		SpawnEnemy(EnemyKind.Melee, new Vector2(1180.0f, -690.0f));
		SpawnEnemy(EnemyKind.Melee, new Vector2(-920.0f, -720.0f));
		SpawnEnemy(EnemyKind.Ranged, new Vector2(1260.0f, 760.0f));
		SpawnEnemy(EnemyKind.Ranged, new Vector2(-1120.0f, 580.0f));
	}

	public void AddObstacle(ObstacleKind kind, Vector2 center, Vector2 size, float rotationRadians)
	{
		Obstacles.Add(new Obstacle
		{
			Kind = kind,
			Center = center,
			Size = size,
			RotationRadians = rotationRadians,
			Health = kind == ObstacleKind.Destructible ? Tuning.World.DestructibleObstacleHealth : float.PositiveInfinity
		});
	}

	public void SpawnEnemy(EnemyKind kind, Vector2 position)
	{
		Enemies.Add(new EnemyAgent
		{
			Kind = kind,
			Position = position,
			Health = Tuning.Enemy.Health,
			State = EnemyState.Idle,
			StrafeSign = random.Next(0, 2) == 0 ? -1.0f : 1.0f
		});
	}

	public void RemoveObstacleAt(Vector2 worldPoint)
	{
		for (int i = Enemies.Count - 1; i >= 0; --i)
		{
			if (Vector2.DistanceSquared(Enemies[i].Position, worldPoint) <= Tuning.Enemy.Radius * Tuning.Enemy.Radius)
			{
				Enemies.RemoveAt(i);
				return;
			}
		}

		for (int i = Obstacles.Count - 1; i >= 0; --i)
		{
			if (Geometry.ContainsPoint(worldPoint, Obstacles[i]))
			{
				Obstacles.RemoveAt(i);
				return;
			}
		}
	}

	public void Update(SimulationInput input, float deltaSeconds)
	{
		float dt = Math.Clamp(deltaSeconds, 0.0f, 0.05f);
		ElapsedSeconds += dt;

		UpdatePlayer(input, dt);
		if (!FreezeEnemies)
		{
			foreach (EnemyAgent enemy in Enemies)
			{
				UpdateEnemy(enemy, dt);
			}
		}

		UpdateProjectiles(dt);
		Enemies.RemoveAll(enemy => enemy.Health <= 0.0f);
		Obstacles.RemoveAll(obstacle => obstacle.IsDestroyed);
	}

	public LineOfFire EvaluateLineOfFire(Vector2 start, Vector2 end)
	{
		LineOfFire result = LineOfFire.Clear;
		float bestTime = float.PositiveInfinity;

		foreach (Obstacle obstacle in Obstacles)
		{
			if (Geometry.TrySegmentHitRectangle(start, end, obstacle, out float hitTime, out _) && hitTime < bestTime)
			{
				bestTime = hitTime;
				result = obstacle.Kind == ObstacleKind.Destructible
					? LineOfFire.BlockedByDestructible
					: LineOfFire.BlockedByIndestructible;
			}
		}

		return result;
	}

	private void UpdatePlayer(SimulationInput input, float dt)
	{
		if (Player.Health <= 0.0f)
		{
			return;
		}

		Vector2 toAim = input.AimWorld - Player.Position;
		if (toAim.LengthSquared() > 1.0f)
		{
			Player.AimDirection = Vector2.Normalize(toAim);
		}

		Vector2 movement = Geometry.NormalizeOrZero(input.MoveDirection) * Tuning.Player.MoveSpeed * dt;
		Player.Position += movement;
		Vector2 playerPosition = Player.Position;
		ResolveCircleCollisions(ref playerPosition, Tuning.Player.Radius);
		Player.Position = playerPosition;
		Player.FireCooldownRemaining = MathF.Max(0.0f, Player.FireCooldownRemaining - dt);

		if (input.FireHeld && Player.FireCooldownRemaining <= 0.0f)
		{
			FireProjectile(ProjectileOwner.Player, Player.Position + Player.AimDirection * (Tuning.Player.Radius + 14.0f), Player.AimDirection, Tuning.Player.ProjectileSpeed, Tuning.Player.ProjectileDamage);
			Player.FireCooldownRemaining = Tuning.Player.FireCooldown;
		}
	}

	private void UpdateEnemy(EnemyAgent enemy, float dt)
	{
		if (enemy.Health <= 0.0f || Player.Health <= 0.0f)
		{
			enemy.State = EnemyState.Idle;
			return;
		}

		enemy.FireCooldownRemaining = MathF.Max(0.0f, enemy.FireCooldownRemaining - dt);
		enemy.StateTimer = MathF.Max(0.0f, enemy.StateTimer - dt);
		enemy.StrafeTimer -= dt;
		if (enemy.StrafeTimer <= 0.0f)
		{
			enemy.StrafeTimer = Tuning.Enemy.RangedStrafeChangeSeconds * RandomRange(0.65f, 1.4f);
			enemy.StrafeSign = random.Next(0, 2) == 0 ? -1.0f : 1.0f;
		}

		Vector2 toPlayer = Player.Position - enemy.Position;
		float distance = toPlayer.Length();
		Vector2 directionToPlayer = distance > 0.001f ? toPlayer / distance : enemy.Facing;
		enemy.Facing = directionToPlayer;

		if (distance > Tuning.Enemy.TrackingRange)
		{
			enemy.State = EnemyState.Idle;
			return;
		}

		if (enemy.Kind == EnemyKind.Melee)
		{
			UpdateMeleeEnemy(enemy, directionToPlayer, distance, dt);
		}
		else
		{
			UpdateRangedEnemy(enemy, directionToPlayer, distance, dt);
		}
	}

	private void UpdateMeleeEnemy(EnemyAgent enemy, Vector2 directionToPlayer, float distance, float dt)
	{
		if (enemy.State == EnemyState.AttackCommit)
		{
			if (!enemy.HasAppliedAttack && enemy.StateTimer <= Tuning.Enemy.MeleeWindupSeconds * 0.45f)
			{
				if (distance <= Tuning.Enemy.MeleeAttackRange + Tuning.Player.Radius)
				{
					Player.Health = MathF.Max(0.0f, Player.Health - Tuning.Enemy.MeleeDamage);
				}

				enemy.HasAppliedAttack = true;
			}

			if (enemy.StateTimer <= 0.0f)
			{
				enemy.State = EnemyState.Recover;
				enemy.StateTimer = Tuning.Enemy.MeleeRecoverSeconds;
			}

			return;
		}

		if (enemy.State == EnemyState.Recover && enemy.StateTimer > 0.0f)
		{
			MoveEnemy(enemy, Geometry.Perpendicular(directionToPlayer) * enemy.StrafeSign * 0.45f, Tuning.Enemy.MeleeMoveSpeed * 0.45f, dt);
			return;
		}

		if (distance <= Tuning.Enemy.MeleeAttackRange + Tuning.Player.Radius)
		{
			enemy.State = EnemyState.AttackCommit;
			enemy.StateTimer = Tuning.Enemy.MeleeWindupSeconds;
			enemy.HasAppliedAttack = false;
			return;
		}

		enemy.State = EnemyState.Approach;
		MoveEnemy(enemy, directionToPlayer, Tuning.Enemy.MeleeMoveSpeed, dt);
	}

	private void UpdateRangedEnemy(EnemyAgent enemy, Vector2 directionToPlayer, float distance, float dt)
	{
		LineOfFire lineOfFire = EvaluateLineOfFire(enemy.Position, Player.Position);
		Vector2 desiredDirection;

		if (lineOfFire == LineOfFire.BlockedByIndestructible)
		{
			enemy.State = EnemyState.SeekLineOfFire;
			Vector2 lateral = Geometry.Perpendicular(directionToPlayer) * enemy.StrafeSign * Tuning.Enemy.SeekLineOfFireStrafeWeight;
			desiredDirection = Geometry.NormalizeOrZero(lateral + directionToPlayer * 0.35f);
		}
		else if (distance < Tuning.Enemy.RangedDangerClose)
		{
			enemy.State = EnemyState.Retreat;
			desiredDirection = Geometry.NormalizeOrZero(-directionToPlayer + Geometry.Perpendicular(directionToPlayer) * enemy.StrafeSign * 0.35f);
		}
		else if (distance > Tuning.Enemy.RangedPreferredMax)
		{
			enemy.State = EnemyState.Approach;
			desiredDirection = Geometry.NormalizeOrZero(directionToPlayer + Geometry.Perpendicular(directionToPlayer) * enemy.StrafeSign * 0.15f);
		}
		else if (distance < Tuning.Enemy.RangedPreferredMin)
		{
			enemy.State = EnemyState.Retreat;
			desiredDirection = -directionToPlayer;
		}
		else
		{
			enemy.State = enemy.StrafeSign > 0.0f ? EnemyState.Strafe : EnemyState.HoldRange;
			desiredDirection = Geometry.Perpendicular(directionToPlayer) * enemy.StrafeSign;
		}

		MoveEnemy(enemy, desiredDirection, Tuning.Enemy.RangedMoveSpeed, dt);

		if (distance <= Tuning.Enemy.RangedAttackRange &&
			lineOfFire != LineOfFire.BlockedByIndestructible &&
			enemy.FireCooldownRemaining <= 0.0f)
		{
			float spreadRadians = DegreesToRadians(lineOfFire == LineOfFire.BlockedByDestructible ? 1.0f : 3.0f);
			Vector2 shotDirection = Geometry.Rotate(directionToPlayer, RandomRange(-spreadRadians, spreadRadians));
			FireProjectile(ProjectileOwner.Enemy, enemy.Position + shotDirection * (Tuning.Enemy.Radius + 14.0f), shotDirection, Tuning.Enemy.RangedProjectileSpeed, Tuning.Enemy.RangedProjectileDamage);
			enemy.FireCooldownRemaining = Tuning.Enemy.RangedFireCooldown * RandomRange(0.82f, 1.22f);
		}
	}

	private void MoveEnemy(EnemyAgent enemy, Vector2 direction, float speed, float dt)
	{
		Vector2 normalized = Geometry.NormalizeOrZero(direction);
		if (normalized == Vector2.Zero)
		{
			return;
		}

		enemy.Position += normalized * speed * dt;
		Vector2 enemyPosition = enemy.Position;
		ResolveCircleCollisions(ref enemyPosition, Tuning.Enemy.Radius);
		enemy.Position = enemyPosition;
	}

	private void UpdateProjectiles(float dt)
	{
		for (int i = Projectiles.Count - 1; i >= 0; --i)
		{
			Projectile projectile = Projectiles[i];
			projectile.LifeSeconds -= dt;
			projectile.PreviousPosition = projectile.Position;
			projectile.Position += projectile.Velocity * dt;

			if (projectile.LifeSeconds <= 0.0f || ResolveProjectileHit(projectile))
			{
				Projectiles.RemoveAt(i);
			}
		}
	}

	private bool ResolveProjectileHit(Projectile projectile)
	{
		float bestTime = float.PositiveInfinity;
		Obstacle? hitObstacle = null;
		EnemyAgent? hitEnemy = null;
		bool hitPlayer = false;

		foreach (Obstacle obstacle in Obstacles)
		{
			if (Geometry.TrySegmentHitRectangle(projectile.PreviousPosition, projectile.Position, obstacle, out float hitTime, out _) && hitTime < bestTime)
			{
				bestTime = hitTime;
				hitObstacle = obstacle;
				hitEnemy = null;
				hitPlayer = false;
			}
		}

		if (projectile.Owner == ProjectileOwner.Player)
		{
			foreach (EnemyAgent enemy in Enemies)
			{
				if (Geometry.TrySegmentHitCircle(projectile.PreviousPosition, projectile.Position, enemy.Position, Tuning.Enemy.Radius + projectile.Radius, out float hitTime) && hitTime < bestTime)
				{
					bestTime = hitTime;
					hitEnemy = enemy;
					hitObstacle = null;
					hitPlayer = false;
				}
			}
		}
		else if (Geometry.TrySegmentHitCircle(projectile.PreviousPosition, projectile.Position, Player.Position, Tuning.Player.Radius + projectile.Radius, out float hitTime) && hitTime < bestTime)
		{
			hitPlayer = true;
			hitObstacle = null;
			hitEnemy = null;
		}

		if (hitObstacle != null)
		{
			if (hitObstacle.Kind == ObstacleKind.Destructible)
			{
				hitObstacle.Health -= projectile.Damage;
			}

			return true;
		}

		if (hitEnemy != null)
		{
			hitEnemy.Health -= projectile.Damage;
			return true;
		}

		if (hitPlayer)
		{
			Player.Health = MathF.Max(0.0f, Player.Health - projectile.Damage);
			return true;
		}

		return false;
	}

	private void ResolveCircleCollisions(ref Vector2 position, float radius)
	{
		for (int pass = 0; pass < 2; ++pass)
		{
			foreach (Obstacle obstacle in Obstacles)
			{
				Geometry.PushCircleOutOfRectangle(obstacle, ref position, radius);
			}
		}
	}

	private void FireProjectile(ProjectileOwner owner, Vector2 start, Vector2 direction, float speed, float damage)
	{
		Vector2 normalized = Geometry.NormalizeOrZero(direction);
		if (normalized == Vector2.Zero)
		{
			return;
		}

		Projectiles.Add(new Projectile
		{
			Owner = owner,
			Position = start,
			PreviousPosition = start,
			Velocity = normalized * speed,
			Damage = damage,
			Radius = Tuning.World.ProjectileRadius
		});
	}

	private float RandomRange(float min, float max)
	{
		return min + (float)random.NextDouble() * (max - min);
	}

	private static float DegreesToRadians(float degrees)
	{
		return degrees * MathF.PI / 180.0f;
	}
}
