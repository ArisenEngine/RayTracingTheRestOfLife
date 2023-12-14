

#include "../rt_weekend.h"
#include "../graph.h"
#include <iostream>
#include <iomanip>

#if _DEBUG
#pragma comment(lib,"opencv_world481d.lib")
#else
#pragma comment(lib,"opencv_world481.lib")
#endif

int main() {
   
    const int size = 500;
    const int sample_count = 1000;
    graph uniform_graph(size, size, "uniform");
    graph stratified_graph(size, size, "stratified");

    for (int i = 0; i < sample_count; ++i)
    {
        uniform_graph.draw_point(random_double(0, 1), random_double(0, 1));
    }
   

    while (true)
    {
        uniform_graph.present();
        stratified_graph.present();
        cv::pollKey();
    }
}