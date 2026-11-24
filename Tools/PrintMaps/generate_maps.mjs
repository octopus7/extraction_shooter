import { execFileSync } from "node:child_process";
import { mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const scriptDir = dirname(fileURLToPath(import.meta.url));
const root = resolve(scriptDir, "../..");
const data = JSON.parse(readFileSync(resolve(scriptDir, "print_maps.json"), "utf8"));
const outputDir = resolve(root, "Docs/PrintMaps");
mkdirSync(outputDir, { recursive: true });

const PAGE_W = data.page.width;
const PAGE_H = data.page.height;
const MAP_BOX = { x: 150, y: 280, w: 2120, h: 2020 };
const SIDEBAR = { x: 2420, y: 300, w: 940 };
const COLORS = {
  quest: { fill: "#fff2e8", stroke: "#b85e2e", text: "#7d3718" },
  achievement: { fill: "#edf4fb", stroke: "#34658e", text: "#234966" },
  enemy: { fill: "#fff0f0", stroke: "#a34848", text: "#782f2f" },
  drop: { fill: "#eaf6f2", stroke: "#267562", text: "#185647" },
  travel: { fill: "#f2f2f2", stroke: "#58616a", text: "#343b41" },
};

const esc = (value) => String(value).replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll(">", "&gt;").replaceAll('"', "&quot;");
const text = (x, y, value, size, color = "#222", weight = 400, anchor = "start") =>
  `<text x="${x}" y="${y}" font-family="Malgun Gothic" font-size="${size}" fill="${color}" font-weight="${weight}" text-anchor="${anchor}">${esc(value)}</text>`;

function renderMap(map) {
  const { minX, maxX, minY, maxY } = map.extent;
  const aspect = (maxY - minY) / (maxX - minX);
  let mapW = MAP_BOX.w;
  let mapH = mapW / aspect;
  if (mapH > MAP_BOX.h) {
    mapH = MAP_BOX.h;
    mapW = mapH * aspect;
  }
  const left = MAP_BOX.x + (MAP_BOX.w - mapW) / 2;
  const top = MAP_BOX.y + (MAP_BOX.h - mapH) / 2;
  const xy = (x, y) => ({ x: left + ((y - minY) / (maxY - minY)) * mapW, y: top + ((maxX - x) / (maxX - minX)) * mapH });
  const parts = [];

  parts.push(`<svg xmlns="http://www.w3.org/2000/svg" width="297mm" height="210mm" viewBox="0 0 ${PAGE_W} ${PAGE_H}" font-family="Malgun Gothic">`);
  parts.push(`<rect width="${PAGE_W}" height="${PAGE_H}" fill="#ffffff"/>`);
  parts.push(text(150, 150, map.title, 112, "#1c2730", 700));
  parts.push(text(152, 220, "현장 좌표도  ·  위쪽이 +X  ·  오른쪽이 +Y  ·  Unreal cm", 34, "#58636d", 400));

  parts.push(`<rect x="${left}" y="${top}" width="${mapW}" height="${mapH}" fill="#fff" stroke="#8b969d" stroke-width="4"/>`);
  const gridStep = 5000;
  const startY = Math.ceil(minY / gridStep) * gridStep;
  for (let y = startY; y <= maxY; y += gridStep) {
    const p = xy(minX, y).x;
    parts.push(`<path d="M ${p} ${top} V ${top + mapH}" stroke="#e9edef" stroke-width="2"/>`);
    parts.push(text(p, top + mapH + 30, `${y / 1000}k`, 25, "#869098", 400, "middle"));
  }
  const startX = Math.ceil(minX / gridStep) * gridStep;
  for (let x = startX; x <= maxX; x += gridStep) {
    const p = xy(x, minY).y;
    parts.push(`<path d="M ${left} ${p} H ${left + mapW}" stroke="#e9edef" stroke-width="2"/>`);
    parts.push(text(left - 22, p + 8, `${x / 1000}k`, 25, "#869098", 400, "end"));
  }

  const scaleKm = (maxY - minY) >= 24000 ? 5 : 2;
  const scalePx = (scaleKm * 1000 / (maxY - minY)) * mapW;
  const scaleX = left + 42;
  const scaleY = top + mapH - 45;
  parts.push(`<path d="M ${scaleX} ${scaleY} h ${scalePx}" stroke="#27343d" stroke-width="8"/><path d="M ${scaleX} ${scaleY - 14} v 28 M ${scaleX + scalePx} ${scaleY - 14} v 28" stroke="#27343d" stroke-width="5"/>`);
  parts.push(text(scaleX + scalePx / 2, scaleY - 18, `${scaleKm} km`, 26, "#27343d", 600, "middle"));
  const compassX = left + mapW - 72;
  const compassY = top + 100;
  parts.push(`<path d="M ${compassX} ${compassY + 72} V ${compassY} l -18 30 M ${compassX} ${compassY} l 18 30" fill="none" stroke="#27343d" stroke-width="6" stroke-linecap="round" stroke-linejoin="round"/>`);
  parts.push(text(compassX, compassY - 18, "N", 30, "#27343d", 700, "middle"));

  for (const path of map.paths ?? []) {
    const points = path.points.map(([x, y]) => xy(x, y));
    let d = `M ${points[0].x} ${points[0].y}`;
    if (path.smooth) {
      const segmentCount = path.closed ? points.length : points.length - 1;
      for (let i = 0; i < segmentCount; i++) {
        const current = points[i];
        const next = points[(i + 1) % points.length];
        const previous = points[(i - 1 + points.length) % points.length];
        const after = points[(i + 2) % points.length];
        const c1 = { x: current.x + (next.x - previous.x) / 6, y: current.y + (next.y - previous.y) / 6 };
        const c2 = { x: next.x - (after.x - current.x) / 6, y: next.y - (after.y - current.y) / 6 };
        d += ` C ${c1.x} ${c1.y}, ${c2.x} ${c2.y}, ${next.x} ${next.y}`;
      }
      if (path.closed) d += " Z";
    } else {
      d += points.slice(1).map((point) => ` L ${point.x} ${point.y}`).join("");
      if (path.closed) d += " Z";
    }
    const color = path.kind === "rail" ? "#d6dde1" : "#e5e9eb";
    parts.push(`<path d="${d}" fill="none" stroke="${color}" stroke-width="14" stroke-linejoin="round" stroke-linecap="round"/>`);
  }

  for (const route of map.routes ?? []) {
    const a = xy(route.from[0], route.from[1]);
    const b = xy(route.to[0], route.to[1]);
    parts.push(`<path d="M ${a.x} ${a.y} L ${b.x} ${b.y}" fill="none" stroke="#c8874a" stroke-width="6" stroke-dasharray="17 14" opacity="0.65"/>`);
  }

  const markerLayouts = [];
  const candidates = [[0, 0], [46, -40], [46, 40], [-46, -40], [-46, 40], [0, -66], [0, 66], [76, 0], [-76, 0], [76, -66], [76, 66], [-76, -66], [-76, 66], [0, -108], [0, 108], [120, 0], [-120, 0], [120, -108], [120, 108], [-120, -108], [-120, 108]];
  for (const marker of map.markers) {
    const point = xy(marker.x, marker.y);
    const p = candidates.map(([dx, dy]) => ({ x: point.x + dx, y: point.y + dy }))
      .find((candidate) => candidate.x >= left + 32 && candidate.x <= left + mapW - 32 && candidate.y >= top + 32 && candidate.y <= top + mapH - 32 && markerLayouts.every((placed) => Math.hypot(placed.display.x - candidate.x, placed.display.y - candidate.y) >= 68));
    const display = p ?? point;
    markerLayouts.push({ point, display });
    if (Math.hypot(display.x - point.x, display.y - point.y) > 1) {
      parts.push(`<path d="M ${point.x} ${point.y} L ${display.x} ${display.y}" stroke="#78838b" stroke-width="3"/>`);
      parts.push(`<circle cx="${point.x}" cy="${point.y}" r="7" fill="#4d5a63"/>`);
    }
  }

  map.markers.forEach((marker, index) => {
    const p = markerLayouts[index].display;
    const c = COLORS[marker.category];
    const r = 30;
    parts.push(`<g><title>${esc(`${marker.id} · ${marker.label}`)}</title>`);
    for (const [x, y] of marker.points ?? []) {
      const exact = xy(x, y);
      parts.push(`<circle cx="${exact.x}" cy="${exact.y}" r="8" fill="${c.stroke}" stroke="#fff" stroke-width="2"/>`);
    }
    if (marker.category === "enemy") {
      parts.push(`<path d="M ${p.x} ${p.y - r} L ${p.x + r} ${p.y + r} L ${p.x - r} ${p.y + r} Z" fill="${c.fill}" stroke="${c.stroke}" stroke-width="6"/>`);
    } else if (marker.category === "drop") {
      parts.push(`<rect x="${p.x - r}" y="${p.y - r}" width="${r * 2}" height="${r * 2}" rx="4" fill="${c.fill}" stroke="${c.stroke}" stroke-width="6"/>`);
    } else if (marker.category === "achievement") {
      parts.push(`<path d="M ${p.x} ${p.y - r} L ${p.x + r} ${p.y} L ${p.x} ${p.y + r} L ${p.x - r} ${p.y} Z" fill="${c.fill}" stroke="${c.stroke}" stroke-width="6"/>`);
    } else if (marker.category === "travel") {
      parts.push(`<path d="M ${p.x - r} ${p.y} H ${p.x + r - 4} M ${p.x + r - 22} ${p.y - 18} L ${p.x + r} ${p.y} L ${p.x + r - 22} ${p.y + 18}" fill="none" stroke="${c.stroke}" stroke-width="8" stroke-linecap="round" stroke-linejoin="round"/>`);
    } else {
      parts.push(`<circle cx="${p.x}" cy="${p.y}" r="${r}" fill="${c.fill}" stroke="${c.stroke}" stroke-width="6"/>`);
    }
    const badgeFontSize = marker.id.length > 4 ? 16 : marker.id.length > 3 ? 19 : 23;
    parts.push(`<text x="${p.x}" y="${p.y + 9}" font-family="Malgun Gothic" text-anchor="middle" font-size="${badgeFontSize}" font-weight="700" fill="${c.text}">${esc(marker.id)}</text></g>`);
  });

  parts.push(`<path d="M 2335 280 V 2300" stroke="#d8dee2" stroke-width="3"/>`);
  const sidebarX = SIDEBAR.x;
  parts.push(text(sidebarX, 360, "기호", 44, "#26333b", 700));
  const legend = [
    ["quest", "퀘스트 목표·액터"],
    ["achievement", "도전과제 관련 지점"],
    ["enemy", "적 출현 위치"],
    ["drop", "확정 드랍 위치"],
    ["travel", "지역 이동 지점"],
  ];
  let ly = 430;
  for (const [kind, label] of legend) {
    const c = COLORS[kind];
    const x = sidebarX + 20;
    if (kind === "enemy") parts.push(`<path d="M ${x} ${ly - 22} l 22 40 h -44 z" fill="${c.fill}" stroke="${c.stroke}" stroke-width="5"/>`);
    else if (kind === "drop") parts.push(`<rect x="${x - 18}" y="${ly - 18}" width="36" height="36" fill="${c.fill}" stroke="${c.stroke}" stroke-width="5"/>`);
    else if (kind === "achievement") parts.push(`<path d="M ${x} ${ly - 22} l 22 22 -22 22 -22 -22 z" fill="${c.fill}" stroke="${c.stroke}" stroke-width="5"/>`);
    else if (kind === "travel") parts.push(`<path d="M ${x - 22} ${ly} h 42 m -18 -16 l 18 16 -18 16" fill="none" stroke="${c.stroke}" stroke-width="6"/>`);
    else parts.push(`<circle cx="${x}" cy="${ly}" r="18" fill="${c.fill}" stroke="${c.stroke}" stroke-width="5"/>`);
    parts.push(text(sidebarX + 60, ly + 11, label, 37, "#36424a", 400));
    ly += 70;
  }
  ly += 32;
  for (const section of map.notes) {
    parts.push(text(sidebarX, ly, section.title, 43, "#26333b", 700));
    ly += 58;
    for (const line of section.lines) {
      parts.push(text(sidebarX, ly, line, 36, "#44515a", 400));
      ly += 48;
    }
    ly += 26;
  }

  parts.push(`<path d="M 150 2350 H 3358" stroke="#d8dee2" stroke-width="3"/>`);
  parts.push(text(150, 2415, "배치 조사와 100% 드랍 설정에서 확인된 위치만 표시", 25, "#707b83", 400));
  parts.push(text(3358, 2415, map.sourceMap, 25, "#707b83", 400, "end"));
  parts.push("</svg>");
  return parts.join("\n");
}

for (const map of data.maps) {
  const svgPath = resolve(outputDir, `${map.id}_quest_achievement_drops.svg`);
  const pngPath = resolve(outputDir, `${map.id}_quest_achievement_drops.png`);
  writeFileSync(svgPath, renderMap(map), "utf8");
  execFileSync("magick", ["-density", "300", svgPath, "-background", "white", "-alpha", "remove", "-alpha", "off", pngPath], { stdio: "inherit" });
  console.log(`${map.id}: ${pngPath}`);
}
