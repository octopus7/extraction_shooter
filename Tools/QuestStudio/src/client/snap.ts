interface AlignmentCard { id: number; x: number; y: number; external?: boolean }
/** Snap within eight screen pixels; hidden chapters and projected boundaries are not anchors. */
export function snapPosition(id: number, x: number, y: number, scale: number, cards: readonly AlignmentCard[]) {
  const threshold = 8 / scale;
  const nearest = (axis: 'x' | 'y', position: number): number | null => {
    let best: number | null = null;
    let distance = threshold;
    for (const card of cards) {
      if (card.id === id || card.external) continue;
      const delta = Math.abs(card[axis] - position);
      if (delta < distance || (delta === distance && (best === null || card[axis] < best))) {
        best = card[axis]; distance = delta;
      }
    }
    return best;
  };
  const guideX = nearest('x', x), guideY = nearest('y', y);
  return { x: guideX ?? x, y: guideY ?? y, guideX, guideY };
}
