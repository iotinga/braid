#include "cbor.h"
#include "esp_err.h"

#include "build_config.h"
#include "core/time.h"
#include "shadow.h"

#define VERSION_STR_SHORT                                                                                              \
    (XSTR(CFG_FW_VERSION_MAJOR) "." XSTR(CFG_FW_VERSION_MINOR) "." XSTR(CFG_FW_VERSION_PATCH) "-" XSTR(                \
        CFG_FW_VERSION_COMMIT))

static const char *TAG = "net/shadow";

static void Shadow_PayloadEncode(CborEncoder *parent, const ShadowPayload *payload) {
    CborEncoder pl;
    cbor_encoder_create_map(parent, &pl, CborIndefiniteLength);

    cbor_encode_text_stringz(&pl, "FW_VER");
    cbor_encode_text_stringz(&pl, VERSION_STR_SHORT);
    cbor_encode_text_stringz(&pl, "DELAY");
    cbor_encode_uint(&pl, payload->reportDelay);

    cbor_encode_text_stringz(&pl, "HUMID");
    cbor_encode_int(&pl, payload->sensorData.humidity);
    cbor_encode_text_stringz(&pl, "TEMP");
    cbor_encode_int(&pl, payload->sensorData.temperature);

    cbor_encode_text_stringz(&pl, "ACC_X");
    cbor_encode_float(&pl, payload->sensorData.acceleration_mg[0]);
    cbor_encode_text_stringz(&pl, "ACC_Y");
    cbor_encode_float(&pl, payload->sensorData.acceleration_mg[1]);
    cbor_encode_text_stringz(&pl, "ACC_Z");
    cbor_encode_float(&pl, payload->sensorData.acceleration_mg[2]);

    cbor_encode_text_stringz(&pl, "LAT");
    cbor_encode_float(&pl, payload->gpsPosition.lat);
    cbor_encode_text_stringz(&pl, "LON");
    cbor_encode_float(&pl, payload->gpsPosition.lon);
    cbor_encode_text_stringz(&pl, "HSPEED");
    cbor_encode_float(&pl, payload->gpsPosition.speed);
    cbor_encode_text_stringz(&pl, "DIR");
    cbor_encode_float(&pl, payload->gpsPosition.direction);
    cbor_encode_text_stringz(&pl, "ALT");
    cbor_encode_float(&pl, payload->gpsPosition.alt);
    cbor_encode_text_stringz(&pl, "H_ACC");
    cbor_encode_float(&pl, payload->gpsPosition.accuracy);

    cbor_encoder_close_container(parent, &pl);
}

size_t Shadow_Encode(uint8_t version, Action action, const ShadowPayload *payload, uint8_t *buf, size_t bufSize) {
    CborEncoder root, rootMap;
    cbor_encoder_init(&root, buf, bufSize, 0);
    cbor_encoder_create_map(&root, &rootMap, CborIndefiniteLength);

    cbor_encode_text_stringz(&rootMap, "TS");
    cbor_encode_uint(&rootMap, Time_GetUnixTimestamp());

    cbor_encode_text_stringz(&rootMap, "VER");
    cbor_encode_uint(&rootMap, version);

    cbor_encode_text_stringz(&rootMap, "PROT");
    cbor_encode_uint(&root, 1);

    cbor_encode_text_stringz(&rootMap, "ACTION");
    cbor_encode_uint(&rootMap, action);

    cbor_encode_text_stringz(&rootMap, "STATUS");
    cbor_encode_uint(&rootMap, 0);

    cbor_encode_text_stringz(&rootMap, "BODY");
    Shadow_PayloadEncode(&rootMap, payload);

    cbor_encoder_close_container(&root, &rootMap);
    return cbor_encoder_get_buffer_size(&root, buf);
}

