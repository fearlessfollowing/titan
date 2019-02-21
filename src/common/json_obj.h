
#ifndef _JSON_OBJ_H_
#define _JSON_OBJ_H_

#include "json-c/json.h"
#include <string>
#include <vector>
#include <memory>

class json_obj
{
public:
	~json_obj();
	json_obj(const char* str); //parse a json string
	json_obj(); //create a json null obj

	bool is_array();
	bool is_obj();
	int32_t length();//field count

	//get
	std::shared_ptr<json_obj> get_obj(const char *key);
	bool get_int(const char *key, int32_t& val);
	bool get_int64(const char *key, int64_t& val);
	bool get_double(const char *key, double& val);
	bool get_string(const char *key, std::string& val);
	bool get_boolean(const char *key, bool& val);
	std::vector<int32_t> get_int_array(const char *key);
	std::vector<std::string> get_string_array(const char *key);
	std::vector<double> get_double_array(const char *key);

	//set
	std::string to_string();
	bool set_string(const char *key, std::string value);
	bool set_int(const char *key, int32_t value);
	bool set_bool(const char *key, bool value);
	bool set_int64(const char *key, int64_t value);
	bool set_double(const char *key, double value);
	bool set_obj(const char *key, const json_obj* value);

	//array
	static std::shared_ptr<json_obj> new_array();
	bool array_add(json_obj* value);
	bool array_add(std::string value);
	bool array_add(double value);
	bool array_add(bool value);
	bool array_add(int32_t value);

private:
	json_obj(json_object* obj);
	json_object* obj_ = nullptr;
};

#endif
