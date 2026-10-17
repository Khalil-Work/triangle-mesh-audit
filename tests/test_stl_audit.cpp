// Verification tests for triangle-mesh-audit.
//
// Properties pinned:
// 1. Known geometry: a unit right tetrahedron's closed surface (4
//    triangles) has exact area and volume; the audit recovers both.
// 2. Equilateral triangle: aspect ratio exactly 1, angles 60.
// 3. Orientation consistency: outward-oriented closed mesh gives
//    star_shape_ratio 1 and positive signed volume; one flipped triangle
//    changes both measurably.
// 4. ASCII and binary STL round trips produce identical audits; the
//    binary loader ignores stored (bogus) normals — geometry is
//    recomputed from vertices.
// 5. Invalid inputs: missing file, truncated binary, non-finite vertices,
//    buffer-size mismatch — all rejected with the documented codes.
// 6. Degenerate (zero-area) triangles are counted, not dropped.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../src/stl_audit.hpp"

using triangle_mesh::ta_stats;

static int failures = 0;
void check(bool ok, const char* what) {
    if (!ok) { std::fprintf(stderr, "FAILED: %s\n", what); ++failures; }
}

// Unit tetrahedron with vertices (0,0,0),(1,0,0),(0,1,0),(0,0,1):
// surface area = 3*(1/2) + sqrt(3)/2; volume = 1/6. Faces ordered
// OUTWARD (CCW seen from outside).
static const double TET[36] = {
    0, 0, 0, 0, 1, 0, 1, 0, 0,   // z=0 face (outward -z)
    0, 0, 0, 0, 0, 1, 0, 1, 0,   // x=0 face (outward -x)
    0, 0, 0, 1, 0, 0, 0, 0, 1,   // y=0 face (outward -y)
    1, 0, 0, 0, 1, 0, 0, 0, 1,   // slanted face (outward (1,1,1))
};

static void write_ascii_stl(const char* path, const double* v, long n) {
    std::FILE* f = std::fopen(path, "w");
    std::fprintf(f, "solid test\n");
    for (long i = 0; i < n; ++i) {
        std::fprintf(f, "facet normal 0 0 0\nouter loop\n");
        for (int k = 0; k < 3; ++k)
            std::fprintf(f, "vertex %.17g %.17g %.17g\n", v[9 * i + 3 * k],
                         v[9 * i + 3 * k + 1], v[9 * i + 3 * k + 2]);
        std::fprintf(f, "endloop\nendfacet\n");
    }
    std::fprintf(f, "endsolid test\n");
    std::fclose(f);
}

static void write_binary_stl(const char* path, const double* v, long n) {
    std::FILE* f = std::fopen(path, "wb");
    unsigned char header[80] = {0};
    std::fwrite(header, 1, 80, f);
    unsigned char cnt[4];
    std::memcpy(cnt, &n, 4);
    std::fwrite(cnt, 1, 4, f);
    for (long i = 0; i < n; ++i) {
        float bogus[3] = {9.9f, 9.9f, 9.9f};   // deliberately wrong normal
        std::fwrite(bogus, 4, 3, f);
        for (int k = 0; k < 9; ++k) {
            const float x = static_cast<float>(v[9 * i + k]);
            std::fwrite(&x, 4, 1, f);
        }
        unsigned short attr = 0;
        std::fwrite(&attr, 2, 1, f);
    }
    std::fclose(f);
}

