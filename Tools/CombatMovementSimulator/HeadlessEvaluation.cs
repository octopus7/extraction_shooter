using System.Globalization;
using System.Numerics;
using System.Text;
using System.Text.Json;

namespace CombatMovementSimulator;

internal static class HeadlessEvaluation
{
	private const float DefaultSeconds = 45.0f;
	private const float DefaultDeltaSeconds = 1.0f / 30.0f;
	private const int DefaultRunsPerCandidate = 8;

	public static int Run(string[] args)
	{
		HeadlessEvaluationOptions options = HeadlessEvaluationOptions.Parse(args);
		DateTime startedAt = DateTime.Now;
		string timestamp = startedAt.ToString("yyyyMMdd_HHmmss", CultureInfo.InvariantCulture);
		string projectDirectory = ResolveProjectDirectory();
		string repoDirectory = ResolveRepoDirectory(projectDirectory);
		string logDirectory = options.LogDirectory ?? Path.Combine(projectDirectory, "evaluation_logs");
		Directory.CreateDirectory(logDirectory);

		CombatTuning baseTuning = CombatTuning.LoadOrDefault(projectDirectory);
		List<EvaluationCandidate> candidates = BuildCandidates(baseTuning);
		List<PlayerPilotProfile> profiles =
		[
			PlayerPilotProfile.BalancedKite,
			PlayerPilotProfile.PressureStrafe,
			PlayerPilotProfile.SurvivalKite,
			PlayerPilotProfile.CoverProbe
		];

		string jsonlPath = Path.Combine(logDirectory, $"{timestamp}_runs.jsonl");
		string summaryPath = Path.Combine(logDirectory, $"{timestamp}_summary.md");
		List<HeadlessRunResult> results = [];
		JsonSerializerOptions jsonOptions = new()
		{
			WriteIndented = false
		};

		using StreamWriter writer = new(jsonlPath, append: false, Encoding.UTF8);
		foreach (EvaluationCandidate candidate in candidates)
		{
			for (int runIndex = 0; runIndex < options.RunsPerCandidate; ++runIndex)
			{
				foreach (PlayerPilotProfile profile in profiles)
				{
					int seed = options.SeedStart + runIndex * 31 + (int)profile * 997;
					HeadlessRunResult result = RunSingle(startedAt, candidate, profile, seed, options);
					results.Add(result);
					writer.WriteLine(JsonSerializer.Serialize(result, jsonOptions));
				}
			}
		}

		List<CandidateSummary> summaries = SummarizeCandidates(results);
		string report = BuildMarkdownReport(startedAt, DateTime.Now, options, jsonlPath, summaryPath, summaries, results);
		File.WriteAllText(summaryPath, report, Encoding.UTF8);
		File.WriteAllText(Path.Combine(repoDirectory, "Docs", "combat_movement_self_eval.md"), report, Encoding.UTF8);
		return 0;
	}

	private static HeadlessRunResult RunSingle(DateTime startedAt, EvaluationCandidate candidate, PlayerPilotProfile profile, int seed, HeadlessEvaluationOptions options)
	{
		SimulationWorld world = new(candidate.CreateTuning(), seed);
		AutomatedPlayerPilot pilot = new(profile, seed);
		CombatRunEvaluator evaluator = new(candidate.Name, profile.ToString(), seed, world, startedAt);

		int steps = Math.Max(1, (int)MathF.Ceiling(options.Seconds / options.DeltaSeconds));
		for (int step = 0; step < steps; ++step)
		{
			SimulationInput input = pilot.NextInput(world);
			world.Update(input, options.DeltaSeconds);
			evaluator.Sample(world, options.DeltaSeconds);

			if (world.Player.Health <= 0.0f || world.Enemies.Count == 0)
			{
				break;
			}
		}

		return evaluator.Complete(world);
	}

	private static List<EvaluationCandidate> BuildCandidates(CombatTuning baseTuning)
	{
		return
		[
			new EvaluationCandidate("Baseline", "현재 combat_tuning.json 값", () => CloneTuning(baseTuning)),
			new EvaluationCandidate("DisciplinedOpeningFire", "첫 사격 후 성급한 접근을 줄이고 원거리 정지 교전 비율을 높인 후보", () =>
			{
				CombatTuning tuning = CloneTuning(baseTuning);
				tuning.Enemy.RangedPreferredMax = MathF.Max(tuning.Enemy.RangedPreferredMax, 1150.0f);
				tuning.Enemy.RangedLongAdvanceDistance = new FloatRange { Min = 280.0f, Max = 440.0f };
				tuning.Enemy.RangedMediumAdvanceDistance = new FloatRange { Min = 180.0f, Max = 320.0f };
				tuning.Enemy.RangedAdvanceStrafeWeight = MathF.Max(tuning.Enemy.RangedAdvanceStrafeWeight, 0.34f);
				tuning.Enemy.RangedAttackRange = MathF.Max(tuning.Enemy.RangedAttackRange, 1600.0f);
				return tuning;
			}),
			new EvaluationCandidate("ReadablePressure", "플랭커와 총기 적이 조금 더 빨리 화선을 만들되 근접 압박은 과하지 않게 제한한 후보", () =>
			{
				CombatTuning tuning = CloneTuning(baseTuning);
				tuning.Enemy.CombatVisionAngleDegrees = MathF.Max(tuning.Enemy.CombatVisionAngleDegrees, 115.0f);
				tuning.Enemy.RangedPreferredMax = MathF.Max(tuning.Enemy.RangedPreferredMax, 1080.0f);
				tuning.Enemy.RangedFireCooldown = MathF.Max(0.72f, tuning.Enemy.RangedFireCooldown);
				tuning.Enemy.FlankerPreferredMax = MathF.Max(tuning.Enemy.FlankerPreferredMax, 960.0f);
				tuning.Enemy.FlankerAttackRange = MathF.Max(tuning.Enemy.FlankerAttackRange, 1230.0f);
				return tuning;
			})
		];
	}

