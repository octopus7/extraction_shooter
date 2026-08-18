"""Validate the generated recessed garage-door side-frame OBJ topology."""

from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MODEL_DIR = ROOT / "TunaSweeper" / "SourceArt" / "Environment" / "BunkerGarageDoor" / "Models"
FRAME_FILES = (
    MODEL_DIR / "SM_GarageDoor_FrameLeft.obj",
    MODEL_DIR / "SM_GarageDoor_FrameRight.obj",
)
EXPECTED_BOUNDS = ((-17.5, -22.5, -195.0), (17.5, 22.5, 195.0))
EXPECTED_FACE_COUNT = 34


def subtract(a: tuple[float, ...], b: tuple[float, ...]) -> tuple[float, ...]:
    return tuple(a[index] - b[index] for index in range(3))


def cross(a: tuple[float, ...], b: tuple[float, ...]) -> tuple[float, ...]:
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def dot(a: tuple[float, ...], b: tuple[float, ...]) -> float:
    return sum(a[index] * b[index] for index in range(3))


def validate_frame(path: Path) -> None:
    vertices: list[tuple[float, float, float]] = []
    normals: list[tuple[float, float, float]] = []
    faces: list[list[tuple[int, int, int]]] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("v "):
            vertices.append(tuple(map(float, line.split()[1:4])))
        elif line.startswith("vn "):
            normals.append(tuple(map(float, line.split()[1:4])))
        elif line.startswith("f "):
            faces.append([
                tuple(int(value) - 1 for value in token.split("/"))
                for token in line.split()[1:]
            ])

    if len(faces) != EXPECTED_FACE_COUNT:
        raise ValueError(f"{path.name}: expected {EXPECTED_FACE_COUNT} quads, found {len(faces)}")

    bounds = (
        tuple(min(vertex[axis] for vertex in vertices) for axis in range(3)),
        tuple(max(vertex[axis] for vertex in vertices) for axis in range(3)),
    )
    if bounds != EXPECTED_BOUNDS:
        raise ValueError(f"{path.name}: unexpected bounds {bounds}")

    edge_counts: Counter[tuple[tuple[float, ...], tuple[float, ...]]] = Counter()
    for face_index, face in enumerate(faces):
        if len(face) != 4:
            raise ValueError(f"{path.name}: face {face_index} is not a quad")
        positions = [vertices[vertex_index] for vertex_index, _, _ in face]
        normal = normals[face[0][2]]
        geometric_normal = cross(
            subtract(positions[1], positions[0]),
            subtract(positions[2], positions[0]),
        )
        if dot(geometric_normal, normal) <= 0.0:
            raise ValueError(f"{path.name}: face {face_index} winding opposes its normal")

        for edge_index in range(4):
            start = positions[edge_index]
            end = positions[(edge_index + 1) % 4]
            edge_counts[tuple(sorted((start, end)))] += 1

    open_edges = [(edge, count) for edge, count in edge_counts.items() if count != 2]
    if open_edges:
        raise ValueError(
            f"{path.name}: non-manifold edges={len(open_edges)}; "
            f"samples={open_edges[:8]}"
        )

    print(
        f"VALID {path.name}: faces={len(faces)}, bounds={bounds}, "
        f"winding=consistent, welded_edges=manifold"
    )


def main() -> None:
    for frame_path in FRAME_FILES:
        validate_frame(frame_path)


if __name__ == "__main__":
    main()
