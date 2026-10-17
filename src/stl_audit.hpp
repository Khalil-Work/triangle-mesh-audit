// triangle-mesh-audit: scoped STL triangle-mesh geometry inspection.
//
// Supported input: ASCII and binary STL files (format auto-detected by the
// "solid" prefix rule). Geometry per triangle: vertices a, b, c; unit
// normal n (recomputed from vertices, not trusted from the file);
// area |(b-a) x (c-a)|/2; signed volume contribution n_z_style (a . (b x
// c))/6 using the CLOSED-surface convention (outward normals) — the total
// signed volume equals the enclosed volume for a closed, consistently
// oriented mesh, and its deviation from |sum of |contributions||/3
// measures orientation inconsistency.
//
// Quality metrics per triangle: shape quality q = 2*sqrt(3)*r_in /
// longest_edge (equilateral = 1; smaller = worse, slivers -> 0);
// min/max angle. Mesh-level statistics: counts, total area, signed
// volume, orientation consistency ratio, quality and angle histograms.
// Degenerate triangles (zero area) are counted, never silently dropped.
//
// Error handling: every entry point returns a ta_result code; inputs are
// validated (null pointers, nonpositive sizes); parse errors report the
// byte offset / line and do not partially populate outputs.

#ifndef STL_AUDIT_HPP
#define STL_AUDIT_HPP

#include <cstddef>

namespace triangle_mesh {

enum ta_result {
    TA_OK = 0,
    TA_ERR_NULL = 1,        // null output pointer
    TA_ERR_IO = 2,          // file could not be opened
    TA_ERR_FORMAT = 3,      // malformed STL (offset/line reported)
    TA_ERR_EMPTY = 4,       // zero triangles parsed
    TA_ERR_NONFINITE = 5    // non-finite vertex coordinate
};

struct ta_stats {
    long n_triangles;
    long n_degenerate;       // zero-area triangles
    double total_area;       // sum of triangle areas
    double signed_volume;    // closed-surface convention: +V for
                             // consistent outward winding
    double abs_volume_sum3;  // sum |per-face origin-tet contributions|
    double star_shape_ratio; // |signed|/abs: 1 iff all contributions share
                             // a sign (star-shaped about the origin); NOT a
                             // general orientation measure
    double aspect_min, aspect_max, aspect_mean;
    double angle_min_deg, angle_max_deg;   // over non-degenerate triangles
};

// Parse the STL file at `path` (ASCII or binary) and fill `stats`.
// Returns a ta_result code; TA_OK on success.
ta_result audit_file(const char* path, ta_stats* stats);

// Same computation from an in-memory buffer of triangle vertex triples:
// vertices[9*i .. 9*i+8] = (ax,ay,az, bx,by,bz, cx,cy,cz).
ta_result audit_vertices(const double* vertices, long n_triangles,
                         ta_stats* stats);

// Write per-triangle CSV (index, area, aspect, angles) for plotting.
ta_result write_triangle_csv(const char* path, const double* vertices,
                             long n_triangles);

// Write the mesh vertices as XYZ rows for plotting.
ta_result write_vertex_csv(const char* path, const double* vertices,
                           long n_triangles);

// Load triangles into a caller-owned buffer of 9*n doubles (validated).
ta_result load_triangles(const char* path, double* vertices,
                         long n_triangles);

// Parse-only: returns the triangle count (>= 0) or a negative ta_result.
long count_triangles(const char* path);

}  // namespace triangle_mesh

#endif  // STL_AUDIT_HPP
