import { GRAPH_CARD_WIDTH, GRAPH_CARD_HEIGHT } from './graph';
import type { GraphProjection } from './types';

export type DisplayNode = GraphProjection['nodes'][number];
export function cardSize(node: Pick<DisplayNode, 'external'>) {
  const factor = node.external ? 0.85 : 1;
  return { width: GRAPH_CARD_WIDTH * factor, height: GRAPH_CARD_HEIGHT * factor };
}
export function edgeCurve(a: DisplayNode, b: DisplayNode) {
  const x1 = a.x + cardSize(a).width, y1 = a.y + cardSize(a).height / 2;
  const x2 = b.x, y2 = b.y + cardSize(b).height / 2;
  const bend = Math.max(60, Math.abs(x2 - x1) / 2);
  return { x1, y1, c1: x1 + bend, c2: x2 - bend, x2, y2 };
}
