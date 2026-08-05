/// <reference lib="webworker" />

import type {
  SimulationRequest,
  SimulationResult,
} from "../shared/types";

const worker = self as unknown as DedicatedWorkerGlobalScope;

function makeRandom(seed: number): () => number {
  let value = seed >>> 0 || 1;
  return () => {
    value = (Math.imul(value, 1664525) + 1013904223) >>> 0;
    return value / 0x100000000;
  };
}

function quantile(sorted: number[], fraction: number): number {
  if (sorted.length === 0) return 0;
  const position = (sorted.length - 1) * fraction;
  const lower = Math.floor(position);
  const upper = Math.ceil(position);
  const lowerValue = sorted[lower] ?? 0;
  const upperValue = sorted[upper] ?? lowerValue;
  if (lower === upper) return lowerValue;
  return lowerValue + (upperValue - lowerValue) * (position - lower);
}

worker.onmessage = (event: MessageEvent<SimulationRequest>) => {
  const { dataset, options } = event.data;
  const random = makeRandom(options.seed);
  const placeById = new Map(
    dataset.places.map((place) => [place.id, place]),
  );
  const steps = dataset.steps.filter((step) => step.enabled);
  const distances = steps.map((step) => {
    const from = placeById.get(step.fromPlaceId);
    const to = placeById.get(step.toPlaceId);
    if (!from || !to) return 0;
    return Math.hypot(
      to.xMeters - from.xMeters,
      to.yMeters - from.yMeters,
    );
  });
  const totalDistanceMeters = distances.reduce((sum, value) => sum + value, 0);
  const averageTravelSeconds = steps.reduce(
    (sum, step, index) =>
      sum + (distances[index] ?? 0) / Math.max(0.1, step.moveSpeedMps),
    0,
  );
  const averageActionSeconds = steps.reduce(
    (sum, step) => sum + Math.max(0, step.actionSeconds),
    0,
  );

  const times = Array.from(
    { length: Math.min(Math.max(options.iterations, 1), 100_000) },
    () => {
      let total = averageTravelSeconds;
      for (const step of steps) {
        const deviation =
          (random() * 2 - 1) *
          Math.max(0, step.actionSeconds) *
          (Math.max(0, step.actionVariancePercent) / 100);
        total += Math.max(0, step.actionSeconds + deviation);
      }
      total += Math.max(0, dataset.settings?.mapTransitionSeconds ?? 0);
      return total;
    },
  ).sort((a, b) => a - b);

  const result: SimulationResult = {
    iterations: times.length,
    averageSeconds: times.reduce((sum, value) => sum + value, 0) / times.length,
    p10Seconds: quantile(times, 0.1),
    p50Seconds: quantile(times, 0.5),
    p90Seconds: quantile(times, 0.9),
    minSeconds: times[0] ?? 0,
    maxSeconds: times[times.length - 1] ?? 0,
    totalDistanceMeters,
    averageTravelSeconds,
    averageActionSeconds,
  };
  worker.postMessage(result);
};