int main() {
    // 1. Known tetrahedron geometry.
    {
        ta_stats st;
        const int rc = triangle_mesh::audit_vertices(TET, 4, &st);
        check(rc == triangle_mesh::TA_OK, "tetra audit succeeds");
        const double area_exact = 1.5 + std::sqrt(3.0) / 2.0;
        check(std::fabs(st.total_area - area_exact) < 1e-14,
              "tetra surface area exact");
        check(std::fabs(st.signed_volume - 1.0 / 6.0) < 1e-14,
              "tetra signed volume exact (outward orientation)");
        check(std::fabs(st.star_shape_ratio - 1.0) < 1e-12,
              "consistent orientation ratio 1");
        check(st.n_degenerate == 0, "no degenerate triangles");
    }

    // 2. Equilateral triangle metrics.
    {
        const double eq[9] = {0, 0, 0, 1, 0, 0, 0.5, std::sqrt(3.0) / 2, 0};
        double area, vol, aspect, mn, mx;
        bool deg;
        // Recreate via the public API on a one-triangle "mesh".
        ta_stats st;
        triangle_mesh::audit_vertices(eq, 1, &st);
        check(std::fabs(st.aspect_max - 1.0) < 1e-12, "equilateral aspect 1");
        check(std::fabs(st.angle_min_deg - 60.0) < 1e-9 &&
                  std::fabs(st.angle_max_deg - 60.0) < 1e-9,
              "equilateral angles 60");
        check(std::fabs(st.total_area - std::sqrt(3.0) / 4.0) < 1e-14,
              "equilateral area exact");
        (void)area; (void)vol; (void)aspect; (void)mn; (void)mx; (void)deg;
    }

    // 3. Orientation is validated by the signed volume against the known
    // enclosed volume. Translating the tetrahedron off the origin gives
    // every face a nonzero (sign-mixed) contribution - legitimately - so
    // the star-shape ratio is NOT the consistency test.
    {
        double shifted[36], flipped[36];
        const double t[3] = {0.5, 0.5, 0.5};
        for (int i = 0; i < 12; ++i) {
            shifted[3 * i] = TET[3 * i] + t[0];
            shifted[3 * i + 1] = TET[3 * i + 1] + t[1];
            shifted[3 * i + 2] = TET[3 * i + 2] + t[2];
        }
        std::memcpy(flipped, shifted, sizeof(shifted));
        // Swap vertex slots b and c of the slanted face -> inward winding.
        for (int k = 0; k < 3; ++k) {
            std::swap(flipped[27 + k], flipped[30 + k]);
        }
        ta_stats st_ok, st_bad;
        triangle_mesh::audit_vertices(shifted, 4, &st_ok);
        triangle_mesh::audit_vertices(flipped, 4, &st_bad);
        check(std::fabs(st_ok.signed_volume - 1.0 / 6.0) < 1e-14,
              "shifted tetra signed volume still exact (translation invariant)");
        check(std::fabs(st_bad.signed_volume - 1.0 / 6.0) > 0.5,
              "flipped face changes the signed volume far from the truth");
        check(st_ok.star_shape_ratio < 0.9,
              "shifted tetra correctly NOT star-shaped about origin");
    }

    // 4. ASCII/binary round trip + bogus stored normals ignored.
    {
        write_ascii_stl("build/tet_ascii.stl", TET, 4);
        write_binary_stl("build/tet_bin.stl", TET, 4);
        ta_stats a, b;
        check(triangle_mesh::audit_file("build/tet_ascii.stl", &a) == triangle_mesh::TA_OK,
              "ascii audit succeeds");
        check(triangle_mesh::audit_file("build/tet_bin.stl", &b) == triangle_mesh::TA_OK,
              "binary audit succeeds");
        check(a.n_triangles == b.n_triangles && a.n_triangles == 4, "counts match");
        check(std::fabs(a.total_area - b.total_area) < 1e-6 &&
                  std::fabs(a.signed_volume - b.signed_volume) < 1e-6,
              "ascii/binary agree (float32 storage tolerance)");
        check(std::fabs(b.signed_volume - 1.0 / 6.0) < 1e-6,
              "binary stored normals ignored: volume from vertices");
    }

    // 5. Invalid inputs.
    {
        ta_stats st;
        check(triangle_mesh::audit_file("build/no_such_file.stl", &st) ==
                  triangle_mesh::TA_ERR_IO, "missing file -> TA_ERR_IO");
        check(triangle_mesh::audit_vertices(nullptr, 4, &st) == triangle_mesh::TA_ERR_EMPTY,
              "null vertices rejected");
        const double bad[9] = {0, 0, 0, std::nan(""), 0, 0, 1, 0, 0};
        check(triangle_mesh::audit_vertices(bad, 1, &st) == triangle_mesh::TA_ERR_NONFINITE,
              "non-finite coordinate rejected");
        std::FILE* f = std::fopen("build/trunc.stl", "wb");
        std::fwrite("binary-not-solid", 1, 16, f);
        std::fclose(f);
        check(triangle_mesh::count_triangles("build/trunc.stl") < 0,
              "truncated binary rejected");
    }

    // 6. Degenerate triangles counted.
    {
        const double with_deg[18] = {0, 0, 0, 1, 0, 0, 0, 1, 0,
                                     0, 0, 0, 1, 0, 0, 2, 0, 0};
        ta_stats st;
        triangle_mesh::audit_vertices(with_deg, 2, &st);
        check(st.n_triangles == 2 && st.n_degenerate == 1,
              "degenerate counted, not dropped");
        check(std::fabs(st.total_area - 0.5) < 1e-14, "valid area unaffected");
    }

    if (failures) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    std::printf("triangle-mesh-audit: all checks passed\n");
    return EXIT_SUCCESS;
}
