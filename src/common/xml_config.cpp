
#include "xml_config.h"
#include <iostream>
#include <assert.h>
#include <inslog.h>
#include <sstream>
#include "common.h"
#include "ins_util.h"
#include "ins_offset_conv.h"
#include <unordered_map>

#define INS_DEFAULT_OFFSET_PANO_4_3 \
"8_2640.000_2640.000_1978.000_0.000_0.000_0.000_5280_3956_31_2640.000_2640.000_1978.000_45.000_0.000_0.000_5280_3956_31_2640.000_2640.000_1978.000_90.000_0.000_0.000_5280_3956_31_2640.000_2640.000_1978.000_135.000_0.000_0.000_5280_3956_31_2640.000_2640.000_1978.000_180.000_0.000_0.000_5280_3956_31_2640.000_2640.000_1978.000_225.000_0.000_0.000_5280_3956_31_2640.000_2640.000_1978.000_270.000_0.000_0.000_5280_3956_31_2640.000_2640.000_1978.000_315.000_0.000_0.000_5280_3956_31_10"

std::mutex xml_config::mtx_;

bool xml_config::is_user_offset_default()
{
	std::string offset = xml_config::get_offset(INS_CROP_FLAG_PIC, INS_OFFSET_USER);
	
	bool res = (offset == INS_DEFAULT_OFFSET_PANO_4_3);

	//LOGINFO("------default offset:%d", res);

	return res;
}

int xml_config::load_file(XMLDocument& xml_doc, std::string filename)
{	
	if (XML_NO_ERROR == xml_doc.LoadFile(filename.c_str()))  return 0;

	LOGERR("load xml file:%s fail:%s", filename.c_str(), xml_doc.ErrorName()); 

	//如果备份配置文件打开失败,使用代码写死配置
	if (recreate_config_file(filename)) return -1;

	if (XML_NO_ERROR == xml_doc.LoadFile(filename.c_str())) 
	{
		return 0;
	}
	else
	{
		LOGERR("reload xml file:%s fail:%s", filename.c_str(), xml_doc.ErrorName()); 
		return -1;
	}
}

int xml_config::save_file(XMLDocument& xml_doc, std::string filename)
{
	if (XML_NO_ERROR != xml_doc.SaveFile(filename.c_str())) 
	{ 
		LOGERR("save xml file fail:%s", filename.c_str(), xml_doc.ErrorName()); 
		return -1; 
	} 
	else
	{	
		return 0;
	}
}

int xml_config::recreate_config_file(std::string filename)
{
	LOGINFO("re create config file:%s", filename.c_str());

	//check dir exist
	ins_util::check_create_dir(filename);

	XMLDocument xml_doc;
	auto dec = xml_doc.NewDeclaration();
	auto e_root = xml_doc.NewElement("insta");
	xml_doc.LinkEndChild(dec);
	xml_doc.LinkEndChild(e_root);
	
	auto e_of = xml_doc.NewElement(INS_CONFIG_OFFSET);
	auto e_offset = xml_doc.NewElement(INS_CONFIG_OFFSET_PANO_4_3);
	e_offset->SetText(INS_DEFAULT_OFFSET_PANO_4_3);
	e_of->LinkEndChild(e_offset);
	e_root->LinkEndChild(e_of);

	auto e_opt = xml_doc.NewElement(INS_CONFIG_OPTION);
	auto e_pid = xml_doc.NewElement(INS_CONFIG_PID);
	e_pid->SetText("8_7_6_5_4_3_2_1");
	e_opt->LinkEndChild(e_pid);
	auto e_storage = xml_doc.NewElement(INS_CONFIG_STORAGE);
	e_storage->SetText("/home/nvidia");
	e_opt->LinkEndChild(e_storage);
	auto e_fixs = xml_doc.NewElement(INS_CONFIG_FIX_STORAGE);
	e_fixs->SetText(0);
	e_opt->LinkEndChild(e_fixs);
	e_root->LinkEndChild(e_opt);

	xml_doc.SaveFile(filename.c_str());
}

XMLElement* xml_config::find_element(XMLDocument& xml_doc, const char* firstname, const char* secondname)
{
	if (!firstname || !secondname)
	{
		//std::cout << "xml: input param is null" << std::endl;
		LOGERR("xml: input param is null");
		return nullptr;
	}
	
	XMLElement* element = xml_doc.RootElement();
	if (!element)
	{
		//std::cout << "xml: cann't find root element" << std::endl;
		LOGERR("xml: cann't find root element");
		return nullptr;
	}
	
	element = element->FirstChildElement(firstname);  
	if (!element)
	{
		//std::cout << "xml: cann't find element " << firstname << std::endl;
		LOGERR("xml: cann't find element:%s", firstname);
		return nullptr;
	}

	auto f_element = element->FirstChildElement(secondname);  
	if (!f_element)
	{
		XMLElement* e = xml_doc.NewElement(secondname);  
		element->LinkEndChild(e);

		//std::cout << "xml: cann't find element " << secondname << std::endl;
		//LOGERR("xml: cann't find element:%s insert elemet", secondname);
		return e;
	}
	else
	{
		return f_element;
	}
}

