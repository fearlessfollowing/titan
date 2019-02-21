#include <iostream>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <chrono>
#include <algorithm>
#include <vector>

template<class T>
class test
{
public:
    test(T a)
    {
        std::cout << a << std::endl;
    }
private:
};

template<class T1, class T2>
auto print(const T1& a, const T2& b) -> decltype(a*b)
{
    return a*b;
    //std::cout << a << std::endl;
};

//template void print<int>(const int&);

void test1()
{
}

void test2()
{
}

class point
{
public:
    point(double x, double y) : x_(x), y_(y) {};
    double x_ = 0;
    double y_ = 0;
};

constexpr double middle(const point& p1, const point& p2)
{
    return p1.x_+p2.x_;
    //return {p1.x_+p2.x_, p1.y_+p2.y_};
}

constexpr int add(int a, int b)
{
    return a + b;
}

int main(int argc, char* argv[])
{
    point p1(1, 2);
    point p2(1, 2);
    middle(p1, p2);
    // std::chrono::duration<double> c1(250);
    // std::chrono::duration<double, std::ratio<1,1000>> c3(100);
    // auto c2 = std::chrono::duration_cast<std::chrono::minutes>(c1);

    // auto start = std::chrono::high_resolution_clock::now();

    // auto end = std::chrono::high_resolution_clock::now();

    // std::chrono::duration<double> diff = end-start;

    // std::cout << diff.count() << std::endl;

    // int a[] = {1, 2, 3, 4, 5};
    // std::for_each(std::begin(a), std::end(a), [](int p){ std::cout << p << " "; });
    // std::cout << std::endl;

    // std::vector<int> v = {8, 5, 2, 9, 1};
    // std::for_each(std::begin(v), std::end(v), [](int p){ std::cout << p << " "; });
    // std::cout << std::endl;
    // std::sort(v.begin(), v.end());
    // std::for_each(std::begin(v), std::end(v), [](int p){ std::cout << p << " "; });
    // std::cout << std::endl;
    // auto sum = std::accumulate(std::next(v.begin()), v.end(), 0);
    // std::cout << sum << std::endl;

    // double d = 5.5;
    // decltype(d) d1;
    // std::cout << sizeof(d) << std::endl;
    // std::cout << sizeof(d1) << std::endl;

    return 0;
}
