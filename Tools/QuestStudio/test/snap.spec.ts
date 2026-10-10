import { describe, expect, it } from 'vitest';
import { snapPosition } from '../src/client/snap';
const cards = [{ id: 1, x: 100, y: 200, external: false }, { id: 2, x: 400, y: 300, external: false }, { id: 3, x: 110, y: 210, external: true }];
describe('drag alignment', () => {
  it('aligns each axis to the nearest other local card', () => {
    expect(snapPosition(2, 106, 205, 1, cards)).toEqual({ x: 100, y: 200, guideX: 100, guideY: 200 });
    expect(snapPosition(2, 125, 205, 1, cards)).toEqual({ x: 125, y: 200, guideX: null, guideY: 200 });
  });
  it('uses screen distance at different zoom levels and leaves distant movement free', () => {
    expect(snapPosition(2, 114, 220, .5, cards).x).toBe(100);
    expect(snapPosition(2, 106, 220, 2, cards).x).toBe(106);
    expect(snapPosition(2, 125, 225, 1, cards)).toEqual({ x: 125, y: 225, guideX: null, guideY: null });
  });
  it('excludes self and external boundary cards; resolves ties independently of array order', () => {
    expect(snapPosition(2, 401, 301, 1, cards).x).toBe(401);
    expect(snapPosition(2, 116, 216, 1, cards).x).toBe(116);
    const tied = [{ id: 3, x: 106, y: 0 }, { id: 1, x: 94, y: 0 }];
    expect(snapPosition(2, 100, 0, 1, tied).x).toBe(94);
    expect(snapPosition(2, 100, 0, 1, [...tied].reverse()).x).toBe(94);
  });
});