int xml_config::get_value(const char* firstname, const char* secondname, int& value)
{	
	std::lock_guard<std::mutex> lock(mtx_);

	//std::string filename = get_config_filename(firstname, secondname);
	XMLDocument xml_doc;
	if (load_file(xml_doc, INS_DEFAULT_XML_FILE)) return -1;

	auto element = find_element(xml_doc, firstname, secondname);
	if (element == nullptr) return -1;

	int tmp;
	if (XML_SUCCESS != element->QueryIntText(&tmp))
	{
		//LOGERR("xml: fail to QueryIntText %s", secondname);
		return -1;
	}
	else
	{
		value = tmp;
		return 0;
	}
}

int xml_config::get_value(const char* firstname, const char* secondname, float& value)
{
	std::lock_guard<std::mutex> lock(mtx_);

	//std::string filename = get_config_filename(firstname, secondname);
	XMLDocument xml_doc;
	if (load_file(xml_doc, INS_DEFAULT_XML_FILE)) return -1;

	auto element = find_element(xml_doc,firstname, secondname);
	if (element == nullptr) return -1;

	float tmp;
	if (XML_SUCCESS != element->QueryFloatText(&tmp))
	{
		//LOGERR("xml: fail to QueryFloatText %s", secondname);
		return -1;
	}
	else
	{
		value = tmp;
		return 0;
	}
}

int xml_config::get_value(const char* firstname, const char* secondname, double& value)
{
	std::lock_guard<std::mutex> lock(mtx_);

	//std::string filename = get_config_filename(firstname, secondname);
	XMLDocument xml_doc;
	if (load_file(xml_doc, INS_DEFAULT_XML_FILE)) return -1;

	auto element = find_element(xml_doc, firstname, secondname);
	if (element == nullptr) return -1;

	double tmp;
	if (XML_SUCCESS != element->QueryDoubleText(&tmp))
	{
		//LOGERR("xml: fail to QueryFloatText %s", secondname);
		return -1;
	}
	else
	{
		value = tmp;
		return 0;
	}
}

int xml_config::get_value(const char* firstname, const char* secondname, std::string& value)
{
	std::lock_guard<std::mutex> lock(mtx_);

	//std::string filename = get_config_filename(firstname, secondname);
	XMLDocument xml_doc;
	if (load_file(xml_doc, INS_DEFAULT_XML_FILE)) return -1;

	auto element = find_element(xml_doc, firstname, secondname);
	if (element == nullptr) return -1;

	const char* value_c = element->GetText();
	if (!value_c)
	{
		//LOGERR("xml: fail to get string text %s", secondname);
		return -1;
	}
	else
	{
		value = value_c;
		return 0;
	}
}

int xml_config::set_value(const char* firstname, const char* secondname, int value)
{	
	std::lock_guard<std::mutex> lock(mtx_);

	//std::string filename = get_config_filename(firstname, secondname);
	XMLDocument xml_doc;
	if (load_file(xml_doc, INS_DEFAULT_XML_FILE)) return -1;

	auto element = find_element(xml_doc, firstname, secondname);
	if (element == nullptr) return -1;
	element->SetText(value);

	return save_file(xml_doc, INS_DEFAULT_XML_FILE);
}

int xml_config::set_value(const char* firstname, const char* secondname, float value)
{	
	std::lock_guard<std::mutex> lock(mtx_);

	//std::string filename = get_config_filename(firstname, secondname);
	XMLDocument xml_doc;
	if (load_file(xml_doc, INS_DEFAULT_XML_FILE)) return -1;

	auto element = find_element(xml_doc, firstname, secondname);
	if (element == nullptr) return -1;
	element->SetText(value);
	
	return save_file(xml_doc, INS_DEFAULT_XML_FILE);
}

int xml_config::set_value(const char* firstname, const char* secondname, double value)
{	
	std::lock_guard<std::mutex> lock(mtx_);

	//std::string filename = get_config_filename(firstname, secondname);
	XMLDocument xml_doc;
	if (load_file(xml_doc, INS_DEFAULT_XML_FILE)) return -1;

	auto element = find_element(xml_doc, firstname, secondname);
	if (element == nullptr) return -1;
	element->SetText(value);
	
	return save_file(xml_doc, INS_DEFAULT_XML_FILE);
}

