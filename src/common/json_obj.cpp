
#include "json_obj.h"
#include "inslog.h"

json_obj::~json_obj()
{
	if (obj_)  
	{  
		json_object_put(obj_);
		obj_ = nullptr;
	} 
}

json_obj::json_obj(const char* str)
{
	obj_ = json_tokener_parse(str);
	if (!obj_) LOGERR("json_tokener_parse fail");
}

json_obj::json_obj(json_object* obj)
{
	obj_ = obj;
}

json_obj::json_obj()
{
	obj_ = json_object_new_object();
	if (!obj_) LOGERR("json_object_new_object fail");
}

bool json_obj::is_obj()
{
	if (!obj_) return false;
	
	auto type = json_object_get_type(obj_);
	if (type == json_type_object)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool json_obj::is_array()
{
	if (!obj_) return false;

	auto type = json_object_get_type(obj_);
	if (type == json_type_array)
	{
		return true;
	}
	else
	{
		return false;
	}
}

int32_t json_obj::length()
{
	if (!obj_) return 0;
	auto ret = json_object_object_length(obj_);
	return ret;
}

std::shared_ptr<json_obj> json_obj::get_obj(const char *key)
{
	if (!obj_) return nullptr;

	json_object* obj_value = nullptr;
	if (!json_object_object_get_ex(obj_, key, &obj_value)) return nullptr;

	json_object_get(obj_value);

	return std::shared_ptr<json_obj>(new json_obj(obj_value));
}

bool json_obj::get_int(const char *key, int32_t& val)
{
	if (!obj_) return false;

	json_object* obj_value;
	if (!json_object_object_get_ex(obj_, key, &obj_value))
	{
		return false;
	}

	val = json_object_get_int(obj_value);

	return true;
}

bool json_obj::get_int64(const char *key, int64_t& val)
{
	if (!obj_) return false;

	json_object* obj_value;
	if (!json_object_object_get_ex(obj_, key, &obj_value))
	{
		return false;
	}

	val = json_object_get_int64(obj_value);

	return true;
}

bool json_obj::get_double(const char *key, double& val)
{
	if (!obj_) return false;

	json_object* obj_value;
	if (!json_object_object_get_ex(obj_, key, &obj_value))
	{
		return false;
	}

	val = json_object_get_double(obj_value);

	return true;
}

bool json_obj::get_string(const char *key, std::string& val)
{
	if (!obj_) return false;

	json_object* obj_value;
	if (!json_object_object_get_ex(obj_, key, &obj_value))
	{
		return false;
	}

	val = json_object_get_string(obj_value);

	return true;
}

bool json_obj::get_boolean(const char *key, bool& val)
{
	if (!obj_) return false;

	json_object* obj_value;
	if (!json_object_object_get_ex(obj_, key, &obj_value))
	{
		return false;
	}

	val = json_object_get_boolean(obj_value);

	return true;
}

std::vector<int32_t> json_obj::get_int_array(const char *key)
{
	std::vector<int32_t> v_result;
	if (!obj_)
	{
		return v_result;
	}

	json_object* obj_value;
	if (!json_object_object_get_ex(obj_, key, &obj_value))
	{
		return v_result;
	}

	int32_t length = json_object_array_length(obj_value);
	//printf("array length:%d\n", length);

	for (int32_t i = 0; i < length; i++)
	{
		auto obj = json_object_array_get_idx(obj_value, i);
		int32_t val = json_object_get_int(obj);
		v_result.push_back(val);
		//printf("array i:%d value:%d\n", i, val);
	}

	return v_result;
}

std::vector<double> json_obj::get_double_array(const char *key)
{
	std::vector<double> v_result;
	if (!obj_)
	{
		return v_result;
	}

	json_object* obj_value;
	if (!json_object_object_get_ex(obj_, key, &obj_value))
	{
		return v_result;
	}

	int32_t length = json_object_array_length(obj_value);
	//printf("array length:%d\n", length);

	for (int32_t i = 0; i < length; i++)
	{
		auto obj = json_object_array_get_idx(obj_value, i);
		double val = json_object_get_double(obj);
		v_result.push_back(val);
		//printf("array i:%d value:%d\n", i, val);
	}

	return v_result;
}

std::vector<std::string> json_obj::get_string_array(const char *key)
{
	std::vector<std::string> v_result;
	if (!obj_)
	{
		return v_result;
	}

	json_object* obj_value;
	if (!json_object_object_get_ex(obj_, key, &obj_value))
	{
		return v_result;
	}

	int32_t length = json_object_array_length(obj_value);
	//printf("array length:%d\n", length);

	for (int32_t i = 0; i < length; i++)
	{
		auto obj = json_object_array_get_idx(obj_value, i);
		std::string val = json_object_get_string(obj);
		v_result.push_back(val);
		//printf("array i:%d value:%d\n", i, val);
	}

	return v_result;
}

std::string json_obj::to_string()
{
	if (!obj_) return "";

	return json_object_to_json_string(obj_);
}

bool json_obj::set_string(const char *key, std::string value)
{
	if (!obj_) return false;

	auto val_obj = json_object_new_string(value.c_str());
	json_object_object_add(obj_, key, val_obj);

	return true;
}

bool json_obj::set_int(const char *key, int32_t value)
{
	if (!obj_) return false;

	auto val_obj = json_object_new_int(value);
	json_object_object_add(obj_, key, val_obj);

	return true;
}

bool json_obj::set_bool(const char *key, bool value)
{
	if (!obj_) return false;

	auto val_obj = json_object_new_boolean(value);
	json_object_object_add(obj_, key, val_obj);

	return true;
}

bool json_obj::set_int64(const char *key, int64_t value)
{
	if (!obj_) return false;

	auto val_obj = json_object_new_int64(value);
	json_object_object_add(obj_, key, val_obj);

	return true;
}

bool json_obj::set_double(const char *key, double value)
{
	if (!obj_) return false;

	auto val_obj = json_object_new_double(value);
	json_object_object_add(obj_, key, val_obj);

	return true;
}

bool json_obj::set_obj(const char *key, const json_obj* value)
{
	if (!obj_) return false;
	if (!value->obj_) return false;

	json_object_object_add(obj_, key, value->obj_);
	json_object_get(value->obj_);

	return true;
}

std::shared_ptr<json_obj> json_obj::new_array()
{
	auto array = json_object_new_array();
	return std::shared_ptr<json_obj>(new json_obj(array));
}

bool json_obj::array_add(json_obj* value)
{
	if (!obj_) return false;
	if (!value) return false;
	if (!value->obj_) return false;

	json_object_array_add(obj_,value->obj_);
	json_object_get(value->obj_);
	return true;
}

bool json_obj::array_add(std::string value)
{
	if (!obj_) return false;

	auto value_obj = json_object_new_string(value.c_str());
	json_object_array_add(obj_, value_obj);
	return true;
}

bool json_obj::array_add(double value)
{
	if (!obj_) return false;

	auto value_obj = json_object_new_double(value);
	json_object_array_add(obj_, value_obj);
	return true;
}

bool json_obj::array_add(bool value)
{
	if (!obj_) return false;

	auto value_obj = json_object_new_boolean(value);
	json_object_array_add(obj_, value_obj);
	return true;
}

bool json_obj::array_add(int32_t value)
{
	if (!obj_) return false;

	auto value_obj = json_object_new_int(value);
	json_object_array_add(obj_, value_obj);
	return true;
}

