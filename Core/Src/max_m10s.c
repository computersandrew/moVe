#include "max_m10s.h"

#include <stddef.h>
#include <string.h>

#define UBX_SYNC_1                  0xB5u
#define UBX_SYNC_2                  0x62u
#define UBX_CLASS_NAV               0x01u
#define UBX_ID_NAV_PVT              0x07u
#define UBX_ID_NAV_VELNED           0x12u
#define UBX_NAV_PVT_LEN             92u
#define UBX_NAV_VELNED_LEN          36u
#define UBX_MAX_PAYLOAD             100u
#define NMEA_MAX_FIELDS             24u
#define KNOTS_TO_MPS                0.514444444f
#define CMPS_TO_MPS                 0.01f
#define MMPS_TO_MPS                 0.001f
#define MM_TO_M                     0.001f
#define UBX_HEADING_SCALE           0.00001f

static int32_t i32_le(const uint8_t *data)
{
    return (int32_t)(((uint32_t)data[3] << 24) |
                     ((uint32_t)data[2] << 16) |
                     ((uint32_t)data[1] << 8) |
                     data[0]);
}

static uint8_t hex_nibble(char c, uint8_t *value)
{
    if ((c >= '0') && (c <= '9')) {
        *value = (uint8_t)(c - '0');
        return 1u;
    }
    if ((c >= 'A') && (c <= 'F')) {
        *value = (uint8_t)(c - 'A' + 10);
        return 1u;
    }
    if ((c >= 'a') && (c <= 'f')) {
        *value = (uint8_t)(c - 'a' + 10);
        return 1u;
    }

    return 0u;
}

static uint8_t parse_float_field(const char *text, float *value)
{
    float result = 0.0f;
    float scale = 0.1f;
    float sign = 1.0f;
    uint8_t seen_digit = 0u;
    uint8_t after_decimal = 0u;

    if ((text == NULL) || (value == NULL) || (*text == '\0')) {
        return 0u;
    }

    if (*text == '-') {
        sign = -1.0f;
        text++;
    } else if (*text == '+') {
        text++;
    }

    while (*text != '\0') {
        if (*text == '.') {
            if (after_decimal != 0u) {
                return 0u;
            }
            after_decimal = 1u;
        } else if ((*text >= '0') && (*text <= '9')) {
            seen_digit = 1u;
            if (after_decimal == 0u) {
                result = (result * 10.0f) + (float)(*text - '0');
            } else {
                result += (float)(*text - '0') * scale;
                scale *= 0.1f;
            }
        } else {
            return 0u;
        }

        text++;
    }

    *value = sign * result;
    return seen_digit;
}

static uint8_t parse_lat_lon(const char *text, const char *hemisphere, float *degrees)
{
    float raw;
    int whole_degrees;
    float minutes;

    if ((hemisphere == NULL) || (degrees == NULL) ||
        (parse_float_field(text, &raw) == 0u)) {
        return 0u;
    }

    whole_degrees = (int)(raw / 100.0f);
    minutes = raw - ((float)whole_degrees * 100.0f);
    *degrees = (float)whole_degrees + (minutes / 60.0f);

    if ((*hemisphere == 'S') || (*hemisphere == 'W')) {
        *degrees = -*degrees;
    }

    return 1u;
}

static uint8_t sentence_type_is(const char *field, const char *type)
{
    const size_t field_len = strlen(field);

    if (field_len < 3u) {
        return 0u;
    }

    return (strncmp(&field[field_len - 3u], type, 3u) == 0) ? 1u : 0u;
}

static uint8_t split_fields(char *line, char **fields, uint8_t max_fields)
{
    uint8_t count = 0u;

    fields[count++] = line;
    while ((*line != '\0') && (count < max_fields)) {
        if (*line == ',') {
            *line = '\0';
            fields[count++] = line + 1;
        }
        line++;
    }

    return count;
}

