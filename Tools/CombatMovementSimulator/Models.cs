using System.Numerics;
using System.Text.Json;

namespace CombatMovementSimulator;

internal enum EnemyKind
{
	Melee,
	Ranged,
	Flanker
}

internal enum EnemyState
{
	Idle,
	Wander,
	AdvanceBurst,
	HoldFire,
	Approach,
	HoldRange,
	Strafe,
	Retreat,
	KeepDistance,
	SeekLineOfFire,
	AttackCommit,
	Recover
}

internal enum ObstacleKind
{
	Destructible,
	Indestructible
}

internal enum ProjectileOwner
{
	Player,
	Enemy
}

internal enum BuildMode
{
	None,
	Destructible,
	Indestructible,
	MeleeEnemy,
	RangedEnemy,
	FlankerEnemy
}

internal enum LineOfFire
{
	Clear,
	BlockedByDestructible,
	BlockedByIndestructible
}

internal sealed class PlayerTuning
{
	public float Radius { get; set; } = 42.0f;
	public float MoveSpeed { get; set; } = 520.0f;
	public float SprintSpeedMultiplier { get; set; } = 2.0f;
	public float Health { get; set; } = 100.0f;
	public float ProjectileSpeed { get; set; } = 2400.0f;
	public float ProjectileDamage { get; set; } = 18.0f;
	public float FireCooldown { get; set; } = 0.105f;
}

internal sealed class FloatRange
{
	public float Min { get; set; }
	public float Max { get; set; }
}

internal sealed class EnemyTuning
{
	public float Radius { get; set; } = 42.0f;
	public float MeleeMoveSpeed { get; set; } = 390.0f;
	public float RangedMoveSpeed { get; set; } = 340.0f;
	public float Health { get; set; } = 70.0f;
	public float TrackingRange { get; set; } = 2300.0f;
	public float CombatDisengageRange { get; set; } = 3600.0f;
	public float CombatVisionAngleDegrees { get; set; } = 100.0f;
	public FloatRange IdleSeconds { get; set; } = new() { Min = 1.4f, Max = 3.7f };
	public FloatRange WanderSeconds { get; set; } = new() { Min = 0.9f, Max = 2.4f };
	public float WanderMoveSpeed { get; set; } = 120.0f;
	public float MeleeAttackRange { get; set; } = 115.0f;
	public float MeleeWindupSeconds { get; set; } = 0.2f;
	public float MeleeRecoverSeconds { get; set; } = 0.45f;
	public float MeleeDamage { get; set; } = 16.0f;
	public float RangedPreferredMin { get; set; } = 650.0f;
	public float RangedPreferredMax { get; set; } = 1000.0f;
	public float RangedDangerClose { get; set; } = 430.0f;
	public float RangedLongAdvanceThreshold { get; set; } = 1600.0f;
	public float RangedMediumAdvanceThreshold { get; set; } = 1200.0f;
	public FloatRange RangedLongAdvanceDistance { get; set; } = new() { Min = 400.0f, Max = 600.0f };
	public FloatRange RangedMediumAdvanceDistance { get; set; } = new() { Min = 250.0f, Max = 400.0f };
	public FloatRange RangedLongHoldSeconds { get; set; } = new() { Min = 1.6f, Max = 2.4f };
	public FloatRange RangedMediumHoldSeconds { get; set; } = new() { Min = 2.4f, Max = 3.6f };
	public FloatRange RangedPreferredHoldSeconds { get; set; } = new() { Min = 2.8f, Max = 4.4f };
	public FloatRange RangedSeekLineOfFireSeconds { get; set; } = new() { Min = 0.8f, Max = 1.4f };
	public FloatRange RangedKeepDistanceSeconds { get; set; } = new() { Min = 0.5f, Max = 0.9f };
	public float RangedAdvanceStrafeWeight { get; set; } = 0.22f;
	public float RangedSeekForwardWeight { get; set; } = 0.25f;
	public float RangedKeepDistanceStrafeWeight { get; set; } = 0.45f;
	public float RangedMoveGoalAcceptanceRadius { get; set; } = 55.0f;
	public float RangedAttackRange { get; set; } = 1450.0f;
	public float RangedProjectileSpeed { get; set; } = 1750.0f;
	public float RangedProjectileDamage { get; set; } = 9.0f;
	public float RangedFireCooldown { get; set; } = 0.75f;
	public float RangedStrafeChangeSeconds { get; set; } = 1.1f;
	public float SeekLineOfFireStrafeWeight { get; set; } = 0.86f;
	public float FlankerMoveSpeed { get; set; } = 370.0f;
	public float FlankerPreferredMin { get; set; } = 560.0f;
	public float FlankerPreferredMax { get; set; } = 880.0f;
	public float FlankerDangerClose { get; set; } = 360.0f;
	public FloatRange FlankerOrbitSeconds { get; set; } = new() { Min = 0.9f, Max = 1.6f };
	public FloatRange FlankerSeekLineOfFireSeconds { get; set; } = new() { Min = 0.7f, Max = 1.2f };
	public FloatRange FlankerKeepDistanceSeconds { get; set; } = new() { Min = 0.45f, Max = 0.8f };
	public FloatRange FlankerAdvanceDistance { get; set; } = new() { Min = 260.0f, Max = 430.0f };
	public float FlankerOrbitRadialCorrectionWeight { get; set; } = 0.55f;
	public float FlankerAdvanceStrafeWeight { get; set; } = 0.85f;
	public float FlankerSeekForwardWeight { get; set; } = 0.18f;
	public float FlankerKeepDistanceStrafeWeight { get; set; } = 0.9f;
	public float FlankerMoveGoalAcceptanceRadius { get; set; } = 55.0f;
	public float FlankerAttackRange { get; set; } = 1150.0f;
	public float FlankerProjectileSpeed { get; set; } = 1650.0f;
	public float FlankerProjectileDamage { get; set; } = 7.0f;
	public float FlankerFireCooldown { get; set; } = 0.65f;
}

