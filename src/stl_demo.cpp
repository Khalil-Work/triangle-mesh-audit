// Demo: generate a deterministic fan-triangulated unit-disk mesh (known
// area pi) plus a coarsened sphere approximation (known volume 4pi/3 in
// the limit), audit both, and write CSVs for plotting.
#define _CRT_SECURE_NO_WARNINGS 1
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "stl_audit.hpp"

namespace {
constexpr double kPi = 3.14159265358979323846;
}  // namespace

int main(int argc, char** argv) {
    const std::string out = argc > 1 ? argv[1] : "build";

    // Unit disk fan: center + ring of n sectors (2 triangles each would be
    // a strip; the fan uses 1 triangle per sector from the center).
    const int n_disk = 360;
    std::vector<double> disk(9.0 * n_disk);
    for (int i = 0; i < n_disk; ++i) {
        const double a0 = 2.0 * kPi * i / n_disk;
        const double a1 = 2.0 * kPi * (i + 1) / n_disk;
        double* v = disk.data() + 9 * i;
        v[0] = 0.0; v[1] = 0.0; v[2] = 0.0;
        v[3] = std::cos(a0); v[4] = std::sin(a0); v[5] = 0.0;
        v[6] = std::cos(a1); v[7] = std::sin(a1); v[8] = 0.0;
    }
    triangle_mesh::ta_stats disk_stats;
    if (triangle_mesh::audit_vertices(disk.data(), n_disk, &disk_stats) != triangle_mesh::TA_OK) {
        std::fprintf(stderr, "disk audit failed\n");
        return 1;
    }
    std::printf("disk: n=%ld area=%.9f (pi=%.9f, rel.err=%.2e) aspect_mean=%.4f\n",
                disk_stats.n_triangles, disk_stats.total_area, kPi,
                std::fabs(disk_stats.total_area - kPi) / kPi,
                disk_stats.aspect_mean);

    // Icosphere-ish: latitude-longitude sphere triangulation (quad split
    // into 2 triangles), radius 1, known volume 4pi/3 approached as the
    // mesh refines; fan caps at the poles.
    const int nlat = 48, nlon = 96;
    auto point = [&](int i, int j) {
        const double th = kPi * i / nlat;
        const double ph = 2.0 * kPi * j / nlon;
        return std::array<double, 3>{std::sin(th) * std::cos(ph),
                                     std::sin(th) * std::sin(ph), std::cos(th)};
    };
    std::vector<double> sphere;
    for (int i = 0; i < nlat; ++i) {
        for (int j = 0; j < nlon; ++j) {
            const auto a = point(i, j), b = point(i + 1, j),
                       c = point(i + 1, j + 1), d = point(i, j + 1);
            if (i == 0) {
                // North cap fan: single triangle a-b-c degenerates a==d.
                sphere.insert(sphere.end(), {b[0], b[1], b[2], c[0], c[1], c[2],
                                             d[0], d[1], d[2]});
            } else if (i == nlat - 1) {
                sphere.insert(sphere.end(), {a[0], a[1], a[2], b[0], b[1], b[2],
                                             d[0], d[1], d[2]});
            } else {
                sphere.insert(sphere.end(), {a[0], a[1], a[2], b[0], b[1], b[2],
                                             c[0], c[1], c[2]});
                sphere.insert(sphere.end(), {a[0], a[1], a[2], c[0], c[1], c[2],
                                             d[0], d[1], d[2]});
            }
        }
    }
    const long n_sphere = static_cast<long>(sphere.size() / 9);
    triangle_mesh::ta_stats sph_stats;
    if (triangle_mesh::audit_vertices(sphere.data(), n_sphere, &sph_stats) != triangle_mesh::TA_OK) {
        std::fprintf(stderr, "sphere audit failed\n");
        return 1;
    }
    const double v_exact = 4.0 * kPi / 3.0;
    std::printf("sphere: n=%ld area=%.6f (4pi=%.6f) vol=%.6f (4pi/3=%.6f, "
                "rel.err=%.2e) star=%.6f aspect_max=%.3f\n",
                sph_stats.n_triangles, sph_stats.total_area, 4.0 * kPi,
                sph_stats.signed_volume, v_exact,
                std::fabs(sph_stats.signed_volume - v_exact) / v_exact,
                sph_stats.star_shape_ratio, sph_stats.aspect_max);

    // CSVs for plotting.
    triangle_mesh::write_triangle_csv((out + "/disk_triangles.csv").c_str(),
                                      disk.data(), n_disk);
    triangle_mesh::write_vertex_csv((out + "/disk_vertices.csv").c_str(),
                                    disk.data(), n_disk);
    triangle_mesh::write_triangle_csv((out + "/sphere_triangles.csv").c_str(),
                                      sphere.data(), n_sphere);
    triangle_mesh::write_vertex_csv((out + "/sphere_vertices.csv").c_str(),
                                    sphere.data(), n_sphere);
    return 0;
}