	private static CombatTuning CloneTuning(CombatTuning tuning)
	{
		string json = JsonSerializer.Serialize(tuning);
		return JsonSerializer.Deserialize<CombatTuning>(json) ?? new CombatTuning();
	}

	private static List<CandidateSummary> SummarizeCandidates(List<HeadlessRunResult> results)
	{
		return results
			.GroupBy(result => result.Candidate)
			.Select(group =>
			{
				List<HeadlessRunResult> groupResults = group.ToList();
				float averageScore = groupResults.Average(result => result.Score);
				return new CandidateSummary
				{
					Candidate = group.Key,
					Runs = groupResults.Count,
					AverageScore = averageScore,
					AveragePlayerHealth = groupResults.Average(result => result.PlayerHealth),
					AverageEnemiesKilled = (float)groupResults.Average(result => result.EnemiesKilled),
					AverageDamageToPlayer = groupResults.Average(result => result.DamageToPlayer),
					AverageEnemyShots = (float)groupResults.Average(result => result.EnemyProjectilesFired),
					AverageOpeningShotDelay = AveragePositive(groupResults.Select(result => result.OpeningEnemyShotDelaySeconds)),
					AverageRangedHoldRatio = groupResults.Average(result => result.RangedHoldRatio),
					AverageRangedAdvanceRatio = groupResults.Average(result => result.RangedAdvanceRatio),
					AverageDangerCloseRatio = groupResults.Average(result => result.DangerCloseRatio),
					SurvivalRate = groupResults.Count(result => result.PlayerSurvived) / (float)Math.Max(1, groupResults.Count),
					Evaluation = DescribeCandidate(groupResults, averageScore)
				};
			})
			.OrderByDescending(summary => summary.AverageScore)
			.ToList();
	}

	private static float AveragePositive(IEnumerable<float> values)
	{
		List<float> positiveValues = values.Where(value => value >= 0.0f).ToList();
		return positiveValues.Count == 0 ? -1.0f : positiveValues.Average();
	}

	private static string DescribeCandidate(List<HeadlessRunResult> results, float averageScore)
	{
		float holdRatio = results.Average(result => result.RangedHoldRatio);
		float advanceRatio = results.Average(result => result.RangedAdvanceRatio);
		float dangerRatio = results.Average(result => result.DangerCloseRatio);
		float averageHealth = results.Average(result => result.PlayerHealth);

		if (averageHealth <= 0.0f || dangerRatio > 0.28f)
		{
			return "압박이 과하거나 플레이어 생존성이 낮아 본 튜닝으로 바로 올리기에는 위험하다.";
		}

		if (holdRatio >= 0.32f && advanceRatio <= 0.22f && averageScore >= 45.0f)
		{
			return "원거리 적이 접근보다 정지 교전을 우선하는 비율이 충분하고 위험 근접 시간이 낮아 우선 후보로 볼 수 있다.";
		}

		if (holdRatio < 0.22f)
		{
			return "정지 교전 시간이 부족해 여전히 이동 중심으로 보일 가능성이 있다.";
		}

		return "치명적인 문제는 없지만 기준 지표가 애매하므로 수동 플레이 감각 검증이 필요하다.";
	}

