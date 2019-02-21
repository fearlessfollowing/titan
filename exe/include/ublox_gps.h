#ifndef __UBLOX_GPS_H__
#define __UBLOX_GPS_H__
#include "gps.h"
#include <stdint.h>

typedef struct {
    int fix_type;   // -1: poll failed 0/1: 未定位, 2: 二维定位, 3: 三维定位
    double latitude;
    double longitude;
    float altitude;
    float h_accuracy;
    float v_accuracy;
    float velocity_east;
    float velocity_north;
    float velocity_up;
    float speed_accuracy;
    double timestamp;
    GpsSvStatus status;
} GoogleGpsData;

int ins_gpsInit(const char *node);
void ins_gpsRelease();

void ins_gpsSetCallback(void(* callback)(GoogleGpsData));

int insgps_set_cmd(int fd, const uint8_t *cmd, size_t len);

#endif
