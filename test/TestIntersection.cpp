/**
 * TestIntersection.cpp
 *
 * By Sebastian Raaphorst, 2018.
 */

#include <catch.hpp>

#include <array>
#include <memory>
#include <vector>

#include "constmath.h"
#include "hit.h"
#include "intersection.h"
#include "plane.h"
#include "ray.h"
#include "sphere.h"
#include "transformers.h"
#include "tuple"

using namespace raytracer;
using namespace raytracer::impl;
using namespace raytracer::shapes;
using namespace raytracer::transformers;

TEST_CASE("Intersection: Intersection can be created and initialized") {
    Intersection it{0.5, Sphere::createSphere()};
    REQUIRE(*it.getObject() == *Sphere::createSphere());
    REQUIRE(it.getT() == 0.5);
}

TEST_CASE("Intersection: Intersect sets the object on the intersection") {
    const Ray r{make_point(0, 0, -5), make_vector(0, 0, 1)};
    const auto s = Sphere::createSphere();
    const auto xs = s->intersect(r);
    REQUIRE(xs.size() == 2);
    REQUIRE(xs[0].getObject() == s);
    REQUIRE(xs[1].getObject() == s);
}

TEST_CASE("Intersection: The hit, when all intersections have positive t") {
    const Intersection i1{1, Sphere::createSphere()};
    const Intersection i2{2, Sphere::createSphere()};
    const std::vector<Intersection> xs{i1, i2};
    const auto hit = Intersection::hit(xs);
    REQUIRE(hit.has_value());
    REQUIRE(hit.value() == i1);
}

TEST_CASE("Intersection: The hit, when some intersections have negative t") {
    const Intersection i1{-1, Sphere::createSphere()};
    const Intersection i2{1, Sphere::createSphere()};
    const std::vector<Intersection> xs{i1, i2};
    const auto hit = Intersection::hit(xs);
    REQUIRE(hit.has_value());
    REQUIRE(hit.value() == i2);
}

TEST_CASE("Intersection: The hit, when all intersections have negative t") {
    const Intersection i1{-1, Sphere::createSphere()};
    const Intersection i2{-2, Sphere::createSphere()};
    const std::vector<Intersection> xs{i1, i2};
    const auto hit = Intersection::hit(xs);
    REQUIRE_FALSE(hit.has_value());
}

TEST_CASE("Intersection: The hit is always the lowest non-negative intersection") {
    const Intersection i1{5, Sphere::createSphere()};
    const Intersection i2{7, Sphere::createSphere()};
    const Intersection i3{-3, Sphere::createSphere()};
    const Intersection i4{2, Sphere::createSphere()};
    const std::vector<Intersection> xs{i1, i2, i3, i4};
    const auto hit = Intersection::hit(xs);
    REQUIRE(hit.has_value());
    REQUIRE(hit.value() == i4);
}

TEST_CASE("Intersection: Precomputing the state of an intersection") {
    const Ray ray{make_point(0, 0, -5), make_vector(0, 0, 1)};
    const Intersection i{4, Sphere::createSphere()};
    const auto hit = Intersection::prepareHit(i, ray, {});
    REQUIRE(hit.getPoint() == make_point(0, 0, -1));
    REQUIRE(hit.getEyeVector() == make_vector(0, 0, -1));
    REQUIRE(hit.getNormalVector() == make_vector(0, 0, -1));
}

TEST_CASE("Intersection: An intersection occurs on the outside") {
    const Ray ray{make_point(0, 0, -5), make_vector(0, 0, 1)};
    const Intersection i{4, Sphere::createSphere()};
    const auto hit = Intersection::prepareHit(i, ray, {});
    REQUIRE_FALSE(hit.isInside());
}

TEST_CASE("Intersection: An intersection occurs on the inside") {
    const Ray ray{make_point(0, 0, 0), make_vector(0, 0, 1)};
    const Intersection i{1, Sphere::createSphere()};
    const auto hit = Intersection::prepareHit(i, ray, {});
    REQUIRE(hit.isInside());
    REQUIRE(hit.getPoint() == make_point(0, 0, 1));
    REQUIRE(hit.getEyeVector() == make_vector(0, 0, -1));
    REQUIRE(hit.getNormalVector() == make_vector(0, 0, -1)); // inverted
}

TEST_CASE("Intersection: The point is offset") {
    const Ray ray{make_point(0, 0, -5), predefined_tuples::z1};
    const Intersection i{4, Sphere::createSphere()};
    const auto hit = Intersection::prepareHit(i, ray, {});
    const auto z = hit.getPoint()[tuple_constants::z];
    REQUIRE(-1.1 < z);
    REQUIRE(z < -1);
}

