export type AppPage = "quest-viewer" | "simulation";

export function pageFromPath(pathname: string): AppPage {
  return pathname === "/simulation" || pathname.startsWith("/simulation/")
    ? "simulation"
    : "quest-viewer";
}
