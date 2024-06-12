#pragma once
#include "ray.h"
#include "vec3.h"
#include <math.h>
#include "rt_weekend.h"

struct view_port
{
	float x, y, width, height;
};

struct float2
{
	float u, v;
};

struct varying
{
	float2 uv;
	int bounce;
};


class camera {

public:
	std::string name{ "default" };
	cv::Mat* buffer{ nullptr };

	camera(std::string name, vec3 position, float near, float far, float fov, int width, int height, int bounce, int samples, bool importance) :
		_position(position),
		_fov(fov),
		_near(near),
		_far(far),
		_aspect_ratio((float)width / height),
		name(name),
		bounce(bounce),
		sample_count(samples),
		importance_sampling(importance)
	{
		cv::namedWindow(name, cv::WINDOW_AUTOSIZE);
		buffer = new cv::Mat(std::vector<int>{height, width}, CV_8UC3, cv::Scalar(0, 0, 0));
	}

	~camera()
	{
		cv::destroyWindow(name);
		delete buffer;
	}

	void present()
	{
		cv::imshow(name, *buffer);
	}

	float aspect_ratio() const { return _aspect_ratio; }
	float fov() const { return _fov; }
	void set_fov(float fov) { _fov = fov; }
	float fov_rad() const { return degrees_to_radians(_fov); }
	vec3 position() const { return _position; }
	void set_position(const vec3& pos) { _position = pos; }
	float near() const { return _near; }
	double focus_distance() const { return focus_dist; }


	vec3 forward() const { return _forward; }
	vec3 right() const { return _right; }
	vec3 up() const { return _up; }

	void look_at(vec3 target, vec3 world_up = vec3(0, 1, 0))
	{
		// always look to -z direction in view space;
		_forward = normalize(target - _position);

		// use right-hand coordinate
		_right = normalize(cross(world_up, -_forward));

		_up = normalize(cross(-_forward, _right));

		//focus_dist = (target - _position).length();
	}

	void set_defocus_angle(double angle)
	{
		// TODO: fix this bug
		defocus_angle = angle * 0.06;
	}

	void set_focus_dist(double dist)
	{
		focus_dist = dist;
	}

	double get_defocus_angle() const
	{
		return defocus_angle;
	}

	point3 defocus_disk_sample() const {


		// Calculate the camera defocus disk basis vectors.
		auto defocus_radius = focus_dist * tan(degrees_to_radians(defocus_angle / 2));
		auto defocus_disk_u = _right * defocus_radius;
		auto defocus_disk_v = _up * defocus_radius;

		// Returns a random point in the camera defocus disk.
		auto p = random_in_unit_disk();
		return _position + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
	}


	// ray tracing
	int sample_count{ 10 };
	int bounce{ 10 };
	color background;               // Scene background color
	bool importance_sampling{ false };

	void render(const hittable_list& world, int&& tile_size)
	{

		if (_rendering_thread.joinable())
		{
			_rendering_thread.join();
		}

		_is_running = true;
		_world = world;


		void (camera:: * tracer)(cv::Mat * buffer,
			const int rows_start, const int rows_end,
			const int cols_start, const int cols_end,
			const int sample_per_pixel, const int bounce);

		tracer = &camera::ray_tracing;

		_rendering_thread = std::thread([this, tracer, &tile_size]() {

			const int horizontal_count = std::ceil((double)buffer->cols / tile_size);
		const int vertical_count = std::ceil((double)buffer->rows / tile_size);

		auto start_clock = clock();

		for (int i = 0; i < vertical_count; ++i)
		{
			for (int j = 0; j < horizontal_count; ++j)
			{
				std::thread tracing_thread(tracer,
					this,
					buffer,										 // render target
					i * tile_size,							     // row start
					std::min(buffer->rows, (i + 1) * tile_size), // row end
					j * tile_size,                               // col start
					std::min(buffer->cols, (j + 1) * tile_size), // col end
					sample_count, // sample count
					bounce  // bounce count
				);

				_tiled_threads.emplace_back(std::move(tracing_thread));
			}
		}

		join_tiled_threads();

		cv::putText(*buffer, "frame time:" + std::to_string((clock() - start_clock) / 1000) + " s ", cv::Point(4, 25), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1.5);
			});


	}

	void dispose()
	{
		_is_running = false;

		join_tiled_threads();

		if (_rendering_thread.joinable())
		{
			_rendering_thread.join();
		}

	}

private:

	std::thread _rendering_thread;
	std::vector<std::thread> _tiled_threads;
	std::atomic<bool> _is_running{ false };
	hittable_list _world;

	void join_tiled_threads()
	{
		for (auto& tile_unit : _tiled_threads)
		{
			if (tile_unit.joinable())
			{
				tile_unit.join();
			}
		}

		_tiled_threads.clear();
	}

	vec3 _position;
	float _near, _far, _fov, _aspect_ratio;

