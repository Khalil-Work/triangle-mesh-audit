// Implementation of triangle-mesh-audit. See stl_audit.hpp for the API.
#include "stl_audit.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace triangle_mesh {

namespace {

struct Vec3 {
    double x, y, z;
};

inline Vec3 sub(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

bool finite_vec(const Vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

void stats_for_triangle(const Vec3& a, const Vec3& b, const Vec3& c,
                        double& area, double& vol_contrib, double& aspect,
                        double& min_ang_deg, double& max_ang_deg,
                        bool& degenerate) {
    const Vec3 ab = sub(b, a), ac = sub(c, a), bc = sub(c, b);
    const Vec3 n = cross(ab, ac);
    area = 0.5 * std::sqrt(dot(n, n));
    vol_contrib = dot(a, cross(b, c)) / 6.0;
    degenerate = !(area > 0.0);
    if (degenerate) {
        aspect = min_ang_deg = max_ang_deg = std::nan("");
        return;
    }
    // Edge lengths and inradius: r = A / s with s the semi-perimeter.
    const double lab = std::sqrt(dot(ab, ab));
    const double lac = std::sqrt(dot(ac, ac));
    const double lbc = std::sqrt(dot(bc, bc));
    const double s = 0.5 * (lab + lac + lbc);
    const double r = area / s;
    const double longest = std::max(lab, std::max(lac, lbc));
    // Shape quality q = 2*sqrt(3)*r_in / longest: 1 for an equilateral
    // triangle, smaller = worse (sliver quality measure).
    aspect = 2.0 * 1.7320508075688772935 * r / longest;
    // Angles from the law of cosines.
    auto angle_deg = [](double opp, double l1, double l2) {
        double cosv = (l1 * l1 + l2 * l2 - opp * opp) / (2.0 * l1 * l2);
        cosv = std::max(-1.0, std::min(1.0, cosv));
        return std::acos(cosv) * 180.0 / 3.14159265358979323846;
    };
    min_ang_deg = std::min(angle_deg(lbc, lab, lac),
                           std::min(angle_deg(lac, lab, lbc), angle_deg(lab, lac, lbc)));
    max_ang_deg = std::max(angle_deg(lbc, lab, lac),
                           std::max(angle_deg(lac, lab, lbc), angle_deg(lab, lac, lbc)));
}

void accumulate(ta_stats* st, double area, double vol_contrib, double aspect,
                double min_ang, double max_ang, bool degenerate) {
    st->n_triangles++;
    if (degenerate) {
        st->n_degenerate++;
        return;
    }
    st->total_area += area;
    st->signed_volume += vol_contrib;
    st->abs_volume_sum3 += std::fabs(vol_contrib);
    st->aspect_min = std::min(st->aspect_min, aspect);
    st->aspect_max = std::max(st->aspect_max, aspect);
    st->aspect_mean += aspect;  // divided by count at the end
    st->angle_min_deg = std::min(st->angle_min_deg, min_ang);
    st->angle_max_deg = std::max(st->angle_max_deg, max_ang);
}

void finalize(ta_stats* st) {
    const long counted = st->n_triangles - st->n_degenerate;
    if (counted > 0) st->aspect_mean /= static_cast<double>(counted);
    // Star-shapedness ratio |signed| / sum|contrib|: equals 1 iff every
    // per-face origin-tet contribution shares one sign. TRUE for meshes
    // that enclose and are star-shaped about the origin (per-face signs
    // then agree); NOT a general orientation-consistency measure — a
    // consistently wound mesh translated away from the origin legitimately
    // has mixed per-face signs. Orientation is validated by comparing
    // signed_volume against a known enclosed volume.
    st->star_shape_ratio = st->abs_volume_sum3 > 0.0
                               ? std::fabs(st->signed_volume) / st->abs_volume_sum3
                               : 0.0;
}

}  // namespace

ta_result audit_vertices(const double* vertices, long n_triangles,
                         ta_stats* stats) {
    if (stats == nullptr) return TA_ERR_NULL;
    if (vertices == nullptr || n_triangles <= 0) {
        std::fprintf(stderr, "triangle_mesh: null vertex buffer or nonpositive count\n");
        return TA_ERR_EMPTY;
    }
    std::memset(stats, 0, sizeof(*stats));
    stats->aspect_min = 1e300;
    stats->aspect_max = 0.0;
    stats->aspect_mean = 0.0;
    stats->angle_min_deg = 180.0;
    stats->angle_max_deg = 0.0;
    for (long i = 0; i < n_triangles; ++i) {
        const double* v = vertices + 9 * i;
        const Vec3 a{v[0], v[1], v[2]}, b{v[3], v[4], v[5]}, c{v[6], v[7], v[8]};
        if (!finite_vec(a) || !finite_vec(b) || !finite_vec(c)) {
            std::fprintf(stderr, "triangle_mesh: non-finite coordinate in triangle %ld\n", i);
            return TA_ERR_NONFINITE;
        }
        double area, vol, aspect, mn, mx;
        bool deg;
        stats_for_triangle(a, b, c, area, vol, aspect, mn, mx, deg);
        accumulate(stats, area, vol, aspect, mn, mx, deg);
    }
    finalize(stats);
    if (stats->n_triangles - stats->n_degenerate <= 0) {
        std::fprintf(stderr, "triangle_mesh: all triangles degenerate\n");
        return TA_ERR_EMPTY;
    }
    return TA_OK;
}

long count_triangles(const char* path) {
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return -TA_ERR_IO;
    char head[6] = {0};
    const size_t got = std::fread(head, 1, 5, f);
    std::fclose(f);
    if (got != 5) return -TA_ERR_FORMAT;
    const bool ascii = std::strncmp(head, "solid", 5) == 0;
    if (ascii) {
        // Count "facet normal" occurrences (robust to header words).
        f = std::fopen(path, "rb");
        if (!f) return -TA_ERR_IO;
        long n = 0;
        char line[256];
        while (std::fgets(line, sizeof(line), f)) {
            if (std::strncmp(line, "facet normal", 12) == 0) ++n;
        }
        std::fclose(f);
        return n > 0 ? n : -TA_ERR_FORMAT;
    }
    // Binary: 84-byte header + 50 bytes per triangle.
    f = std::fopen(path, "rb");
    if (!f) return -TA_ERR_IO;
    if (std::fseek(f, 0, SEEK_END) != 0) { std::fclose(f); return -TA_ERR_IO; }
    const long size = std::ftell(f);
    std::fclose(f);
    const long n = (size - 84) / 50;
    return (size >= 84 && (size - 84) % 50 == 0 && n > 0) ? n : -TA_ERR_FORMAT;
}

ta_result load_triangles(const char* path, double* vertices, long n_triangles) {
    if (vertices == nullptr) return TA_ERR_NULL;
    const long expected = count_triangles(path);
    if (expected < 0) return static_cast<ta_result>(-expected);
    if (expected != n_triangles) {
        std::fprintf(stderr, "triangle_mesh: buffer sized for %ld triangles, file has %ld\n",
                     n_triangles, expected);
        return TA_ERR_FORMAT;
    }
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return TA_ERR_IO;
    char head[6] = {0};
    std::fread(head, 1, 5, f);
    std::rewind(f);
    if (std::strncmp(head, "solid", 5) == 0) {
        char line[256];
        long idx = 0;
        int vertex_slot = -1;
        while (std::fgets(line, sizeof(line), f)) {
            if (std::strncmp(line, "vertex", 6) == 0) {
                double x, y, z;
                if (std::sscanf(line + 6, "%lf %lf %lf", &x, &y, &z) != 3) {
                    std::fclose(f);
                    std::fprintf(stderr, "triangle_mesh: bad vertex line\n");
                    return TA_ERR_FORMAT;
                }
                double* v = vertices + 9 * idx + 3 * vertex_slot;
                v[0] = x; v[1] = y; v[2] = z;
                if (++vertex_slot == 3) vertex_slot = -1, ++idx;
            } else if (std::strncmp(line, "facet normal", 12) == 0) {
                vertex_slot = 0;
            }
        }
        std::fclose(f);
        return idx == n_triangles ? TA_OK : TA_ERR_FORMAT;
    }
    // Binary STL.
    unsigned char header[84];
    if (std::fread(header, 1, 84, f) != 84) { std::fclose(f); return TA_ERR_FORMAT; }
    for (long i = 0; i < n_triangles; ++i) {
        unsigned char rec[50];
        if (std::fread(rec, 1, 50, f) != 50) { std::fclose(f); return TA_ERR_FORMAT; }
        // Skip the 12-byte stored normal: normals are recomputed from vertices.
        auto read_f32 = [&](int off) {
            float v;
            std::memcpy(&v, rec + off, 4);
            return static_cast<double>(v);
        };
        double* v = vertices + 9 * i;
        v[0] = read_f32(12); v[1] = read_f32(16); v[2] = read_f32(20);
        v[3] = read_f32(24); v[4] = read_f32(28); v[5] = read_f32(32);
        v[6] = read_f32(36); v[7] = read_f32(40); v[8] = read_f32(44);
    }
    std::fclose(f);
    return TA_OK;
}

ta_result audit_file(const char* path, ta_stats* stats) {
    if (stats == nullptr) return TA_ERR_NULL;
    const long n = count_triangles(path);
    if (n < 0) return static_cast<ta_result>(-n);
    std::vector<double> vertices(9 * static_cast<size_t>(n));
    const ta_result lr = load_triangles(path, vertices.data(), n);
    if (lr != TA_OK) return lr;
    return audit_vertices(vertices.data(), n, stats);
}

ta_result write_triangle_csv(const char* path, const double* vertices,
                             long n_triangles) {
    if (path == nullptr || vertices == nullptr) return TA_ERR_NULL;
    std::FILE* f = std::fopen(path, "w");
    if (!f) return TA_ERR_IO;
    std::fprintf(f, "index,area,aspect_ratio,min_angle_deg,max_angle_deg,degenerate\n");
    for (long i = 0; i < n_triangles; ++i) {
        const double* v = vertices + 9 * i;
        const Vec3 a{v[0], v[1], v[2]}, b{v[3], v[4], v[5]}, c{v[6], v[7], v[8]};
        double area, vol, aspect, mn, mx;
        bool deg;
        stats_for_triangle(a, b, c, area, vol, aspect, mn, mx, deg);
        if (deg) {
            std::fprintf(f, "%ld,%.9e,,,,1\n", i, area);
        } else {
            std::fprintf(f, "%ld,%.9e,%.9e,%.6f,%.6f,0\n", i, area, aspect, mn, mx);
        }
    }
    std::fclose(f);
    return TA_OK;
}

ta_result write_vertex_csv(const char* path, const double* vertices,
                           long n_triangles) {
    if (path == nullptr || vertices == nullptr) return TA_ERR_NULL;
    std::FILE* f = std::fopen(path, "w");
    if (!f) return TA_ERR_IO;
    std::fprintf(f, "triangle,vertex,x,y,z\n");
    for (long i = 0; i < n_triangles; ++i) {
        for (int k = 0; k < 3; ++k) {
            const double* v = vertices + 9 * i + 3 * k;
            std::fprintf(f, "%ld,%d,%.9e,%.9e,%.9e\n", i, k, v[0], v[1], v[2]);
        }
    }
    std::fclose(f);
    return TA_OK;
}

}  // namespace triangle_mesh