static uint8_t validate_nmea_checksum(const char *line, uint16_t *body_len)
{
    uint8_t checksum = 0u;
    uint8_t expected_hi;
    uint8_t expected_lo;
    uint8_t expected;
    uint16_t i = 1u;

    if ((line == NULL) || (line[0] != '$')) {
        return 0u;
    }

    while ((line[i] != '\0') && (line[i] != '*')) {
        checksum ^= (uint8_t)line[i];
        i++;
    }

    if ((line[i] != '*') ||
        (hex_nibble(line[i + 1u], &expected_hi) == 0u) ||
        (hex_nibble(line[i + 2u], &expected_lo) == 0u)) {
        return 0u;
    }

    expected = (uint8_t)((expected_hi << 4) | expected_lo);
    if (checksum != expected) {
        return 0u;
    }

    *body_len = (uint16_t)(i - 1u);
    return 1u;
}

static uint8_t parse_nmea_line(const char *line, MAXM10S_NavSample *sample)
{
    char body[128];
    char *fields[NMEA_MAX_FIELDS];
    uint16_t body_len = 0u;
    uint8_t field_count;
    uint8_t updates = MAXM10S_UPDATE_NONE;
    float value;

    if ((line == NULL) || (sample == NULL) ||
        (validate_nmea_checksum(line, &body_len) == 0u) ||
        (body_len >= sizeof(body))) {
        return MAXM10S_UPDATE_NONE;
    }

    memcpy(body, &line[1], body_len);
    body[body_len] = '\0';
    field_count = split_fields(body, fields, NMEA_MAX_FIELDS);

    if ((field_count > 8u) && (sentence_type_is(fields[0], "RMC") != 0u)) {
        sample->fix_valid = (fields[2][0] == 'A') ? 1u : 0u;
        if (parse_lat_lon(fields[3], fields[4], &sample->latitude_deg) != 0u) {
            updates |= MAXM10S_UPDATE_NAV;
        }
        if (parse_lat_lon(fields[5], fields[6], &sample->longitude_deg) != 0u) {
            updates |= MAXM10S_UPDATE_NAV;
        }
        if (parse_float_field(fields[7], &value) != 0u) {
            sample->ground_speed_mps = value * KNOTS_TO_MPS;
            sample->speed_valid = sample->fix_valid;
            updates |= MAXM10S_UPDATE_SPEED;
        }
        if (parse_float_field(fields[8], &value) != 0u) {
            sample->course_deg = value;
            updates |= MAXM10S_UPDATE_NAV;
        }
    } else if ((field_count > 9u) && (sentence_type_is(fields[0], "GGA") != 0u)) {
        sample->fix_valid = (fields[6][0] > '0') ? 1u : 0u;
        sample->num_sats = 0u;
        if (parse_float_field(fields[7], &value) != 0u) {
            sample->num_sats = (uint8_t)value;
        }
        if (parse_float_field(fields[9], &sample->altitude_m) != 0u) {
            sample->altitude_valid = sample->fix_valid;
            updates |= MAXM10S_UPDATE_ALTITUDE;
        }
        if (parse_lat_lon(fields[2], fields[3], &sample->latitude_deg) != 0u) {
            updates |= MAXM10S_UPDATE_NAV;
        }
        if (parse_lat_lon(fields[4], fields[5], &sample->longitude_deg) != 0u) {
            updates |= MAXM10S_UPDATE_NAV;
        }
    } else if ((field_count > 7u) && (sentence_type_is(fields[0], "VTG") != 0u)) {
        if (parse_float_field(fields[1], &value) != 0u) {
            sample->course_deg = value;
            updates |= MAXM10S_UPDATE_NAV;
        }
        if (parse_float_field(fields[5], &value) != 0u) {
            sample->ground_speed_mps = value * KNOTS_TO_MPS;
            sample->speed_valid = 1u;
            updates |= MAXM10S_UPDATE_SPEED;
        }
    }

    if (updates != MAXM10S_UPDATE_NONE) {
        sample->last_protocol = MAXM10S_PROTOCOL_NMEA;
    }

    return updates;
}