	vec3 _forward = vec3(0, 0, -1);
	vec3 _right = vec3(1, 0, 0);
	vec3 _up = vec3(0, 1, 0);

	double defocus_angle = 0;  // Variation angle of rays through each pixel
	double focus_dist = 10;    // Distance from camera lookfrom point to plane of perfect focus

	color sky_color(const ray& r)
	{
		vec3 unit_direction = normalize(r.direction());
		auto a = 0.5 * (unit_direction.y() + 1.0);
		return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
	}

	static float linear_to_gamma(float value)
	{
		return pow(value, 1 / 2.2);
	}

	color ray_cast(const ray& r, const hittable& world, const int bounce) {

		if (bounce <= 0 || !_is_running)
		{
			return color(0, 0, 0);
		}

		hit_record rec;

		// If the ray hits nothing, return the background color.
		if (!world.hit(r, interval(0.001, infinity), rec))
			return background;

		ray scattered;
		color attenuation;
		color color_from_emission = rec.mat->emitted(scattered, rec, rec.u, rec.v, rec.p);
		double pdf = 1.0;
		if (!rec.mat->scatter(r, rec, attenuation, scattered, pdf, importance_sampling))
			return color_from_emission;

		auto on_light = point3(random_double(213, 343), 554, random_double(227, 332));
		auto to_light = on_light - rec.p;
		auto distance_squared = to_light.length_squared();
		to_light = normalize(to_light);

		if (dot(to_light, rec.normal) < 0)
			return color_from_emission;

		double light_area = (343 - 213) * (332 - 227);
		auto light_cosine = fabs(to_light.y());
		if (light_cosine < 0.000001)
			return color_from_emission;

		pdf = distance_squared / (light_cosine * light_area);
		scattered = ray(rec.p, to_light, r.time());

		double scattering_pdf = rec.mat->scattering_pdf(r, rec, scattered);
	
		color color_from_scatter =
			(attenuation * scattering_pdf * ray_cast(scattered, world, bounce - 1)) / pdf;
	

		return color_from_emission + color_from_scatter;
	}


	color pixel_shader(varying varying) {

		auto r = get_ray(varying.uv);

		return ray_cast(r, _world, varying.bounce);
	}

	ray get_ray(float2 uv)
	{
		auto view_height = tan(fov_rad() / 2) * near() * 2;
		auto view_width = view_height * aspect_ratio();

		auto camera_up = up();
		auto camera_right = right();
		auto camera_forward = -forward();

		assert(approx_equal(0, dot(camera_up, camera_right)));
		assert(approx_equal(0, dot(camera_up, camera_forward)));
		assert(approx_equal(0, dot(camera_right, camera_forward)));


		auto pixel_view_pos = vec3
		{
			(uv.u - 0.5) * view_width ,
			(uv.v - 0.5) * view_height ,
			// camera is look to -z
			-near()
		};

		vec3 ray_direction_view{ normalize(pixel_view_pos) };
		vec3 ray_direction(
			dot(vec3(camera_right.x(), camera_up.x(), camera_forward.x()),
				ray_direction_view),
			dot(vec3(camera_right.y(), camera_up.y(), camera_forward.y()),
				ray_direction_view),
			dot(vec3(camera_right.z(), camera_up.z(), camera_forward.z()),
				ray_direction_view)
		);

		double defocus_angle = get_defocus_angle();

		vec3 ray_origin = defocus_angle <= 0 ? position() : defocus_disk_sample();
		auto ray_time = random_double();

		return ray(ray_origin, ray_direction, ray_time);

	}

	const interval color_intensity = interval(0.000, 0.999);


	void ray_tracing(
		cv::Mat* buffer,
		const int rows_start, const int rows_end,
		const int cols_start, const int cols_end,
		const int sample_per_pixel, const int bounce)
	{
		auto target_width = buffer->cols;
		auto target_height = buffer->rows;

		for (int j = rows_start; j < rows_end; ++j) {
			for (int i = cols_start; i < cols_end; ++i) {

				color final_color;
				for (int sample = 0; sample < sample_per_pixel; ++sample)
				{
					if (!_is_running)
					{
						break;
					}

					auto color = pixel_shader(varying
						{
							// uv
							float2{
							(float)((float)i / target_width + random_double() / (target_width)),
							1.0f - (float)((float)j / target_height + random_double() / target_height)},
							// bounce
							bounce
						}
					);

					final_color += color;
				}

				final_color /= sample_per_pixel;

				//BGR£¨À¶¡¢ÂÌ¡¢ºì£©
				buffer->at<cv::Vec3b>(j, i)[0] = (uchar)(linear_to_gamma(color_intensity.clamp(final_color.z())) * 255);
				buffer->at<cv::Vec3b>(j, i)[1] = (uchar)(linear_to_gamma(color_intensity.clamp(final_color.y())) * 255);
				buffer->at<cv::Vec3b>(j, i)[2] = (uchar)(linear_to_gamma(color_intensity.clamp(final_color.x())) * 255);

			}
		}

	}
};