	private static string BuildMarkdownReport(
		DateTime startedAt,
		DateTime endedAt,
		HeadlessEvaluationOptions options,
		string jsonlPath,
		string summaryPath,
		List<CandidateSummary> summaries,
		List<HeadlessRunResult> results)
	{
		TimeSpan elapsed = endedAt - startedAt;
		CandidateSummary best = summaries[0];
		StringBuilder builder = new();
		builder.AppendLine("# Combat Movement Self Evaluation");
		builder.AppendLine();
		builder.AppendLine($"- 시작: `{startedAt:yyyy-MM-dd HH:mm:ss}`");
		builder.AppendLine($"- 종료: `{endedAt:yyyy-MM-dd HH:mm:ss}`");
		builder.AppendLine($"- 실제 소요: `{elapsed:hh\\:mm\\:ss}`");
		builder.AppendLine($"- 후보 수: `{summaries.Count}`");
		builder.AppendLine($"- 후보별 반복: `{options.RunsPerCandidate}`");
		builder.AppendLine($"- 플레이어 자동 조종 프로필: `BalancedKite`, `PressureStrafe`, `SurvivalKite`, `CoverProbe`");
		builder.AppendLine($"- 시뮬레이션 길이: `run당 {options.Seconds:0.#}초`, `dt {options.DeltaSeconds:0.####}초`");
		builder.AppendLine($"- 총 표본 실행: `{results.Count}`");
		builder.AppendLine($"- 총 시뮬레이션 전투 시간: `{TimeSpan.FromSeconds(results.Sum(result => result.SimulatedSeconds)):hh\\:mm\\:ss}`");
		builder.AppendLine($"- 상세 로그: `{jsonlPath}`");
		builder.AppendLine($"- 요약 로그: `{summaryPath}`");
		builder.AppendLine();
		builder.AppendLine("## 구조 판단");
		builder.AppendLine();
		builder.AppendLine("- 제작 쪽은 `EvaluationCandidate`가 튜닝 후보를 생성한다.");
		builder.AppendLine("- 플레이어 쪽은 `AutomatedPlayerPilot`이 `SimulationInput`만 만든다.");
		builder.AppendLine("- 평가 쪽은 `CombatRunEvaluator`가 월드 상태와 텔레메트리만 읽고, 플레이어 파일럿의 의사결정 내부값은 보지 않는다.");
		builder.AppendLine("- 따라서 만드는 에이전트와 평가 에이전트는 코드 레벨에서 독립되어 있고, 판단 근거는 JSONL 로그로 재검증 가능하다.");
		builder.AppendLine();
		builder.AppendLine("## 후보 요약");
		builder.AppendLine();
		builder.AppendLine("| 후보 | 평균점수 | 생존율 | 평균HP | 평균킬 | 받은피해 | 적 사격 | 원거리 첫 사격 지연 | Hold 비율 | Advance 비율 | 근접위험 | 판단 |");
		builder.AppendLine("|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|");
		foreach (CandidateSummary summary in summaries)
		{
			builder.AppendLine(
				$"| {summary.Candidate} | {summary.AverageScore:0.0} | {summary.SurvivalRate:P0} | {summary.AveragePlayerHealth:0.0} | {summary.AverageEnemiesKilled:0.0} | {summary.AverageDamageToPlayer:0.0} | {summary.AverageEnemyShots:0.0} | {FormatSeconds(summary.AverageOpeningShotDelay)} | {summary.AverageRangedHoldRatio:P0} | {summary.AverageRangedAdvanceRatio:P0} | {summary.AverageDangerCloseRatio:P0} | {summary.Evaluation} |");
		}

		builder.AppendLine();
		builder.AppendLine("## 현재 결론");
		builder.AppendLine();
		builder.AppendLine($"- 1차 자동 평가 기준 최상위 후보: `{best.Candidate}`");
		builder.AppendLine($"- 판단 근거는 평균 점수 `{best.AverageScore:0.0}`, 원거리 Hold 비율 `{best.AverageRangedHoldRatio:P0}`, Advance 비율 `{best.AverageRangedAdvanceRatio:P0}`, 근접위험 비율 `{best.AverageDangerCloseRatio:P0}`다.");
		builder.AppendLine(best.Candidate == "Baseline"
			? "- 후보 비교 결과 현재 `combat_tuning.json` 값이 가장 안정적이므로 자동 튜닝 반영은 보류하고 현재값을 유지한다."
			: $"- `{best.Candidate}`가 최상위지만 자동 점수만으로 본 튜닝 파일을 덮어쓰지는 않는다.");
		builder.AppendLine("- 다만 이 점수는 사람의 재미 판단이 아니라 반복 가능한 전투 리듬 지표다. 최종 채택 전에는 GUI에서 수동 감각 검증이 필요하다.");
		builder.AppendLine();
		builder.AppendLine("## 평가식");
		builder.AppendLine();
		builder.AppendLine("- 생존 보너스, 처치 수, 원거리 첫 사격 지연, 원거리 Hold 비율을 가점한다.");
		builder.AppendLine("- 플레이어 피해량, 근접위험 시간, 원거리 Advance 과다, 적 사격이 너무 늦는 경우를 감점한다.");
		builder.AppendLine("- 목표는 적을 약하게 만드는 것이 아니라 `교전 진입 직후 사격`, `과한 즉시 접근 억제`, `거리 유지 상태의 가독성`을 수치로 확인하는 것이다.");
		builder.AppendLine();
		builder.AppendLine("## 표본 실행 상위 8개");
		builder.AppendLine();
		builder.AppendLine("| 후보 | 프로필 | 시드 | 점수 | HP | 킬 | 적 사격 | 원거리 첫 사격 지연 | Hold | Advance | 근접위험 |");
		builder.AppendLine("|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|");
		foreach (HeadlessRunResult result in results.OrderByDescending(result => result.Score).Take(8))
		{
			builder.AppendLine(
				$"| {result.Candidate} | {result.PlayerProfile} | {result.Seed} | {result.Score:0.0} | {result.PlayerHealth:0.0} | {result.EnemiesKilled} | {result.EnemyProjectilesFired} | {FormatSeconds(result.OpeningEnemyShotDelaySeconds)} | {result.RangedHoldRatio:P0} | {result.RangedAdvanceRatio:P0} | {result.DangerCloseRatio:P0} |");
		}

		builder.AppendLine();
		builder.AppendLine("## 다음 개선 후보");
		builder.AppendLine();
		builder.AppendLine("- 점수 상위 후보를 바로 `combat_tuning.json`에 반영하지 않고, GUI에서 사람이 2~3회 플레이해 체감 접근성/압박감을 확인한다.");
		builder.AppendLine("- `CoverProbe`는 1차 버전이라 엄폐 체류 시간과 시야 차단 품질 지표를 다음 단계에서 더 보강한다.");
		builder.AppendLine("- UE 이식 전에는 이 JSONL 지표를 기준으로 `첫 교전 사격까지 시간`, `Hold/Advance 비율`, `근접위험 비율`을 회귀 기준으로 삼는다.");
		return builder.ToString();
	}

