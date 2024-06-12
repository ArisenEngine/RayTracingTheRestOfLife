#pragma once

#include "rt_weekend.h"
#include "hittable_list.h"
#include "texture.h"
#include "onb.h"

class hit_record;

class material {
public:
    virtual ~material() = default;

    virtual double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
        const {
        return 0;
    }

    virtual color emitted(
        const ray& r_in, const hit_record& rec, double u, double v, const point3& p
    ) const {
        return color(0, 0, 0);
    }

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, double& pdf , bool importance_sampling = false) const = 0;
};


class lambertian : public material {
public:
    lambertian(const color& a) : albedo(make_shared<solid_color>(a)) {}
    lambertian(std::shared_ptr<texture> a) : albedo(a) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, double& pdf, bool importance_sampling = false)
        const override {
        onb uvw;
        uvw.build_from_w(rec.normal);
        auto scatter_direction = uvw.local(random_cosine_direction());
        scattered = ray(rec.p, normalize(scatter_direction), r_in.time());
        attenuation = albedo->value(rec.u, rec.v, rec.p);
        pdf = dot(uvw.w(), scattered.direction()) / pi;
        return true;
    }

    double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const {
        return 1 / (2 * pi);
    }

private:
    std::shared_ptr<texture> albedo;
};


class metal : public material {
public:
    metal(const color& a, double f = 0.0) : albedo(make_shared<solid_color>(a)), fuzz(f < 1 ? f : 1) {}
    metal(std::shared_ptr<texture> a, double f = 0.0) : albedo(a), fuzz(f < 1 ? f : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, double& pdf, bool importance_sampling = false)
        const override {
        vec3 reflected = reflect(r_in.direction(), rec.normal);

        scattered = ray(rec.p,
            importance_sampling ?
            ggx_weighted_random_vector(rec.normal, fuzz) :
            reflected + fuzz * random_in_unit_sphere(),
            r_in.time());
        attenuation = albedo->value(rec.u, rec.v, rec.p);;
        return (dot(scattered.direction(), rec.normal) > 0);
    }

private:
    std::shared_ptr<texture> albedo;
    double fuzz;
};

class dielectric : public material {
public:
    dielectric(double index_of_refraction) : ir(index_of_refraction) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, double& pdf, bool importance_sampling = false)
        const override {
        attenuation = color(1.0, 1.0, 1.0);
        double refraction_ratio = rec.front_face ? (1.0 / ir) : ir;

        vec3 unit_direction = normalize(r_in.direction());
        double cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, refraction_ratio);

        scattered = ray(rec.p, direction, r_in.time());
        return true;
    }

private:
    double ir; // Index of Refraction

    static double reflectance(double cosine, double ref_idx) {
        // Use Schlick's approximation for reflectance.
        auto r0 = (1 - ref_idx) / (1 + ref_idx);
        r0 = r0 * r0;
        return r0 + (1 - r0) * pow((1 - cosine), 5);
    }
};

//class one_bounce_specular : public material {
//public:
//    one_bounce_specular(const color& a): albedo(make_shared<solid_color>(a)) {}
//    one_bounce_specular(std::shared_ptr<texture> a, double f = 0.0) : albedo(a){}
//
//    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
//        const override {
//        vec3 reflected = reflect(normalize(r_in.direction()), rec.normal);
//        scattered = ray(rec.p, reflected, r_in.time());
//        attenuation = albedo->value(rec.u, rec.v, rec.p);
//        return false;
//    }
//
//private:
//    std::shared_ptr<texture> albedo;
//   
//};

class diffuse_light : public material {
public:
    diffuse_light(shared_ptr<texture> a) : emit(a) {}
    diffuse_light(color c) : emit(make_shared<solid_color>(c)) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, double& pdf, bool importance_sampling = false)
        const override {
        return false;
    }

    color emitted(const ray& r_in, const hit_record& rec, double u, double v, const point3& p)
        const override {
        if (!rec.front_face)
            return color(0, 0, 0);
        return emit->value(u, v, p);
    }

private:
    shared_ptr<texture> emit;
};

class isotropic : public material {
public:
    isotropic(color c) : albedo(make_shared<solid_color>(c)) {}
    isotropic(shared_ptr<texture> a) : albedo(a) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered, double& pdf, bool importance_sampling = false)
        const override {
        scattered = ray(rec.p, random_unit_vector(), r_in.time());
        attenuation = albedo->value(rec.u, rec.v, rec.p);
        pdf = 1 / (4 * pi);
        return true;
    }

    double scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered)
        const override {
        return 1 / (4 * pi);
    }
private:
    shared_ptr<texture> albedo;
};