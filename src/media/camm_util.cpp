
#include "camm_util.h"
#include "inslog.h"
#include "common.h"

extern "C" 
{
#include "libavutil/intreadwrite.h"
}

#define CAMM_HEAD_SIZE 4
const uint32_t camm_util::gps_size = CAMM_HEAD_SIZE + sizeof(int32_t) + 3*sizeof(double) + 7*sizeof(float);
const uint32_t camm_util::gyro_size = CAMM_HEAD_SIZE + 3*sizeof(float); //16
const uint32_t camm_util::accel_size = CAMM_HEAD_SIZE + 3*sizeof(float); //16
const uint32_t camm_util::exposure_size = (CAMM_HEAD_SIZE + 2*sizeof(int32_t))*INS_CAM_NUM; //72
const uint32_t camm_util::gyro_accel_size = camm_util::gyro_size + camm_util::accel_size;
const uint32_t camm_util::gyro_accel_mgm_size = camm_util::gyro_accel_size + CAMM_HEAD_SIZE + 3*sizeof(float);
const uint32_t camm_util::camm_size = camm_util::gps_size + camm_util::gyro_size + camm_util::accel_size;

int32_t camm_util::gen_camm_packet(const ins_camm_data& camm, std::shared_ptr<insbuff>& buff)
{
	uint8_t* p = buff->data();

    if (camm.b_gyro)
    {
        p += 2; // reserve
        AV_WL16(p, 2); //type
        p += 2;
        AV_WL32(p, *((int*)&camm.gyro.x));
        p += 4;
        AV_WL32(p, *((int*)&camm.gyro.y));
        p += 4;
        AV_WL32(p, *((int*)&camm.gyro.z));
        p += 4;
    }

    if (camm.b_accel)
    {
        p += 2; // reserve
        AV_WL16(p, 3); //type
        p += 2;
        AV_WL32(p, *((int*)&camm.accel.x));
        p += 4;
        AV_WL32(p, *((int*)&camm.accel.y));
        p += 4;
        AV_WL32(p, *((int*)&camm.accel.z));
        p += 4;
    }

    if (camm.b_gps)
    {
        double ts = (double)camm.gps.pts/1000000.0;

        p += 2; // reserve
        AV_WL16(p, 6); //type
        p += 2;
        AV_WL64(p, *((long long*)&ts));
        p += 8;
        AV_WL32(p, camm.gps.fix_type);
        p += 4;
        AV_WL64(p, *((long long*)&camm.gps.latitude));
        p += 8;
        AV_WL64(p, *((long long*)&camm.gps.longitude));
        p += 8;
        AV_WL32(p, *((int*)&camm.gps.altitude));
        p += 4;
        AV_WL32(p, *((int*)&camm.gps.h_accuracy));
        p += 4;
        AV_WL32(p, *((int*)&camm.gps.v_accuracy));
        p += 4;
        AV_WL32(p, *((int*)&camm.gps.velocity_east));
        p += 4;
        AV_WL32(p, *((int*)&camm.gps.velocity_north));
        p += 4;
        AV_WL32(p, *((int*)&camm.gps.velocity_up));
        p += 4;
        AV_WL32(p, *((int*)&camm.gps.speed_accuracy));
        p += 4;
    }

	return INS_OK;
}

int32_t camm_util::gen_camm_gyro_packet(const ins_gyro_data& gyro, std::shared_ptr<insbuff>& buff)
{
	uint8_t* p = buff->data();

    p += 2; // reserve
    AV_WL16(p, 2); //type
    p += 2;
    AV_WL32(p, *((int*)&gyro.x));
    p += 4;
    AV_WL32(p, *((int*)&gyro.y));
    p += 4;
    AV_WL32(p, *((int*)&gyro.z));
    p += 4;

	return INS_OK;
}