#define CBOR_CHECK(x)                                                                                                  \
    do {                                                                                                               \
        CborError __err = (x);                                                                                         \
        if (__err != CborNoError) {                                                                                    \
            ESP_LOGW(TAG, "CBOR error (%d) at %s:%d: " #x, __err, basename(__FILE__), __LINE__);                       \
            return ESP_FAIL;                                                                                           \
        }                                                                                                              \
    } while (0)

#define CBOR_CHECK_TYPE(value, type)                                                                                   \
    do {                                                                                                               \
        CborType __type = cbor_value_get_type(value);                                                                  \
        if (__type != (type)) {                                                                                        \
            ESP_LOGW(TAG, "CBOR error (type %d != %d) at %s:%d: ", __type, type, basename(__FILE__), __LINE__);        \
            return ESP_FAIL;                                                                                           \
        }                                                                                                              \
    } while (0)

#define CBOR_GET_FLOAT_OR_DOUBLE(value, outFloat)                                                                      \
    do {                                                                                                               \
        switch (cbor_value_get_type(value)) {                                                                          \
        case CborDoubleType:                                                                                           \
            double __double_val = 0;                                                                                   \
            CBOR_CHECK(cbor_value_get_double(value, &__double_val));                                                   \
            *outFloat = (float)__double_val;                                                                           \
            break;                                                                                                     \
        case CborFloatType:                                                                                            \
            CBOR_CHECK(cbor_value_get_float(value, outFloat));                                                         \
            break;                                                                                                     \
        default:                                                                                                       \
            ESP_LOGW(TAG, "Value for is not double nor float");                                                        \
            *outFloat = 0.0f;                                                                                          \
        }                                                                                                              \
    } while (0)

static esp_err_t Shadow_DecodePayload(CborValue *parent, ShadowPayload *payload) {
    CborValue pl;

    payload->sensorData = (SensorData){
        .humidity = 0.0,
        .temperature = 0.0,
        .acceleration_mg = {0},
    };

    CBOR_CHECK(cbor_value_enter_container(parent, &pl));

    while (!cbor_value_at_end(&pl)) {
        char keyBuf[16] = {0};
        size_t keyLength = sizeof(keyBuf);

        CBOR_CHECK_TYPE(&pl, CborTextStringType);
        CBOR_CHECK(cbor_value_copy_text_string(&pl, keyBuf, &keyLength, &pl));

        ESP_LOGD(TAG, "Found payload key '%s'", keyBuf);

        if (strcmp(keyBuf, "FW_VER") == 0) {
            CBOR_CHECK_TYPE(&pl, CborTextStringType);
        } else if (strcmp(keyBuf, "DELAY") == 0) {
            CBOR_CHECK_TYPE(&pl, CborIntegerType);

            int64_t value;
            CBOR_CHECK(cbor_value_get_int64(&pl, &value));

            payload->reportDelay = (uint32_t)value;
        } else if (strcmp(keyBuf, "HUMID") == 0) {
            CBOR_CHECK_TYPE(&pl, CborIntegerType);
            int64_t value;
            CBOR_CHECK(cbor_value_get_int64(&pl, &value));
        } else if (strcmp(keyBuf, "TEMP") == 0) {
            CBOR_CHECK_TYPE(&pl, CborIntegerType);
            int64_t value;
            CBOR_CHECK(cbor_value_get_int64(&pl, &value));
        } else if (strcmp(keyBuf, "ACC_X") == 0) {
            float value = 0;
            CBOR_GET_FLOAT_OR_DOUBLE(&pl, &value);
        } else if (strcmp(keyBuf, "ACC_Y") == 0) {
            float value = 0;
            CBOR_GET_FLOAT_OR_DOUBLE(&pl, &value);
        } else if (strcmp(keyBuf, "ACC_Z") == 0) {
            float value = 0;
            CBOR_GET_FLOAT_OR_DOUBLE(&pl, &value);
        } else if (strcmp(keyBuf, "LAT") == 0) {
            float value = 0;
            CBOR_GET_FLOAT_OR_DOUBLE(&pl, &value);
        } else if (strcmp(keyBuf, "LON") == 0) {
            float value = 0;
            CBOR_GET_FLOAT_OR_DOUBLE(&pl, &value);
        } else if (strcmp(keyBuf, "HSPEED") == 0) {
            float value = 0;
            CBOR_GET_FLOAT_OR_DOUBLE(&pl, &value);
        } else if (strcmp(keyBuf, "DIR") == 0) {
            float value = 0;
            CBOR_GET_FLOAT_OR_DOUBLE(&pl, &value);
        } else if (strcmp(keyBuf, "ALT") == 0) {
            float value = 0;
            CBOR_GET_FLOAT_OR_DOUBLE(&pl, &value);
        } else if (strcmp(keyBuf, "H_ACC") == 0) {
            float value = 0;
            CBOR_GET_FLOAT_OR_DOUBLE(&pl, &value);
        } else {
            ESP_LOGW(TAG, "Unknown payload key: '%s'", keyBuf);
        }

        CBOR_CHECK(cbor_value_advance(&pl));
    }

    CBOR_CHECK(cbor_value_leave_container(parent, &pl));

    return ESP_OK;
}

esp_err_t Shadow_Decode(const uint8_t *buf, size_t bufSize, ShadowPayload *payload, ShadowHeader *header) {
    CborParser parser;
    CborValue rootIt;
    CBOR_CHECK(cbor_parser_init(buf, bufSize, 0, &parser, &rootIt));

    CBOR_CHECK(cbor_value_validate(&rootIt, CborValidateBasic));
    CBOR_CHECK_TYPE(&rootIt, CborMapType);

    CborValue rootMapIt;
    CBOR_CHECK(cbor_value_enter_container(&rootIt, &rootMapIt));

    while (!cbor_value_at_end(&rootMapIt)) {
        char keyBuf[16] = {0};
        size_t keyLength = sizeof(keyBuf);

        CBOR_CHECK_TYPE(&rootMapIt, CborTextStringType);
        CBOR_CHECK(cbor_value_copy_text_string(&rootMapIt, keyBuf, &keyLength, &rootMapIt));

        ESP_LOGD(TAG, "Found shadow key '%s'", keyBuf);

        if (strcmp(keyBuf, "TS") == 0 && (header != NULL)) {
            CBOR_CHECK_TYPE(&rootMapIt, CborIntegerType);

            int64_t value;
            CBOR_CHECK(cbor_value_get_int64(&rootMapIt, &value));

            header->ts = (uint32_t)value;
        } else if (strcmp(keyBuf, "VER") == 0 && (header != NULL)) {
            CBOR_CHECK_TYPE(&rootMapIt, CborIntegerType);

            int64_t value;
            CBOR_CHECK(cbor_value_get_int64(&rootMapIt, &value));

            header->ver = (uint8_t)value;
        } else if (strcmp(keyBuf, "PROT") == 0 && (header != NULL)) {
            CBOR_CHECK_TYPE(&rootMapIt, CborIntegerType);

            int64_t value;
            CBOR_CHECK(cbor_value_get_int64(&rootMapIt, &value));

            header->prot = (uint8_t)value;
        } else if (strcmp(keyBuf, "ACTION") == 0 && (header != NULL)) {
            CBOR_CHECK_TYPE(&rootMapIt, CborIntegerType);

            int64_t value;
            CBOR_CHECK(cbor_value_get_int64(&rootMapIt, &value));

            header->action = (Action)value;
        } else if (strcmp(keyBuf, "STATUS") == 0 && (header != NULL)) {
            CBOR_CHECK_TYPE(&rootMapIt, CborIntegerType);

            int64_t value;
            CBOR_CHECK(cbor_value_get_int64(&rootMapIt, &value));

            header->status = (uint16_t)value;
        } else if (strcmp(keyBuf, "BODY") == 0) {
            CBOR_CHECK_TYPE(&rootMapIt, CborMapType);

            ESP_ERROR_CHECK(Shadow_DecodePayload(&rootMapIt, payload));
        } else {
            ESP_LOGW(TAG, "Unknown shadow key: '%s'", keyBuf);
        }

        if (!cbor_value_at_end(&rootMapIt)) {
            CBOR_CHECK(cbor_value_advance(&rootMapIt));
        }
    }

    CBOR_CHECK(cbor_value_leave_container(&rootIt, &rootMapIt));

    return ESP_OK;
}