	private static string FormatSeconds(float seconds)
	{
		return seconds < 0.0f ? "n/a" : $"{seconds:0.00}s";
	}

	private static float ScoreRun(HeadlessRunResult result)
	{
		float score = 35.0f;
		score += result.PlayerSurvived ? 18.0f : -24.0f;
		score += result.EnemiesKilled * 7.0f;
		score -= result.DamageToPlayer * 0.42f;

		if (result.FirstRangedEngageSeconds >= 0.0f)
		{
			if (result.OpeningEnemyShotDelaySeconds >= 0.0f && result.OpeningEnemyShotDelaySeconds <= 1.25f)
			{
				score += 14.0f;
			}
			else
			{
				score -= 10.0f;
			}
		}

		score += 18.0f * Clamp01(1.0f - MathF.Abs(result.RangedHoldRatio - 0.45f) / 0.45f);
		score -= MathF.Max(0.0f, result.RangedAdvanceRatio - 0.22f) * 45.0f;
		score -= result.DangerCloseRatio * 50.0f;
		score -= result.RangedEnemyProjectilesFired <= 0 ? 18.0f : 0.0f;
		return score;
	}

	private static float Clamp01(float value)
	{
		return Math.Clamp(value, 0.0f, 1.0f);
	}

	private static string ResolveProjectDirectory()
	{
		DirectoryInfo? directory = new(AppContext.BaseDirectory);
		while (directory != null)
		{
			if (File.Exists(Path.Combine(directory.FullName, "CombatMovementSimulator.csproj")))
			{
				return directory.FullName;
			}

			directory = directory.Parent;
		}

		return Directory.GetCurrentDirectory();
	}

	private static string ResolveRepoDirectory(string projectDirectory)
	{
		DirectoryInfo? directory = new(projectDirectory);
		while (directory != null)
		{
			if (File.Exists(Path.Combine(directory.FullName, "Docs", "requests.md")))
			{
				return directory.FullName;
			}

			directory = directory.Parent;
		}

		return Directory.GetCurrentDirectory();
	}

	private sealed class EvaluationCandidate(string name, string description, Func<CombatTuning> createTuning)
	{
		public string Name { get; } = name;
		public string Description { get; } = description;
		public CombatTuning CreateTuning() => createTuning();
	}

	private sealed class HeadlessEvaluationOptions
	{
		public int RunsPerCandidate { get; private init; } = DefaultRunsPerCandidate;
		public float Seconds { get; private init; } = DefaultSeconds;
		public float DeltaSeconds { get; private init; } = DefaultDeltaSeconds;
		public int SeedStart { get; private init; } = 5000;
		public string? LogDirectory { get; private init; }

		public static HeadlessEvaluationOptions Parse(string[] args)
		{
			HeadlessEvaluationOptions options = new();
			foreach (string argument in args)
			{
				if (TryReadInt(argument, "--runs=", out int runs))
				{
					options = options.WithRuns(Math.Max(1, runs));
				}
				else if (TryReadFloat(argument, "--seconds=", out float seconds))
				{
					options = options.WithSeconds(MathF.Max(1.0f, seconds));
				}
				else if (TryReadFloat(argument, "--dt=", out float dt))
				{
					options = options.WithDeltaSeconds(Math.Clamp(dt, 0.005f, 0.1f));
				}
				else if (TryReadInt(argument, "--seed=", out int seed))
				{
					options = options.WithSeedStart(seed);
				}
				else if (argument.StartsWith("--log-dir=", StringComparison.OrdinalIgnoreCase))
				{
					options = options.WithLogDirectory(argument["--log-dir=".Length..].Trim('"'));
				}
			}

			return options;
		}

		private HeadlessEvaluationOptions WithRuns(int runs) => new()
		{
			RunsPerCandidate = runs,
			Seconds = Seconds,
			DeltaSeconds = DeltaSeconds,
			SeedStart = SeedStart,
			LogDirectory = LogDirectory
		};

		private HeadlessEvaluationOptions WithSeconds(float seconds) => new()
		{
			RunsPerCandidate = RunsPerCandidate,
			Seconds = seconds,
			DeltaSeconds = DeltaSeconds,
			SeedStart = SeedStart,
			LogDirectory = LogDirectory
		};

		private HeadlessEvaluationOptions WithDeltaSeconds(float dt) => new()
		{
			RunsPerCandidate = RunsPerCandidate,
			Seconds = Seconds,
			DeltaSeconds = dt,
			SeedStart = SeedStart,
			LogDirectory = LogDirectory
		};