internal sealed class WorldTuning
{
	public float ProjectileRadius { get; set; } = 6.0f;
	public float DestructibleObstacleHealth { get; set; } = 70.0f;
	public float SimulationScale { get; set; } = 0.16f;
}

internal sealed class CombatTuning
{
	public PlayerTuning Player { get; set; } = new();
	public EnemyTuning Enemy { get; set; } = new();
	public WorldTuning World { get; set; } = new();

	public static CombatTuning LoadOrDefault(string directory)
	{
		string path = Path.Combine(directory, "combat_tuning.json");
		if (!File.Exists(path))
		{
			return new CombatTuning();
		}

		try
		{
			return JsonSerializer.Deserialize<CombatTuning>(File.ReadAllText(path), new JsonSerializerOptions
			{
				PropertyNameCaseInsensitive = true
			}) ?? new CombatTuning();
		}
		catch
		{
			return new CombatTuning();
		}
	}
}

internal sealed class PlayerAgent
{
	public Vector2 Position { get; set; }
	public Vector2 AimDirection { get; set; } = new(1.0f, 0.0f);
	public float Health { get; set; }
	public float FireCooldownRemaining { get; set; }
}

internal sealed class EnemyAgent
{
	public EnemyKind Kind { get; set; }
	public EnemyState State { get; set; } = EnemyState.Idle;
	public Vector2 Position { get; set; }
	public Vector2 Facing { get; set; } = new(1.0f, 0.0f);
	public float Health { get; set; }
	public float FireCooldownRemaining { get; set; }
	public float StateTimer { get; set; }
	public float StrafeTimer { get; set; }
	public float StrafeSign { get; set; } = 1.0f;
	public Vector2 RangedMoveDirection { get; set; }
	public Vector2 RangedMoveGoal { get; set; }
	public bool HasAppliedAttack { get; set; }
	public bool IsCombatEngaged { get; set; }
	public bool IsOpeningHold { get; set; }
}

internal sealed class Obstacle
{
	public ObstacleKind Kind { get; set; }
	public Vector2 Center { get; set; }
	public Vector2 Size { get; set; }
	public float RotationRadians { get; set; }
	public float Health { get; set; }

	public bool IsDestroyed => Kind == ObstacleKind.Destructible && Health <= 0.0f;
}

internal sealed class Projectile
{
	public ProjectileOwner Owner { get; set; }
	public Vector2 Position { get; set; }
	public Vector2 PreviousPosition { get; set; }
	public Vector2 Velocity { get; set; }
	public float Damage { get; set; }
	public float Radius { get; set; }
	public float LifeSeconds { get; set; } = 1.6f;
}

internal sealed class SimulationInput
{
	public Vector2 MoveDirection { get; set; }
	public Vector2 AimWorld { get; set; }
	public bool FireHeld { get; set; }
	public bool SprintHeld { get; set; }
}
