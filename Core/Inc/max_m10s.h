#ifndef MAX_M10S_H
#define MAX_M10S_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAXM10S_UPDATE_NONE          0x00u
#define MAXM10S_UPDATE_NAV           0x01u
#define MAXM10S_UPDATE_ALTITUDE      0x02u
#define MAXM10S_UPDATE_SPEED         0x04u

typedef enum
{
    MAXM10S_PROTOCOL_NONE = 0,
    MAXM10S_PROTOCOL_NMEA = 1,
    MAXM10S_PROTOCOL_UBX = 2
} MAXM10S_Protocol;

typedef struct
{
    float latitude_deg;
    float longitude_deg;
    float altitude_m;
    float ground_speed_mps;
    float course_deg;
    uint8_t fix_valid;
    uint8_t altitude_valid;
    uint8_t speed_valid;
    uint8_t num_sats;
    MAXM10S_Protocol last_protocol;
} MAXM10S_NavSample;

typedef enum
{
    MAXM10S_UBX_SYNC1 = 0,
    MAXM10S_UBX_SYNC2,
    MAXM10S_UBX_CLASS,
    MAXM10S_UBX_ID,
    MAXM10S_UBX_LEN1,
    MAXM10S_UBX_LEN2,
    MAXM10S_UBX_PAYLOAD,
    MAXM10S_UBX_CKA,
    MAXM10S_UBX_CKB
} MAXM10S_UbxState;

typedef struct
{
    MAXM10S_UbxState ubx_state;
    uint8_t ubx_class;
    uint8_t ubx_id;
    uint16_t ubx_length;
    uint16_t ubx_index;
    uint8_t ubx_ck_a;
    uint8_t ubx_ck_b;
    uint8_t ubx_payload[100];
    char nmea_line[128];
    uint16_t nmea_index;
    uint8_t nmea_active;
} MAXM10S_Parser;

void MAXM10S_Init(MAXM10S_Parser *parser);
uint8_t MAXM10S_ProcessByte(MAXM10S_Parser *parser,
                            uint8_t byte,
                            MAXM10S_NavSample *sample);
uint8_t MAXM10S_ProcessBuffer(MAXM10S_Parser *parser,
                              const uint8_t *data,
                              uint16_t length,
                              MAXM10S_NavSample *sample);

#ifdef __cplusplus
}
#endif

#endif
