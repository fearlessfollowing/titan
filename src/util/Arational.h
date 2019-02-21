#ifndef _A_RATIONAL_H_
#define _A_RATIONAL_H_

struct Arational
{
    Arational() = default;
    Arational(int32_t n, int32_t d) :num(n), den(d) {};
    double to_double() const
    {
        return (double)num/(double)den;
    }

    int32_t num = 0;
    int32_t den = 1;
};

#endif