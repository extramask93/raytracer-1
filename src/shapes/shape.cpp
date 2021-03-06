/**
 * shape.cpp
 *
 * By Sebastian Raaphorst, 2018.
 */

#include <memory>
#include <tuple>
#include <vector>

#include "affine_transform.h"
#include "bounding_box.h"
#include "shape.h"
#include "intersection.h"
#include "material.h"
#include "matrix.h"
#include "ray.h"

using namespace raytracer;
using namespace raytracer::impl;

namespace raytracer::shapes {
    Shape::Shape(dummy d) noexcept:
        InstanceManager{d},
        transformation{predefined_matrices::I<double, 4>},
        transformationInverse{predefined_matrices::I<double, 4>},
        transformationInverseTranspose{predefined_matrices::I<double, 4>},
        material{std::make_shared<Material>()},
        parent(nullptr),
        casts_shadow{true} {}

    bool Shape::operator==(const Shape &other) const noexcept {
        return typeid(*this) == typeid(other)
               && transformation == other.transformation
               && casts_shadow == other.casts_shadow
               && *material == *other.material
               && doCompare(other);
    }

    bool Shape::operator!=(const Shape &other) const noexcept {
        return !(*this == other);
    }

    const Transformation &Shape::getTransformation() const noexcept {
        return transformation;
    }

    const Transformation &Shape::getTransformationInverse() const noexcept {
        return transformationInverse;
    }

    void Shape::setTransformation(Transformation&& t) noexcept {
        transformation = t;
        transformationInverse = transformation.invert();
        transformationInverseTranspose = transformationInverse.transpose();
    }

    void Shape::setTransformation(const Transformation &t) noexcept {
        transformation = t;
        transformationInverse = transformation.invert();
        transformationInverseTranspose = transformationInverse.transpose();
    }

    void Shape::setTransformation(Transformation &t) noexcept {
        transformation = t;
        transformationInverse = transformation.invert();
        transformationInverseTranspose = transformationInverse.transpose();
    }

    const std::shared_ptr<Material> &Shape::getMaterial() const noexcept {
        return material;
    }

    std::shared_ptr<Material> &Shape::getMaterial() noexcept {
        return material;
    }

    void Shape::setMaterial(std::shared_ptr<Material> &&m) noexcept {
        material = std::move(m);
    }

    void Shape::setMaterial(const std::shared_ptr<Material> &m) noexcept {
        material = m;
    }

    void Shape::setMaterial(std::shared_ptr<Material> &m) noexcept {
        material = m;
    }

    const std::shared_ptr<const Shape> Shape::getParent() const noexcept {
        return parent;
    }

    void Shape::setParent(std::shared_ptr<const Shape> p) noexcept {
        parent = p;
    }

    bool Shape::castsShadow() const noexcept {
        return casts_shadow;
    }

    void Shape::setCastsShadow(bool s) noexcept {
        casts_shadow = s;
    }

    const std::vector<Intersection> Shape::intersect(const Ray &r0) const noexcept {
        // Transform the ray to object space.
        const Ray r = r0.transform(transformationInverse);

        // Return the shape-dependent implementation results.
        return localIntersection(r);
    }

    const Tuple Shape::normalAt(const Tuple &world_point) const noexcept {
        const auto local_point = worldToObject(world_point);
        const auto local_normal = localNormalAt(local_point);
        return normalToWorld(local_normal);

    }

    const Tuple Shape::worldToObject(const Tuple &point) const noexcept {
        return transformationInverse * (parent == nullptr ? point : parent->worldToObject(point));
    }

    const Tuple Shape::normalToWorld(const Tuple &normal) const noexcept {
        const auto n1 = transformationInverseTranspose * normal;
        const auto n2 = make_vector(n1[tuple_constants::x],
                                    n1[tuple_constants::y],
                                    n1[tuple_constants::z]).normalize();
        return (parent == nullptr) ? n2 : parent->normalToWorld(n2);
    }

    BoundingBox Shape::parentSpaceBounds() const {
        return bounds().transform(transformation);
    }
}