static void ubx_checksum_add(MAXM10S_Parser *parser, uint8_t byte)
{
    parser->ubx_ck_a = (uint8_t)(parser->ubx_ck_a + byte);
    parser->ubx_ck_b = (uint8_t)(parser->ubx_ck_b + parser->ubx_ck_a);
}

static uint8_t parse_ubx_nav_pvt(const uint8_t *payload, MAXM10S_NavSample *sample)
{
    const uint8_t fix_type = payload[20];
    const uint8_t flags = payload[21];
    const uint8_t gnss_fix_ok = flags & 0x01u;

    sample->fix_valid = ((gnss_fix_ok != 0u) && (fix_type >= 3u)) ? 1u : 0u;
    sample->num_sats = payload[23];
    sample->longitude_deg = (float)i32_le(&payload[24]) * 0.0000001f;
    sample->latitude_deg = (float)i32_le(&payload[28]) * 0.0000001f;
    sample->altitude_m = (float)i32_le(&payload[36]) * MM_TO_M;
    sample->ground_speed_mps = (float)i32_le(&payload[60]) * MMPS_TO_MPS;
    sample->course_deg = (float)i32_le(&payload[64]) * UBX_HEADING_SCALE;
    sample->altitude_valid = sample->fix_valid;
    sample->speed_valid = sample->fix_valid;
    sample->last_protocol = MAXM10S_PROTOCOL_UBX;

    return MAXM10S_UPDATE_NAV | MAXM10S_UPDATE_ALTITUDE | MAXM10S_UPDATE_SPEED;
}

static uint8_t parse_ubx_nav_velned(const uint8_t *payload, MAXM10S_NavSample *sample)
{
    sample->ground_speed_mps = (float)i32_le(&payload[20]) * CMPS_TO_MPS;
    sample->course_deg = (float)i32_le(&payload[24]) * UBX_HEADING_SCALE;
    sample->speed_valid = 1u;
    sample->last_protocol = MAXM10S_PROTOCOL_UBX;

    return MAXM10S_UPDATE_SPEED;
}

static uint8_t parse_ubx_message(MAXM10S_Parser *parser, MAXM10S_NavSample *sample)
{
    if ((parser->ubx_class == UBX_CLASS_NAV) &&
        (parser->ubx_id == UBX_ID_NAV_PVT) &&
        (parser->ubx_length >= UBX_NAV_PVT_LEN)) {
        return parse_ubx_nav_pvt(parser->ubx_payload, sample);
    }

    if ((parser->ubx_class == UBX_CLASS_NAV) &&
        (parser->ubx_id == UBX_ID_NAV_VELNED) &&
        (parser->ubx_length >= UBX_NAV_VELNED_LEN)) {
        return parse_ubx_nav_velned(parser->ubx_payload, sample);
    }

    return MAXM10S_UPDATE_NONE;
}

static void reset_ubx(MAXM10S_Parser *parser)
{
    parser->ubx_state = MAXM10S_UBX_SYNC1;
    parser->ubx_class = 0u;
    parser->ubx_id = 0u;
    parser->ubx_length = 0u;
    parser->ubx_index = 0u;
    parser->ubx_ck_a = 0u;
    parser->ubx_ck_b = 0u;
}

