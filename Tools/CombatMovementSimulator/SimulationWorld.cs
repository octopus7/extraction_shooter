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

		float speedMultiplier = input.SprintHeld ? MathF.Max(0.0f, Tuning.Player.SprintSpeedMultiplier) : 1.0f;
		Vector2 movement = Geometry.NormalizeOrZero(input.MoveDirection) * Tuning.Player.MoveSpeed * speedMultiplier * dt;
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
		float dangerCloseRange = MathF.Max(0.0f, Tuning.Enemy.RangedDangerClose);
		float preferredMin = MathF.Max(dangerCloseRange, Tuning.Enemy.RangedPreferredMin);
		float preferredMax = MathF.Max(preferredMin, Tuning.Enemy.RangedPreferredMax);

		if (distance <= dangerCloseRange)
		{
			if (enemy.State != EnemyState.KeepDistance || enemy.StateTimer <= 0.0f)
			{
				StartRangedKeepDistance(enemy, directionToPlayer);
			}

			MoveRangedCombatState(enemy, dt);
			return;
		}

		switch (enemy.State)
		{
			case EnemyState.AdvanceBurst:
				if (lineOfFire == LineOfFire.BlockedByIndestructible)
				{
					StartRangedSeekLineOfFire(enemy, distance, directionToPlayer);
					MoveRangedCombatState(enemy, dt);
					return;
				}

				if (enemy.StateTimer <= 0.0f ||
					distance <= preferredMin ||
					Vector2.DistanceSquared(enemy.Position, enemy.RangedMoveGoal) <= MathF.Pow(MathF.Max(1.0f, Tuning.Enemy.RangedMoveGoalAcceptanceRadius), 2.0f))
				{
					StartRangedHold(enemy, distance);
					return;
				}

				MoveRangedCombatState(enemy, dt);
				return;

			case EnemyState.HoldFire:
				if (lineOfFire == LineOfFire.BlockedByIndestructible)
				{
					StartRangedSeekLineOfFire(enemy, distance, directionToPlayer);
					MoveRangedCombatState(enemy, dt);
					return;
				}

				TryRangedAttack(enemy, directionToPlayer, distance, lineOfFire);
				if (enemy.StateTimer > 0.0f)
				{
					return;
				}

				enemy.State = EnemyState.Idle;
				break;

			case EnemyState.SeekLineOfFire:
				if (lineOfFire != LineOfFire.BlockedByIndestructible)
				{
					StartRangedHold(enemy, distance);
					return;
				}

				if (enemy.StateTimer > 0.0f)
				{
					MoveRangedCombatState(enemy, dt);
					return;
				}

				enemy.State = EnemyState.Idle;
				break;

			case EnemyState.KeepDistance:
				if (distance >= preferredMin || enemy.StateTimer <= 0.0f)
				{
					StartRangedHold(enemy, distance);
					return;
				}

				MoveRangedCombatState(enemy, dt);
				return;
		}

		if (lineOfFire == LineOfFire.BlockedByIndestructible)
		{
			StartRangedSeekLineOfFire(enemy, distance, directionToPlayer);
			MoveRangedCombatState(enemy, dt);
			return;
		}

		if (distance > preferredMax)
		{
			StartRangedAdvance(enemy, distance, directionToPlayer);
			MoveRangedCombatState(enemy, dt);
			return;
		}

		StartRangedHold(enemy, distance);
		TryRangedAttack(enemy, directionToPlayer, distance, lineOfFire);
	}

	private void MoveRangedCombatState(EnemyAgent enemy, float dt)
	{
		if (dt <= 0.0f ||
			enemy.State is not (EnemyState.AdvanceBurst or EnemyState.SeekLineOfFire or EnemyState.KeepDistance) ||
			enemy.StateTimer <= 0.0f)
		{
			return;
		}

		if (enemy.State == EnemyState.AdvanceBurst &&
			Vector2.DistanceSquared(enemy.Position, enemy.RangedMoveGoal) <= MathF.Pow(MathF.Max(1.0f, Tuning.Enemy.RangedMoveGoalAcceptanceRadius), 2.0f))
		{
			return;
		}

		MoveEnemy(enemy, enemy.RangedMoveDirection, Tuning.Enemy.RangedMoveSpeed, dt);
	}

	private void StartRangedAdvance(EnemyAgent enemy, float distance, Vector2 directionToPlayer)
	{
		float preferredMin = MathF.Max(MathF.Max(0.0f, Tuning.Enemy.RangedDangerClose), Tuning.Enemy.RangedPreferredMin);
		float maxUsefulAdvanceDistance = MathF.Max(0.0f, distance - preferredMin);
		float advanceDistance = MathF.Min(ResolveRangedAdvanceDistance(distance), maxUsefulAdvanceDistance);
		if (advanceDistance <= Tuning.Enemy.RangedMoveGoalAcceptanceRadius)
		{
			StartRangedHold(enemy, distance);
			return;
		}

		float strafeSign = random.Next(0, 2) == 0 ? -1.0f : 1.0f;
		Vector2 strafeDirection = Geometry.Perpendicular(directionToPlayer) * strafeSign;
		Vector2 moveDirection = Geometry.NormalizeOrZero(directionToPlayer + strafeDirection * MathF.Max(0.0f, Tuning.Enemy.RangedAdvanceStrafeWeight));
		enemy.RangedMoveDirection = moveDirection == Vector2.Zero ? directionToPlayer : moveDirection;
		enemy.RangedMoveGoal = enemy.Position + enemy.RangedMoveDirection * advanceDistance;
		enemy.State = EnemyState.AdvanceBurst;
		enemy.StateTimer = ResolveRangedMoveDuration(advanceDistance);
	}

	private void StartRangedHold(EnemyAgent enemy, float distance)
	{
		enemy.State = EnemyState.HoldFire;
		enemy.RangedMoveDirection = Vector2.Zero;
		enemy.RangedMoveGoal = Vector2.Zero;
		enemy.StateTimer = ResolveRangedHoldSeconds(distance);
	}

	private void StartRangedSeekLineOfFire(EnemyAgent enemy, float distance, Vector2 directionToPlayer)
	{
		float strafeSign = random.Next(0, 2) == 0 ? -1.0f : 1.0f;
		Vector2 strafeDirection = Geometry.Perpendicular(directionToPlayer) * strafeSign;
		float forwardWeight = distance > MathF.Max(Tuning.Enemy.RangedPreferredMin, Tuning.Enemy.RangedDangerClose)
			? MathF.Max(0.0f, Tuning.Enemy.RangedSeekForwardWeight)
			: 0.0f;
		enemy.RangedMoveDirection = Geometry.NormalizeOrZero(strafeDirection + directionToPlayer * forwardWeight);
		enemy.RangedMoveGoal = Vector2.Zero;
		enemy.State = EnemyState.SeekLineOfFire;
		enemy.StateTimer = RandomRange(Tuning.Enemy.RangedSeekLineOfFireSeconds, 0.05f);
	}

	private void StartRangedKeepDistance(EnemyAgent enemy, Vector2 directionToPlayer)
	{
		float strafeSign = random.Next(0, 2) == 0 ? -1.0f : 1.0f;
		Vector2 strafeDirection = Geometry.Perpendicular(directionToPlayer) * strafeSign;
		enemy.RangedMoveDirection = Geometry.NormalizeOrZero(-directionToPlayer + strafeDirection * MathF.Max(0.0f, Tuning.Enemy.RangedKeepDistanceStrafeWeight));
		enemy.RangedMoveGoal = Vector2.Zero;
		enemy.State = EnemyState.KeepDistance;
		enemy.StateTimer = RandomRange(Tuning.Enemy.RangedKeepDistanceSeconds, 0.05f);
	}

	private void TryRangedAttack(EnemyAgent enemy, Vector2 directionToPlayer, float distance, LineOfFire lineOfFire)
	{
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

	private float ResolveRangedAdvanceDistance(float distance)
	{
		return distance >= MathF.Max(0.0f, Tuning.Enemy.RangedLongAdvanceThreshold)
			? RandomRange(Tuning.Enemy.RangedLongAdvanceDistance, 0.0f)
			: RandomRange(Tuning.Enemy.RangedMediumAdvanceDistance, 0.0f);
	}

	private float ResolveRangedHoldSeconds(float distance)
	{
		if (distance >= MathF.Max(0.0f, Tuning.Enemy.RangedLongAdvanceThreshold))
		{
			return RandomRange(Tuning.Enemy.RangedLongHoldSeconds, 0.05f);
		}

		if (distance >= MathF.Max(0.0f, Tuning.Enemy.RangedMediumAdvanceThreshold))
		{
			return RandomRange(Tuning.Enemy.RangedMediumHoldSeconds, 0.05f);
		}

		return RandomRange(Tuning.Enemy.RangedPreferredHoldSeconds, 0.05f);
	}

	private float ResolveRangedMoveDuration(float moveDistance)
	{
		float moveSpeed = MathF.Max(1.0f, Tuning.Enemy.RangedMoveSpeed);
		return MathF.Max(0.25f, moveDistance / moveSpeed + 0.25f);
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

	private float RandomRange(FloatRange range, float minValue)
	{
		float min = MathF.Min(range.Min, range.Max);
		float max = MathF.Max(range.Min, range.Max);
		return MathF.Max(minValue, RandomRange(min, max));
	}

	private static float DegreesToRadians(float degrees)
	{
		return degrees * MathF.PI / 180.0f;
	}
}
