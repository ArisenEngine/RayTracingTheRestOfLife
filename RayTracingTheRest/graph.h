#pragma once
#include "ray.h"
#include "vec3.h"
#include <math.h>
#include "rt_weekend.h"

class graph
{
public:
	graph() = delete;
	graph(int width, int height, const std::string&& name, cv::Scalar background = cv::Scalar(255, 255, 255)):
		_width(width), _height(height), _name(name)
	{
		
		_buffer = new cv::Mat(std::vector<int>{height, width}, 
			CV_8UC3, background);

		cv::namedWindow(_name, cv::WINDOW_AUTOSIZE);
	}

	void draw_point(double x, double y, 
		cv::Scalar color = cv::Scalar(0, 0, 255))
	{
	
		int row = y * _height;
		int col = x * _width;

		_buffer->at<cv::Vec3b>(row, col)[0] = (uchar)color[0];
		_buffer->at<cv::Vec3b>(row, col)[1] = (uchar)color[1];
		_buffer->at<cv::Vec3b>(row, col)[2] = (uchar)color[2];
	}

	void draw_point_and_present(double x, double y,
		cv::Scalar color = cv::Scalar(0, 0, 255))
	{
		draw_point(x, y, color);
		present();
	}

	void present()
	{
		cv::imshow(_name, *_buffer);
	}

	~graph()
	{
		cv::destroyWindow(_name);
		delete _buffer;
	}
private:
	int _width, _height;
	cv::Mat* _buffer;
	std::string _name;
};