		private HeadlessEvaluationOptions WithSeedStart(int seed) => new()
		{
			RunsPerCandidate = RunsPerCandidate,
			Seconds = Seconds,
			DeltaSeconds = DeltaSeconds,
			SeedStart = seed,
			LogDirectory = LogDirectory
		};

		private HeadlessEvaluationOptions WithLogDirectory(string? logDirectory) => new()
		{
			RunsPerCandidate = RunsPerCandidate,
			Seconds = Seconds,
			DeltaSeconds = DeltaSeconds,
			SeedStart = SeedStart,
			LogDirectory = string.IsNullOrWhiteSpace(logDirectory) ? null : logDirectory
		};

		private static bool TryReadInt(string argument, string prefix, out int value)
		{
			value = 0;
			return argument.StartsWith(prefix, StringComparison.OrdinalIgnoreCase) &&
				int.TryParse(argument[prefix.Length..], NumberStyles.Integer, CultureInfo.InvariantCulture, out value);
		}

		private static bool TryReadFloat(string argument, string prefix, out float value)
		{
			value = 0.0f;
			return argument.StartsWith(prefix, StringComparison.OrdinalIgnoreCase) &&
				float.TryParse(argument[prefix.Length..], NumberStyles.Float, CultureInfo.InvariantCulture, out value);
		}
	}

	private enum PlayerPilotProfile
	{
		BalancedKite,
		PressureStrafe,
		SurvivalKite,
		CoverProbe
	}

	private sealed class AutomatedPlayerPilot(PlayerPilotProfile profile, int seed)
	{
		private readonly float phase = (seed % 1009) * 0.013f;

		public SimulationInput NextInput(SimulationWorld world)
		{
			EnemyAgent? target = SelectTarget(world);
			Vector2 aimWorld = target?.Position ?? world.Player.Position + world.Player.AimDirection * 600.0f;
			Vector2 moveDirection = ComputeMoveDirection(world, target);
			float targetDistance = target == null ? float.PositiveInfinity : Vector2.Distance(world.Player.Position, target.Position);
			LineOfFire lineOfFire = target == null ? LineOfFire.BlockedByIndestructible : world.EvaluateLineOfFire(world.Player.Position, target.Position);

			return new SimulationInput
			{
				AimWorld = aimWorld,
				MoveDirection = moveDirection,
				FireHeld = target != null && lineOfFire != LineOfFire.BlockedByIndestructible && targetDistance <= 2600.0f,
				SprintHeld = ShouldSprint(targetDistance)
			};
		}

		private EnemyAgent? SelectTarget(SimulationWorld world)
		{
			if (world.Enemies.Count == 0)
			{
				return null;
			}

			return world.Enemies
				.OrderBy(enemy =>
				{
					float distance = Vector2.Distance(world.Player.Position, enemy.Position);
					LineOfFire lineOfFire = world.EvaluateLineOfFire(world.Player.Position, enemy.Position);
					float linePenalty = lineOfFire == LineOfFire.BlockedByIndestructible ? 1800.0f : 0.0f;
					float kindPriority = enemy.Kind == EnemyKind.Ranged ? -180.0f : enemy.Kind == EnemyKind.Flanker ? -120.0f : 0.0f;
					return distance + linePenalty + kindPriority + enemy.Health * 0.7f;
				})
				.First();
		}

		private Vector2 ComputeMoveDirection(SimulationWorld world, EnemyAgent? target)
		{
			if (target == null)
			{
				return Vector2.Zero;
			}

			Vector2 playerPosition = world.Player.Position;
			Vector2 threatAway = Vector2.Zero;
			float closestDistance = float.PositiveInfinity;
			foreach (EnemyAgent enemy in world.Enemies)
			{
				Vector2 away = playerPosition - enemy.Position;
				float distance = away.Length();
				if (distance < 0.001f)
				{
					continue;
				}

				closestDistance = MathF.Min(closestDistance, distance);
				float weight = 1.0f / MathF.Max(160.0f, distance);
				threatAway += away / distance * weight;
			}

			Vector2 awayDirection = Geometry.NormalizeOrZero(threatAway);
			if (awayDirection == Vector2.Zero)
			{
				awayDirection = Geometry.NormalizeOrZero(playerPosition - target.Position);
			}

			Vector2 approachDirection = -awayDirection;
			float side = MathF.Sin(world.ElapsedSeconds * ResolveStrafeFrequency() + phase) >= 0.0f ? 1.0f : -1.0f;
			Vector2 strafeDirection = Geometry.Perpendicular(awayDirection) * side;

			return profile switch
			{
				PlayerPilotProfile.PressureStrafe => ComputePressureMove(closestDistance, approachDirection, awayDirection, strafeDirection),
				PlayerPilotProfile.SurvivalKite => ComputeSurvivalMove(closestDistance, approachDirection, awayDirection, strafeDirection),
				PlayerPilotProfile.CoverProbe => ComputeCoverMove(world, target, closestDistance, approachDirection, awayDirection, strafeDirection),
				_ => ComputeBalancedMove(closestDistance, approachDirection, awayDirection, strafeDirection)
			};
		}

