# Model and method

## Supported formats

ASCII STL (auto-detected by the `solid` prefix) and binary STL (84-byte
header, 50-byte facet records). The stored facet normals are deliberately
IGNORED — normals are recomputed from the vertices (the tests inject bogus
stored normals and verify unchanged results).

## Per-triangle geometry

For vertices a, b, c:

- **Area** |(b−a) × (c−a)|/2.
- **Signed volume contribution** a·(b×c)/6 — the signed volume of the
  origin-tetrahedron (origin, a, b, c). For a CLOSED, consistently
  OUTWARD-wound surface the sum equals +V (the enclosed volume); the sum
  is translation-invariant while individual terms are not (and may
  legitimately have mixed signs — see the star-shape note below).
- **Shape quality** q = 2√3·r_in/L_max with the inradius r = A/s and s the
  semi-perimeter: q = 1 for an equilateral triangle and → 0 for slivers.
- **Angles** from the law of cosines, per-triangle min/max.

## Mesh statistics

Counts (including degenerate zero-area triangles, which are counted —
never silently dropped), total area, signed volume, Σ|contributions|, and
the **star-shape ratio** |Σsigned| / Σ|contrib|: equals 1 iff every
per-face origin-tet contribution shares one sign — true for meshes that
enclose and are star-shaped about the origin. It is NOT a general
orientation-consistency measure: a correctly wound mesh translated away
from the origin legitimately mixes per-face signs. Orientation is
validated by comparing signed_volume to a known enclosed volume (unit
tetrahedron → 1/6; the demo sphere → 4π/3).

## Error handling

Result codes: TA_OK; TA_ERR_NULL (null output); TA_ERR_IO (file);
TA_ERR_FORMAT (malformed STL — size not ≡ 84+50n for binary, vertex-count
mismatch for ASCII); TA_ERR_EMPTY (no triangles / all degenerate);
TA_ERR_NONFINITE (vertex coordinate). Parse failures never partially
populate outputs.

## Limits

Triangle soups only: no polygon merging, no manifold validation, no
duplicate-vertex welding, no normals output (inspection tool, not a mesh
repair tool). Volume/area values are float32-precision-limited when read
from binary STL (vertices are float32 by format).