TEST_CASE("Intersection: Precomputing the reflection vector") {
    const std::shared_ptr<Shape> shape = Plane::createPlane();
    constexpr double sqrt2 = const_sqrtd(2);
    constexpr double sqrt2by2 = sqrt2/2;
    const Ray ray{make_point(0, 1, -1), make_vector(0, -sqrt2by2, sqrt2by2)};
    const Intersection hit{sqrt2, shape};
    const auto prepared_hit = Intersection::prepareHit(hit, ray, {});
    REQUIRE(prepared_hit.getReflectVector() == make_vector(0, sqrt2by2, sqrt2by2));
}

TEST_CASE("Intersection: n1 and n2 at various intersections") {
    std::shared_ptr<Shape> sphere1 = Sphere::createGlassSphere();
    sphere1->setTransformation(scale(2, 2, 2));
    sphere1->getMaterial()->setRefractiveIndex(1.5);

    std::shared_ptr<Shape> sphere2 = Sphere::createGlassSphere();
    sphere2->setTransformation(translation(0, 0, -0.25));
    sphere2->getMaterial()->setRefractiveIndex(2);

    std::shared_ptr<Shape> sphere3 = Sphere::createGlassSphere();
    sphere3->setTransformation(translation(0, 0, 0.25));
    sphere3->getMaterial()->setRefractiveIndex(2.5);

    const Ray ray{make_point(0, 0, -4), predefined_tuples::z1};
    std::vector<Intersection> xs{Intersection{2, sphere1},
                                 Intersection{2.75, sphere2},
                                 Intersection{3.25, sphere3},
                                 Intersection{4.75, sphere2},
                                 Intersection{5.25, sphere3},
                                 Intersection{6,    sphere1}};

    constexpr std::array<double, 7> n1s{1.0, 1.5, 2.0, 2.5, 2.5, 1.5};
    constexpr std::array<double, 7> n2s{1.5, 2.0, 2.5, 2.5, 1.5, 1.0};

    for (auto idx = 0; idx < 6; ++idx) {
        const auto hit = Intersection::prepareHit(xs[idx], ray, xs);
        REQUIRE(hit.getN1() == n1s[idx]);
        REQUIRE(hit.getN2() == n2s[idx]);
    }
}

TEST_CASE("Intersection: The under point is offset below the surface") {
    const std::shared_ptr<Shape> sphere = Sphere::createGlassSphere();
    const Ray ray{make_point(0, 0, -5), predefined_tuples::z1};
    const std::vector<Intersection> xs{Intersection{4, sphere}};
    const auto hit = Intersection::prepareHit(xs[0], ray, xs);
    const auto z = hit.getUnderPoint()[tuple_constants::z];
    REQUIRE(z > -1);
    REQUIRE(z < -0.9);
}

TEST_CASE("Intersection: Schlick approximation under total internal reflection") {
    const auto sphere = Sphere::createGlassSphere();
    constexpr auto sqrt2by2 = const_sqrtd(2)/2;
    const Ray ray{make_point(0, 0, sqrt2by2), predefined_tuples::y1};
    const std::vector<Intersection> xs{Intersection{-sqrt2by2, sphere},
                                       Intersection{ sqrt2by2, sphere}};
    const auto hit = Intersection::prepareHit(xs[1], ray, xs);
    const auto reflectance = hit.schlick();
    REQUIRE(reflectance == 1.0);
}

TEST_CASE("Intersection: Schlick approximation with a perpendicular viewing angle") {
    const auto sphere = Sphere::createGlassSphere();
    const Ray ray{predefined_tuples::zero_point, predefined_tuples::y1};
    const std::vector<Intersection> xs{Intersection{-1, sphere},
                                       Intersection{ 1, sphere}};
    const auto hit = Intersection::prepareHit(xs[1], ray, xs);
    const auto reflectance = hit.schlick();
    REQUIRE(ALMOST_EQUALS(reflectance, 0.04));
}

TEST_CASE("Schlick approximation with small angle and n2 > n1") {
    const auto sphere = Sphere::createGlassSphere();
    const Ray ray{make_point(0, 0.99, -2), predefined_tuples::z1};
    const std::vector<Intersection> xs{Intersection{1.8589, sphere}};
    const auto hit = Intersection::prepareHit(xs[0], ray, xs);
    const auto reflectance = hit.schlick();
    REQUIRE(ALMOST_EQUALS(reflectance, 0.48873));
}