		private float ResolveStrafeFrequency()
		{
			return profile switch
			{
				PlayerPilotProfile.PressureStrafe => 1.25f,
				PlayerPilotProfile.SurvivalKite => 0.72f,
				PlayerPilotProfile.CoverProbe => 0.64f,
				_ => 0.92f
			};
		}

		private static Vector2 ComputeBalancedMove(float distance, Vector2 approach, Vector2 away, Vector2 strafe)
		{
			if (distance < 650.0f)
			{
				return Geometry.NormalizeOrZero(away * 0.95f + strafe * 0.45f);
			}

			if (distance > 1350.0f)
			{
				return Geometry.NormalizeOrZero(approach * 0.6f + strafe * 0.35f);
			}

			return Geometry.NormalizeOrZero(strafe + away * 0.12f);
		}

		private static Vector2 ComputePressureMove(float distance, Vector2 approach, Vector2 away, Vector2 strafe)
		{
			if (distance < 500.0f)
			{
				return Geometry.NormalizeOrZero(away + strafe * 0.35f);
			}

			if (distance > 900.0f)
			{
				return Geometry.NormalizeOrZero(approach * 0.9f + strafe * 0.55f);
			}

			return Geometry.NormalizeOrZero(strafe + approach * 0.2f);
		}

		private static Vector2 ComputeSurvivalMove(float distance, Vector2 approach, Vector2 away, Vector2 strafe)
		{
			if (distance < 900.0f)
			{
				return Geometry.NormalizeOrZero(away + strafe * 0.55f);
			}

			if (distance > 1750.0f)
			{
				return Geometry.NormalizeOrZero(approach * 0.45f + strafe * 0.25f);
			}

			return Geometry.NormalizeOrZero(strafe * 0.9f + away * 0.25f);
		}

		private static Vector2 ComputeCoverMove(SimulationWorld world, EnemyAgent target, float distance, Vector2 approach, Vector2 away, Vector2 strafe)
		{
			if (distance < 560.0f)
			{
				return Geometry.NormalizeOrZero(away + strafe * 0.45f);
			}

			Obstacle? cover = world.Obstacles
				.Where(obstacle => obstacle.Kind == ObstacleKind.Indestructible)
				.OrderBy(obstacle =>
				{
					float playerDistance = Vector2.Distance(world.Player.Position, obstacle.Center);
					float targetDistance = Vector2.Distance(target.Position, obstacle.Center);
					return playerDistance + targetDistance * 0.35f;
				})
				.FirstOrDefault();

			if (cover == null || distance > 1900.0f)
			{
				return ComputeBalancedMove(distance, approach, away, strafe);
			}

			Vector2 coverFromTarget = Geometry.NormalizeOrZero(cover.Center - target.Position);
			if (coverFromTarget == Vector2.Zero)
			{
				coverFromTarget = away;
			}

			float coverPadding = MathF.Max(cover.Size.X, cover.Size.Y) * 0.5f + world.Tuning.Player.Radius + 140.0f;
			Vector2 desiredPosition = cover.Center + coverFromTarget * coverPadding;
			Vector2 toDesired = desiredPosition - world.Player.Position;
			if (toDesired.LengthSquared() > 140.0f * 140.0f)
			{
				return Geometry.NormalizeOrZero(toDesired + strafe * 0.18f);
			}

			return Geometry.NormalizeOrZero(strafe + away * 0.25f);
		}

		private bool ShouldSprint(float targetDistance)
		{
			return profile switch
			{
				PlayerPilotProfile.PressureStrafe => targetDistance < 520.0f || targetDistance > 1500.0f,
				PlayerPilotProfile.SurvivalKite => targetDistance < 1050.0f,
				PlayerPilotProfile.CoverProbe => targetDistance < 700.0f || targetDistance > 1650.0f,
				_ => targetDistance < 720.0f || targetDistance > 1750.0f
			};
		}
	}

	private sealed class CombatRunEvaluator
	{
		private readonly string candidate;
		private readonly string playerProfile;
		private readonly int seed;
		private readonly DateTime startedAt;
		private readonly int initialEnemyCount;
		private readonly Dictionary<EnemyKind, int> initialEnemyCounts;
		private readonly Dictionary<string, float> stateSeconds = [];
		private readonly Dictionary<string, float> kindCombatSeconds = [];
		private readonly Dictionary<string, float> kindDistanceSums = [];
		private readonly Dictionary<string, int> kindDistanceSamples = [];
		private float simulatedSeconds;
		private float dangerCloseSeconds;
		private float firstRangedEngageSeconds = -1.0f;
		private float firstEnemyShotSeconds = -1.0f;
		private float firstRangedEnemyShotSeconds = -1.0f;
		private float firstPlayerDamageSeconds = -1.0f;
		private int previousEnemyProjectiles;
		private int previousRangedEnemyProjectiles;
		private int previousPlayerDamageEvents;