int xml_config::set_value(const char* firstname, const char* secondname, std::string value)
{	
	std::lock_guard<std::mutex> lock(mtx_);

	//std::string filename = get_config_filename(firstname, secondname);
	XMLDocument xml_doc;
	if (load_file(xml_doc, INS_DEFAULT_XML_FILE)) return -1;

	auto element = find_element(xml_doc, firstname, secondname);
	if (element == nullptr) return -1;
	element->SetText(value.c_str());

	return save_file(xml_doc, INS_DEFAULT_XML_FILE);
}

int xml_config::set_factory_offset(std::string& offset)
{
	XMLDocument xml_doc;
	XMLElement* e_root;
	if (XML_NO_ERROR != xml_doc.LoadFile(INS_FACTORY_SETING_XML))
	{
		auto dec = xml_doc.NewDeclaration();
		e_root = xml_doc.NewElement("insta");
		xml_doc.LinkEndChild(dec);
		xml_doc.LinkEndChild(e_root);
	}
	else
	{
		e_root = xml_doc.RootElement();
		if (!e_root) e_root = xml_doc.NewElement("insta");
	}

	if (offset != "")
	{
		auto e_4_3 = e_root->FirstChildElement("offset_pano_4_3");
		if (!e_4_3) e_4_3 = xml_doc.NewElement("offset_pano_4_3");
		e_4_3->SetText(offset.c_str());
		e_root->LinkEndChild(e_4_3);
	}

	xml_doc.SaveFile(INS_FACTORY_SETING_XML);

	usleep(2*1000*1000);

	return INS_OK;
}

std::string xml_config::get_factory_offset()
{
	XMLDocument xml_doc;
	if (XML_NO_ERROR != xml_doc.LoadFile(INS_FACTORY_SETING_XML))
	{
		LOGERR("xml:%s load fail", INS_FACTORY_SETING_XML);
		return "";
	}

	auto e_root = xml_doc.RootElement();
	if (!e_root)
	{
		LOGERR("no root element");
		return "";
	}

	auto e_offset_4 = e_root->FirstChildElement("offset_pano_4_3");
	if (!e_offset_4)
	{
		LOGERR("no offset_pano_4_3");
		return "";
	}

	return e_offset_4->GetText();
}

int xml_config::set_accel_offset(double x, double y, double z)
{
	XMLDocument xml_doc;
	auto dec = xml_doc.NewDeclaration();
	auto e_root = xml_doc.NewElement("insta");
	xml_doc.LinkEndChild(dec);
	xml_doc.LinkEndChild(e_root);

	auto e_acc = xml_doc.NewElement("acc_offset");
	e_acc->SetAttribute("x", x);
	e_acc->SetAttribute("y", y);
	e_acc->SetAttribute("z", z);
	e_root->LinkEndChild(e_acc);

	xml_doc.SaveFile(INS_GYRO_OFFSET_XML);

	usleep(2*1000*1000);

	LOGINFO("set accel offset:%lf %lf %lf", x, y, z);

	return INS_OK;
}

int xml_config::get_accel_offset(double& x, double& y, double& z)
{
	XMLDocument xml_doc;
	if (XML_NO_ERROR != xml_doc.LoadFile(INS_GYRO_OFFSET_XML))  
	{
		LOGERR("load %s fail", INS_GYRO_OFFSET_XML);
		return INS_ERR;
	}

	auto e_root = xml_doc.RootElement();
	if (!e_root)
	{
		LOGERR("cann't find root in %s", INS_GYRO_OFFSET_XML);
		return INS_ERR;
	}

	auto e_acc = e_root->FirstChildElement("acc_offset");
	if (!e_acc)
	{
		LOGERR("cann't find acc element in %s", INS_GYRO_OFFSET_XML);
		return INS_ERR;
	}

	e_acc->QueryAttribute("x", &x);
	e_acc->QueryAttribute("y", &y);
	e_acc->QueryAttribute("z", &z);

	return INS_OK;
}