static uint8_t process_ubx_byte(MAXM10S_Parser *parser,
                                uint8_t byte,
                                MAXM10S_NavSample *sample)
{
    uint8_t updates = MAXM10S_UPDATE_NONE;

    switch (parser->ubx_state) {
    case MAXM10S_UBX_SYNC1:
        if (byte == UBX_SYNC_1) {
            parser->ubx_state = MAXM10S_UBX_SYNC2;
        }
        break;

    case MAXM10S_UBX_SYNC2:
        parser->ubx_state = (byte == UBX_SYNC_2) ? MAXM10S_UBX_CLASS : MAXM10S_UBX_SYNC1;
        break;

    case MAXM10S_UBX_CLASS:
        parser->ubx_class = byte;
        parser->ubx_ck_a = 0u;
        parser->ubx_ck_b = 0u;
        ubx_checksum_add(parser, byte);
        parser->ubx_state = MAXM10S_UBX_ID;
        break;

    case MAXM10S_UBX_ID:
        parser->ubx_id = byte;
        ubx_checksum_add(parser, byte);
        parser->ubx_state = MAXM10S_UBX_LEN1;
        break;

    case MAXM10S_UBX_LEN1:
        parser->ubx_length = byte;
        ubx_checksum_add(parser, byte);
        parser->ubx_state = MAXM10S_UBX_LEN2;
        break;

    case MAXM10S_UBX_LEN2:
        parser->ubx_length |= (uint16_t)byte << 8;
        ubx_checksum_add(parser, byte);
        parser->ubx_index = 0u;
        if (parser->ubx_length > UBX_MAX_PAYLOAD) {
            reset_ubx(parser);
        } else if (parser->ubx_length == 0u) {
            parser->ubx_state = MAXM10S_UBX_CKA;
        } else {
            parser->ubx_state = MAXM10S_UBX_PAYLOAD;
        }
        break;

    case MAXM10S_UBX_PAYLOAD:
        parser->ubx_payload[parser->ubx_index++] = byte;
        ubx_checksum_add(parser, byte);
        if (parser->ubx_index >= parser->ubx_length) {
            parser->ubx_state = MAXM10S_UBX_CKA;
        }
        break;

    case MAXM10S_UBX_CKA:
        if (byte == parser->ubx_ck_a) {
            parser->ubx_state = MAXM10S_UBX_CKB;
        } else {
            reset_ubx(parser);
        }
        break;

    case MAXM10S_UBX_CKB:
        if (byte == parser->ubx_ck_b) {
            updates = parse_ubx_message(parser, sample);
        }
        reset_ubx(parser);
        break;

    default:
        reset_ubx(parser);
        break;
    }

    return updates;
}

static uint8_t process_nmea_byte(MAXM10S_Parser *parser,
                                 uint8_t byte,
                                 MAXM10S_NavSample *sample)
{
    if (byte == '$') {
        parser->nmea_active = 1u;
        parser->nmea_index = 0u;
        parser->nmea_line[parser->nmea_index++] = (char)byte;
        return MAXM10S_UPDATE_NONE;
    }

    if (parser->nmea_active == 0u) {
        return MAXM10S_UPDATE_NONE;
    }

    if ((byte == '\r') || (byte == '\n')) {
        parser->nmea_line[parser->nmea_index] = '\0';
        parser->nmea_active = 0u;
        return parse_nmea_line(parser->nmea_line, sample);
    }

    if (parser->nmea_index >= (sizeof(parser->nmea_line) - 1u)) {
        parser->nmea_active = 0u;
        parser->nmea_index = 0u;
        return MAXM10S_UPDATE_NONE;
    }

    parser->nmea_line[parser->nmea_index++] = (char)byte;
    return MAXM10S_UPDATE_NONE;
}

void MAXM10S_Init(MAXM10S_Parser *parser)
{
    if (parser == NULL) {
        return;
    }

    memset(parser, 0, sizeof(*parser));
    reset_ubx(parser);
}

uint8_t MAXM10S_ProcessByte(MAXM10S_Parser *parser,
                            uint8_t byte,
                            MAXM10S_NavSample *sample)
{
    uint8_t updates;

    if ((parser == NULL) || (sample == NULL)) {
        return MAXM10S_UPDATE_NONE;
    }

    updates = process_ubx_byte(parser, byte, sample);
    updates |= process_nmea_byte(parser, byte, sample);

    return updates;
}

uint8_t MAXM10S_ProcessBuffer(MAXM10S_Parser *parser,
                              const uint8_t *data,
                              uint16_t length,
                              MAXM10S_NavSample *sample)
{
    uint16_t i;
    uint8_t updates = MAXM10S_UPDATE_NONE;

    if ((parser == NULL) || (data == NULL) || (sample == NULL)) {
        return MAXM10S_UPDATE_NONE;
    }

    for (i = 0u; i < length; i++) {
        updates |= MAXM10S_ProcessByte(parser, data[i], sample);
    }

    return updates;
}