int32_t camm_util::gen_camm_accel_packet(const ins_accel_data& accel, std::shared_ptr<insbuff>& buff)
{
	uint8_t* p = buff->data();

    p += 2; // reserve
    AV_WL16(p, 3); //type
    p += 2;
    AV_WL32(p, *((int*)&accel.x));
    p += 4;
    AV_WL32(p, *((int*)&accel.y));
    p += 4;
    AV_WL32(p, *((int*)&accel.z));
    p += 4;

	return INS_OK;
}

int32_t camm_util::gen_gyro_accel_packet(const amba_gyro_data* data, std::shared_ptr<insbuff>& buff)
{
	uint8_t* p = buff->data();
    //gyro
    p += 2; // reserve
    AV_WL16(p, 2); //type
    p += 2;
    for (int32_t i = 0; i < 3; i++)
    {
        float f = (float)data->gyro[i];
        AV_WL32(p, *((int*)&f));
        p += 4;
    }
    
    //accel
    p += 2; // reserve
    AV_WL16(p, 3); //type
    p += 2;
    for (int32_t i = 0; i < 3; i++)
    {
        float f = (float)data->accel[i];
        AV_WL32(p, *((int*)&f));
        p += 4;
    }

	return INS_OK;
}

int32_t camm_util::gen_gyro_accel_mgm_packet(const amba_gyro_data* data, std::shared_ptr<insbuff>& buff)
{
	uint8_t* p = buff->data();
    //gyro
    p += 2; // reserve
    AV_WL16(p, 2); //type
    p += 2;
    for (int32_t i = 0; i < 3; i++)
    {
        float f = (float)data->gyro[i];
        AV_WL32(p, *((int*)&f));
        p += 4;
    }
    
    //accel
    p += 2; // reserve
    AV_WL16(p, 3); //type
    p += 2;
    for (int32_t i = 0; i < 3; i++)
    {
        float f = (float)data->accel[i];
        AV_WL32(p, *((int*)&f));
        p += 4;
    }

    #ifdef GYRO_EXT
    //magmeter
    p += 2; // reserve
    AV_WL16(p, 4); //type
    p += 2;
    for (int32_t i = 0; i < 3; i++)
    {
        float f = (float)data->position[i];
        AV_WL32(p, *((int*)&f));
        p += 4;
    }
    #endif

	return INS_OK;
}

int32_t camm_util::gen_camm_exposure_packet(const exposure_times* exp, std::shared_ptr<insbuff>& buff)
{
	uint8_t* p = buff->data();

    for (int32_t i = 0; i < INS_CAM_NUM; i++)
    {
        p += 2; // reserve
        AV_WL16(p, 1); //type
        p += 2;
        AV_WL32(p, exp->expousre_time_ns[i]);
        p += 4;
        AV_WL32(p, 0);
        p += 4;
    }

	return INS_OK;
}

int32_t camm_util::gen_camm_gps_packet(const ins_gps_data* gps, std::shared_ptr<insbuff>& buff)
{
	uint8_t* p = buff->data();

    double ts = (double)gps->pts/1000000.0;

    p += 2; // reserve
    AV_WL16(p, 6); //type
    p += 2;
    AV_WL64(p, *((long long*)&ts));
    p += 8;
    AV_WL32(p, gps->fix_type);
    p += 4;
    AV_WL64(p, *((long long*)&gps->latitude));
    p += 8;
    AV_WL64(p, *((long long*)&gps->longitude));
    p += 8;
    AV_WL32(p, *((int*)&gps->altitude));
    p += 4;
    AV_WL32(p, *((int*)&gps->h_accuracy));
    p += 4;
    AV_WL32(p, *((int*)&gps->v_accuracy));
    p += 4;
    AV_WL32(p, *((int*)&gps->velocity_east));
    p += 4;
    AV_WL32(p, *((int*)&gps->velocity_north));
    p += 4;
    AV_WL32(p, *((int*)&gps->velocity_up));
    p += 4;
    AV_WL32(p, *((int*)&gps->speed_accuracy));
    p += 4;

	return INS_OK;
}