int xml_config::set_gyro_rotation(const std::vector<double>& quat)
{
	if (quat.size() != 4) return INS_ERR;
	
	XMLDocument xml_doc;
	auto dec = xml_doc.NewDeclaration();
	auto e_root = xml_doc.NewElement("insta");
	xml_doc.LinkEndChild(dec);
	xml_doc.LinkEndChild(e_root);

	auto e_rotation = xml_doc.NewElement("imu_rotation");
	e_rotation->SetAttribute("x", quat[0]);
	e_rotation->SetAttribute("y", quat[1]);
	e_rotation->SetAttribute("z", quat[2]);
	e_rotation->SetAttribute("w", quat[3]);
	e_root->LinkEndChild(e_rotation);

	xml_doc.SaveFile(INS_GYRO_CALIBRATION_XML);

	usleep(2*1000*1000);

	LOGINFO("set gyro rotation: %lf %lf %lf %lf", quat[0], quat[1],quat[2],quat[3]);

	return INS_OK;
}

int xml_config::get_gyro_rotation(std::vector<double>& quat)
{
	XMLDocument xml_doc;
	if (XML_NO_ERROR != xml_doc.LoadFile(INS_GYRO_CALIBRATION_XML))  
	{
		LOGERR("load %s fail", INS_GYRO_CALIBRATION_XML);
		return INS_ERR;
	}

	auto e_root = xml_doc.RootElement();
	if (!e_root)
	{
		LOGERR("cann't find root in %s", INS_GYRO_CALIBRATION_XML);
		return INS_ERR;
	}

	auto e_rotation = e_root->FirstChildElement("imu_rotation");
	if (!e_rotation)
	{
		LOGERR("cann't find imu_rotation element in %s", INS_GYRO_CALIBRATION_XML);
		return INS_ERR;
	}

	double value = 0;
	e_rotation->QueryAttribute("x", &value);
	quat.push_back(value);
	e_rotation->QueryAttribute("y", &value);
	quat.push_back(value);
	e_rotation->QueryAttribute("z", &value);
	quat.push_back(value);
	e_rotation->QueryAttribute("w", &value);
	quat.push_back(value);

	//LOGINFO("get gyro rotation:%lf %lf %lf %lf", quat[0], quat[1],quat[2],quat[3]);

	return INS_OK;
}


std::string xml_config::get_offset(int32_t crop_flag, int32_t type)
{
	//LOGINFO("---------------crop:%d type:%d", crop_flag, type);

	std::string pic_offset;
	if (type == INS_OFFSET_FACTORY) {
		pic_offset = get_factory_offset();
		if (pic_offset == "") {
			LOGERR("----------NO factory offset");
		}
	}
	
	if (pic_offset == "") {
		get_value(INS_CONFIG_OFFSET, INS_CONFIG_OFFSET_PANO_4_3, pic_offset);
		if (pic_offset == "") 
			pic_offset = INS_DEFAULT_OFFSET_PANO_4_3;
	}

	std::string offset;
	ins_offset_conv::conv(pic_offset, offset, crop_flag);

	return offset;
}


int xml_config::get_gyro_delay_time(int32_t w, int32_t h, int32_t framerate, bool hdr)
{
    static std::unordered_map<std::string, int32_t> default_delay =
    {
        {"r_3840x2880_30", 115000},
        {"r_3840x2160_30", 113500},
        {"r_3840x2160_5", 614500},
        {"r_3840x1920_60", 63200},
        {"r_3200x2400_30", 115000},
        {"r_3200x2400_60", 64000},
		{"r_2560_1920_30", 115000},
		{"r_2560_1440_30", 113500},
        {"r_1920x1440_30", 115000},
        {"r_1920x1440_120", 27000},
		{"r_3840x2880_30_hdr", 93000},
		{"r_1920x1440_30_hdr", 93000},
		{"r_2560_1920_30_hdr", 93000},
    };

    std::stringstream ss;
    ss << "r_" << w << "x" << h << "_" << framerate;
	if (hdr) ss << "_hdr";

    int32_t delay;
    auto it = default_delay.find(ss.str());
    if (it == default_delay.end())
    {
        delay = 78000;
    }
    else
    {
        delay = it->second;
    }

    XMLDocument xml_doc;
	if (XML_NO_ERROR != xml_doc.LoadFile(INS_GYRO_DELAY_XML))  
	{
		LOGERR("load %s fail:%s", INS_GYRO_DELAY_XML, xml_doc.ErrorName());
		return delay;
	}

	auto e_root = xml_doc.RootElement();
	if (!e_root)
	{
		LOGERR("cann't find root in %s", INS_GYRO_DELAY_XML);
		return delay;
	}

	auto e_res = e_root->FirstChildElement(ss.str().c_str());
	if (!e_res)
	{
		LOGERR("cann't find %s element in %s", ss.str().c_str(), INS_GYRO_DELAY_XML);
		return delay;
	}

	e_res->QueryAttribute("delay", &delay);

    //LOGINFO("-------%s delay:%d", ss.str().c_str(), delay);

    return delay;
}