		public CombatRunEvaluator(string candidate, string playerProfile, int seed, SimulationWorld world, DateTime startedAt)
		{
			this.candidate = candidate;
			this.playerProfile = playerProfile;
			this.seed = seed;
			this.startedAt = startedAt;
			initialEnemyCount = world.Enemies.Count;
			initialEnemyCounts = world.Enemies
				.GroupBy(enemy => enemy.Kind)
				.ToDictionary(group => group.Key, group => group.Count());
			previousEnemyProjectiles = world.Telemetry.EnemyProjectilesFired;
			previousRangedEnemyProjectiles = world.Telemetry.RangedEnemyProjectilesFired;
			previousPlayerDamageEvents = world.Telemetry.PlayerDamageEvents;
		}

		public void Sample(SimulationWorld world, float dt)
		{
			simulatedSeconds = world.ElapsedSeconds;
			if (world.Telemetry.EnemyProjectilesFired > previousEnemyProjectiles && firstEnemyShotSeconds < 0.0f)
			{
				firstEnemyShotSeconds = world.ElapsedSeconds;
			}

			if (world.Telemetry.RangedEnemyProjectilesFired > previousRangedEnemyProjectiles && firstRangedEnemyShotSeconds < 0.0f)
			{
				firstRangedEnemyShotSeconds = world.ElapsedSeconds;
			}

			if (world.Telemetry.PlayerDamageEvents > previousPlayerDamageEvents && firstPlayerDamageSeconds < 0.0f)
			{
				firstPlayerDamageSeconds = world.ElapsedSeconds;
			}

			previousEnemyProjectiles = world.Telemetry.EnemyProjectilesFired;
			previousRangedEnemyProjectiles = world.Telemetry.RangedEnemyProjectilesFired;
			previousPlayerDamageEvents = world.Telemetry.PlayerDamageEvents;

			float closestDistance = float.PositiveInfinity;
			foreach (EnemyAgent enemy in world.Enemies)
			{
				float distance = Vector2.Distance(world.Player.Position, enemy.Position);
				closestDistance = MathF.Min(closestDistance, distance);
				string kind = enemy.Kind.ToString();
				kindDistanceSums[kind] = kindDistanceSums.GetValueOrDefault(kind) + distance;
				kindDistanceSamples[kind] = kindDistanceSamples.GetValueOrDefault(kind) + 1;

				if (enemy.IsCombatEngaged)
				{
					kindCombatSeconds[kind] = kindCombatSeconds.GetValueOrDefault(kind) + dt;
					if (enemy.Kind == EnemyKind.Ranged && firstRangedEngageSeconds < 0.0f)
					{
						firstRangedEngageSeconds = world.ElapsedSeconds;
					}
				}

				string stateKey = $"{kind}.{enemy.State}";
				stateSeconds[stateKey] = stateSeconds.GetValueOrDefault(stateKey) + dt;
			}

			if (closestDistance <= MathF.Max(world.Tuning.Enemy.RangedDangerClose, world.Tuning.Enemy.FlankerDangerClose))
			{
				dangerCloseSeconds += dt;
			}
		}

		public HeadlessRunResult Complete(SimulationWorld world)
		{
			Dictionary<EnemyKind, int> finalEnemyCounts = world.Enemies
				.GroupBy(enemy => enemy.Kind)
				.ToDictionary(group => group.Key, group => group.Count());
			int enemiesKilled = Math.Max(0, initialEnemyCount - world.Enemies.Count);
			float rangedCombatSeconds = kindCombatSeconds.GetValueOrDefault(nameof(EnemyKind.Ranged));
			float rangedHoldSeconds = stateSeconds.GetValueOrDefault($"{nameof(EnemyKind.Ranged)}.{nameof(EnemyState.HoldFire)}");
			float rangedAdvanceSeconds = stateSeconds.GetValueOrDefault($"{nameof(EnemyKind.Ranged)}.{nameof(EnemyState.AdvanceBurst)}");
			HeadlessRunResult result = new()
			{
				Timestamp = startedAt.ToString("yyyy-MM-dd HH:mm:ss", CultureInfo.InvariantCulture),
				Candidate = candidate,
				PlayerProfile = playerProfile,
				Seed = seed,
				SimulatedSeconds = simulatedSeconds,
				PlayerSurvived = world.Player.Health > 0.0f,
				PlayerHealth = world.Player.Health,
				InitialEnemies = initialEnemyCount,
				RemainingEnemies = world.Enemies.Count,
				EnemiesKilled = enemiesKilled,
				PlayerProjectilesFired = world.Telemetry.PlayerProjectilesFired,
				EnemyProjectilesFired = world.Telemetry.EnemyProjectilesFired,
				RangedEnemyProjectilesFired = world.Telemetry.RangedEnemyProjectilesFired,
				FlankerEnemyProjectilesFired = world.Telemetry.FlankerEnemyProjectilesFired,
				PlayerDamageEvents = world.Telemetry.PlayerDamageEvents,
				EnemyDamageEvents = world.Telemetry.EnemyDamageEvents,
				DamageToPlayer = world.Telemetry.DamageToPlayer,
				DamageToEnemies = world.Telemetry.DamageToEnemies,
				FirstRangedEngageSeconds = firstRangedEngageSeconds,
				FirstEnemyShotSeconds = firstEnemyShotSeconds,
				FirstRangedEnemyShotSeconds = firstRangedEnemyShotSeconds,
				OpeningEnemyShotDelaySeconds = firstRangedEngageSeconds >= 0.0f && firstRangedEnemyShotSeconds >= 0.0f
					? firstRangedEnemyShotSeconds - firstRangedEngageSeconds
					: -1.0f,
				FirstPlayerDamageSeconds = firstPlayerDamageSeconds,
				RangedHoldRatio = rangedCombatSeconds <= 0.0f ? 0.0f : rangedHoldSeconds / rangedCombatSeconds,
				RangedAdvanceRatio = rangedCombatSeconds <= 0.0f ? 0.0f : rangedAdvanceSeconds / rangedCombatSeconds,
				DangerCloseRatio = simulatedSeconds <= 0.0f ? 0.0f : dangerCloseSeconds / simulatedSeconds,
				AverageRangedDistance = AverageDistance(nameof(EnemyKind.Ranged)),
				AverageFlankerDistance = AverageDistance(nameof(EnemyKind.Flanker)),
				StateSeconds = stateSeconds.OrderBy(pair => pair.Key).ToDictionary(pair => pair.Key, pair => pair.Value),
				InitialEnemyCounts = initialEnemyCounts.ToDictionary(pair => pair.Key.ToString(), pair => pair.Value),
				FinalEnemyCounts = finalEnemyCounts.ToDictionary(pair => pair.Key.ToString(), pair => pair.Value)
			};
			result.Score = ScoreRun(result);
			result.Evaluation = DescribeRun(result);
			return result;
		}

