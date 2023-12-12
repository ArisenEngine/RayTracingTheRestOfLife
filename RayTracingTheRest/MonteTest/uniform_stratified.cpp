

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
    int inside_circle = 0;
    int inside_circle_stratified = 0;
    int sqrt_N = 100;
    int size = 500;
    graph uniform_graph(size, size, "uniform");
    graph stratified_graph(size, size, "stratified");

    for (int i = 0; i < sqrt_N; i++) {
        for (int j = 0; j < sqrt_N; j++) {
            auto x = random_double(-1, 1);
            auto y = random_double(-1, 1);
            uniform_graph.draw_point(x * 0.5 + 0.5, y * 0.5 + 0.5);
            if (x * x + y * y < 1)
                inside_circle++;
            x = 2 * ((i + random_double()) / sqrt_N) - 1;
            y = 2 * ((j + random_double()) / sqrt_N) - 1;
            stratified_graph.draw_point(x * 0.5 + 0.5, y * 0.5 + 0.5);
            if (x * x + y * y < 1)
                inside_circle_stratified++;
        }
    }

    std::cout << std::fixed << std::setprecision(12);
    std::cout
        << "Regular    Estimate of Pi = "
        << (4.0 * inside_circle) / (sqrt_N * sqrt_N) << '\n'
        << "Stratified Estimate of Pi = "
        << (4.0 * inside_circle_stratified) / (sqrt_N * sqrt_N) << '\n';

    while (true)
    {
        uniform_graph.present();
        stratified_graph.present();
        cv::pollKey();
    }
}