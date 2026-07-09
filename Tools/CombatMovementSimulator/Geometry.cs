using System.Numerics;

namespace CombatMovementSimulator;

internal static class Geometry
{
	public static Vector2 NormalizeOrZero(Vector2 value)
	{
		return value.LengthSquared() > 0.0001f ? Vector2.Normalize(value) : Vector2.Zero;
	}

	public static Vector2 Perpendicular(Vector2 value)
	{
		return new Vector2(-value.Y, value.X);
	}

	public static Vector2 Rotate(Vector2 value, float radians)
	{
		float cos = MathF.Cos(radians);
		float sin = MathF.Sin(radians);
		return new Vector2(value.X * cos - value.Y * sin, value.X * sin + value.Y * cos);
	}

	public static Vector2 ToLocal(Vector2 world, Obstacle obstacle)
	{
		return Rotate(world - obstacle.Center, -obstacle.RotationRadians);
	}

	public static Vector2 ToWorld(Vector2 local, Obstacle obstacle)
	{
		return obstacle.Center + Rotate(local, obstacle.RotationRadians);
	}

	public static Vector2[] GetRectangleVertices(Obstacle obstacle)
	{
		Vector2 half = obstacle.Size * 0.5f;
		return
		[
			ToWorld(new Vector2(-half.X, -half.Y), obstacle),
			ToWorld(new Vector2(half.X, -half.Y), obstacle),
			ToWorld(new Vector2(half.X, half.Y), obstacle),
			ToWorld(new Vector2(-half.X, half.Y), obstacle)
		];
	}

	public static bool ContainsPoint(Vector2 point, Obstacle obstacle)
	{
		Vector2 local = ToLocal(point, obstacle);
		Vector2 half = obstacle.Size * 0.5f;
		return MathF.Abs(local.X) <= half.X && MathF.Abs(local.Y) <= half.Y;
	}

	public static bool PushCircleOutOfRectangle(Obstacle obstacle, ref Vector2 circleCenter, float radius)
	{
		Vector2 local = ToLocal(circleCenter, obstacle);
		Vector2 half = obstacle.Size * 0.5f;
		Vector2 closest = new(
			Math.Clamp(local.X, -half.X, half.X),
			Math.Clamp(local.Y, -half.Y, half.Y));

		Vector2 delta = local - closest;
		float distanceSquared = delta.LengthSquared();
		if (distanceSquared > radius * radius)
		{
			return false;
		}

		Vector2 normal;
		float penetration;
		if (distanceSquared > 0.0001f)
		{
			float distance = MathF.Sqrt(distanceSquared);
			normal = delta / distance;
			penetration = radius - distance;
		}
		else
		{
			float distanceToLeft = MathF.Abs(local.X + half.X);
			float distanceToRight = MathF.Abs(half.X - local.X);
			float distanceToBottom = MathF.Abs(local.Y + half.Y);
			float distanceToTop = MathF.Abs(half.Y - local.Y);
			float minDistance = MathF.Min(MathF.Min(distanceToLeft, distanceToRight), MathF.Min(distanceToBottom, distanceToTop));

			if (minDistance == distanceToLeft)
			{
				normal = new Vector2(-1.0f, 0.0f);
				penetration = radius + distanceToLeft;
			}
			else if (minDistance == distanceToRight)
			{
				normal = new Vector2(1.0f, 0.0f);
				penetration = radius + distanceToRight;
			}
			else if (minDistance == distanceToBottom)
			{
				normal = new Vector2(0.0f, -1.0f);
				penetration = radius + distanceToBottom;
			}
			else
			{
				normal = new Vector2(0.0f, 1.0f);
				penetration = radius + distanceToTop;
			}
		}

		circleCenter += Rotate(normal * penetration, obstacle.RotationRadians);
		return true;
	}

	public static bool TrySegmentHitRectangle(Vector2 start, Vector2 end, Obstacle obstacle, out float hitTime, out Vector2 hitPoint)
	{
		Vector2 localStart = ToLocal(start, obstacle);
		Vector2 localEnd = ToLocal(end, obstacle);
		Vector2 direction = localEnd - localStart;
		Vector2 half = obstacle.Size * 0.5f;

		float minTime = 0.0f;
		float maxTime = 1.0f;
		if (!ClipAxis(localStart.X, direction.X, -half.X, half.X, ref minTime, ref maxTime) ||
			!ClipAxis(localStart.Y, direction.Y, -half.Y, half.Y, ref minTime, ref maxTime))
		{
			hitTime = 0.0f;
			hitPoint = Vector2.Zero;
			return false;
		}

		hitTime = Math.Clamp(minTime, 0.0f, 1.0f);
		hitPoint = ToWorld(localStart + direction * hitTime, obstacle);
		return true;
	}

	public static bool TrySegmentHitCircle(Vector2 start, Vector2 end, Vector2 center, float radius, out float hitTime)
	{
		Vector2 segment = end - start;
		Vector2 fromCenter = start - center;
		float a = Vector2.Dot(segment, segment);
		float b = 2.0f * Vector2.Dot(fromCenter, segment);
		float c = Vector2.Dot(fromCenter, fromCenter) - radius * radius;
		float discriminant = b * b - 4.0f * a * c;

		if (a <= 0.0001f || discriminant < 0.0f)
		{
			hitTime = 0.0f;
			return false;
		}

		float sqrt = MathF.Sqrt(discriminant);
		float t1 = (-b - sqrt) / (2.0f * a);
		float t2 = (-b + sqrt) / (2.0f * a);
		if (t1 is >= 0.0f and <= 1.0f)
		{
			hitTime = t1;
			return true;
		}

		if (t2 is >= 0.0f and <= 1.0f)
		{
			hitTime = t2;
			return true;
		}

		hitTime = 0.0f;
		return false;
	}

	private static bool ClipAxis(float start, float direction, float min, float max, ref float minTime, ref float maxTime)
	{
		if (MathF.Abs(direction) < 0.0001f)
		{
			return start >= min && start <= max;
		}

		float inverse = 1.0f / direction;
		float t1 = (min - start) * inverse;
		float t2 = (max - start) * inverse;
		if (t1 > t2)
		{
			(t1, t2) = (t2, t1);
		}

		minTime = MathF.Max(minTime, t1);
		maxTime = MathF.Min(maxTime, t2);
		return minTime <= maxTime;
	}
}