		private float AverageDistance(string kind)
		{
			int samples = kindDistanceSamples.GetValueOrDefault(kind);
			return samples <= 0 ? -1.0f : kindDistanceSums.GetValueOrDefault(kind) / samples;
		}

		private static string DescribeRun(HeadlessRunResult result)
		{
			if (!result.PlayerSurvived)
			{
				return "플레이어가 사망해 압박이 과한 표본이다.";
			}

			if (result.OpeningEnemyShotDelaySeconds is >= 0.0f and <= 1.25f &&
				result.RangedHoldRatio >= 0.30f &&
				result.RangedAdvanceRatio <= 0.22f)
			{
				return "첫 사격과 정지 교전 리듬이 목표에 가깝다.";
			}

			if (result.OpeningEnemyShotDelaySeconds < 0.0f)
			{
				return "원거리 적 첫 사격이 관측되지 않아 인지/화선/사거리 재검토가 필요하다.";
			}

			if (result.RangedAdvanceRatio > 0.28f)
			{
				return "원거리 접근 비율이 높아 즉시 접근 느낌이 남을 수 있다.";
			}

			return "치명적 문제는 없지만 수동 감각 검증이 필요하다.";
		}
	}

	private sealed class HeadlessRunResult
	{
		public string Timestamp { get; set; } = "";
		public string Candidate { get; set; } = "";
		public string PlayerProfile { get; set; } = "";
		public int Seed { get; set; }
		public float SimulatedSeconds { get; set; }
		public bool PlayerSurvived { get; set; }
		public float PlayerHealth { get; set; }
		public int InitialEnemies { get; set; }
		public int RemainingEnemies { get; set; }
		public int EnemiesKilled { get; set; }
		public int PlayerProjectilesFired { get; set; }
		public int EnemyProjectilesFired { get; set; }
		public int RangedEnemyProjectilesFired { get; set; }
		public int FlankerEnemyProjectilesFired { get; set; }
		public int PlayerDamageEvents { get; set; }
		public int EnemyDamageEvents { get; set; }
		public float DamageToPlayer { get; set; }
		public float DamageToEnemies { get; set; }
		public float FirstRangedEngageSeconds { get; set; }
		public float FirstEnemyShotSeconds { get; set; }
		public float FirstRangedEnemyShotSeconds { get; set; }
		public float OpeningEnemyShotDelaySeconds { get; set; }
		public float FirstPlayerDamageSeconds { get; set; }
		public float RangedHoldRatio { get; set; }
		public float RangedAdvanceRatio { get; set; }
		public float DangerCloseRatio { get; set; }
		public float AverageRangedDistance { get; set; }
		public float AverageFlankerDistance { get; set; }
		public Dictionary<string, float> StateSeconds { get; set; } = [];
		public Dictionary<string, int> InitialEnemyCounts { get; set; } = [];
		public Dictionary<string, int> FinalEnemyCounts { get; set; } = [];
		public float Score { get; set; }
		public string Evaluation { get; set; } = "";
	}

	private sealed class CandidateSummary
	{
		public string Candidate { get; set; } = "";
		public int Runs { get; set; }
		public float AverageScore { get; set; }
		public float SurvivalRate { get; set; }
		public float AveragePlayerHealth { get; set; }
		public float AverageEnemiesKilled { get; set; }
		public float AverageDamageToPlayer { get; set; }
		public float AverageEnemyShots { get; set; }
		public float AverageOpeningShotDelay { get; set; }
		public float AverageRangedHoldRatio { get; set; }
		public float AverageRangedAdvanceRatio { get; set; }
		public float AverageDangerCloseRatio { get; set; }
		public string Evaluation { get; set; } = "";
	}
}
