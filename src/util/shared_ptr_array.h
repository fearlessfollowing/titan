#ifndef _SHARED_PTR_ARRAY_H_
#define _SHARED_PTR_ARRAY_H_

#include <memory>

template <class T>
std::shared_ptr<T> make_shared_array(uint32_t size)
{
    return std::shared_ptr<T>(new T[size], [](T* p){ if (p) delete[] p;});
};

#endif