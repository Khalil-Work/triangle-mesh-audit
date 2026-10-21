# API reference (C++)

Header `src/stl_audit.hpp`, namespace `triangle_mesh`.

```cpp
struct ta_stats {
    long n_triangles;
    long n_degenerate;
    double total_area;
    double signed_volume;     // +V for consistent outward winding
    double abs_volume_sum3;   // sum |per-face origin-tet contributions|
    double star_shape_ratio;  // |signed|/abs (see docs/model.md semantics)
    double aspect_min, aspect_max, aspect_mean;   // q = 2*sqrt(3)*r/L
    double angle_min_deg, angle_max_deg;
};
```

- `audit_file(path, &stats) -> ta_result` — parse + audit an STL file.
- `audit_vertices(vertices, n, &stats) -> ta_result` — audit an in-memory
  buffer of 9n doubles (triangle-major, vertex-major, xyz).
- `load_triangles(path, vertices, n) -> ta_result` — parse into a
  caller-owned buffer; fails if the file's count ≠ n.
- `count_triangles(path) -> long` — triangle count, or negative ta_result.
- `write_triangle_csv(path, vertices, n)`, `write_vertex_csv(...)` — CSV
  exports for plotting.

Error codes as documented in docs/model.md; no partial outputs on parse
failure.

## Build

```bash
cmake -S . -B build
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Direct gcc equivalent used by the results script:
`g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic src/stl_demo.cpp src/stl_audit.cpp -o build/stl_demo`.

## Example

```cpp
triangle_mesh::ta_stats st;
const int rc = triangle_mesh::audit_file("mesh.stl", &st);
if (rc == triangle_mesh::TA_OK) {
    std::printf("area=%.6f volume=%.6f q_max=%.3f\n",
                st.total_area, st.signed_volume, st.aspect_max);
}
```
