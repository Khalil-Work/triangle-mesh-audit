# Verification evidence

Reproduced by `ctest` (`tests/test_stl_audit.cpp` — the CI gate) and
`python examples/run_verification.py` (builds/runs the C++ demo, renders
figures/GIF/CSVs). Verified with GCC 16.2.0 `-std=c++17 -O2 -Wall -Wextra
-Wpedantic`; CI repeats the build+test on Ubuntu and Windows.

## What each check establishes

| Check | Property established |
| --- | --- |
| Unit-tetrahedron geometry | Surface area = 3/2 + √3/2 and signed volume = 1/6 EXACTLY (outward winding); consistent winding gives star-ratio 1 (origin-enclosing mesh). |
| Equilateral metrics | q = 1 and angles = 60° exactly; area = √3/4 exactly. |
| Translation invariance | The shifted tetrahedron's signed volume is still exactly 1/6 — while its per-face contributions legitimately mix signs (star-ratio < 0.9), proving the ratio measures origin-star-shape, not winding. |
| Flipped-face detection | One inward-wound face moves the signed volume far (≥ 0.5) from the truth — the recommended orientation check. |
| ASCII/binary round trip | Both formats audit identically (float32 tolerance); binary with deliberately BOGUS stored normals returns the same geometry — normals are recomputed from vertices. |
| Invalid inputs | Missing file → TA_ERR_IO; null buffer → TA_ERR_EMPTY; non-finite coordinate → TA_ERR_NONFINITE; truncated binary rejected. |
| Degenerate handling | Zero-area triangles are counted (n_degenerate), never dropped; valid areas unaffected. |

## Recorded run (`docs/results/`)

- `sphere_audit.png` + demo CSVs: 48×96 latitude-longitude unit sphere
  (9,024 triangles): volume within **1.78e-3** of 4π/3 (polygonal
  approximation error at this resolution), area within 8.9e-4 of 4π,
  star-ratio 1.000000, quality max 0.722 (pole-fan triangles honest).
- `disk_refinement.png/.csv`: fan-disk area error vs π across N = 8…1024
  follows the O(h²) inscribed-polygon reference (independent formula
  cross-checking the audit convention).
- `mesh_refinement.gif`: 41 frames, each a computed fan mesh with its
  measured area error.
- `run_summary.json`: metrics + runtime context.

## Limits of this evidence

- Geometry math verified against exact analytic values for simple
  primitives; no test against third-party mesh libraries.
- Binary STL values are float32 by format — sub-1e-6 agreements are not
  meaningful there.
- Not validated: non-manifold detection, duplicate-vertex welding, polygon
  soups with n-gons (out of scope by design).
