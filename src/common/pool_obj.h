#ifndef _POOL_OBJ_H_
#define _POOL_OBJ_H_

#include <memory>
#include <string>
#include <assert.h>
#include "obj_pool.h"

template <class T>
class pool_obj {
public:
    void set_pool(std::shared_ptr<obj_pool<T>> pool) {
        pool_ = pool;
    }
	
    std::shared_ptr<obj_pool<T>> get_pool() {
        assert(pool_);
        return pool_;
    }
    virtual void clean() {};

private:
    std::shared_ptr<obj_pool<T>> 	pool_;
    std::string 					obj_name_;
};

#endif