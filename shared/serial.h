#pragma once

#include <Arduino.h>

// gyro flags
#define SI_FLAG_ST_GY_CONNECTED _BV(0)
#define SI_FLAG_ST_GY_READY     _BV(1)
// #define SI_FLAG_ST_RESET_ORIENTATION _BV(2)
#define SI_FLAG_ST_INVERT_X _BV(3)
#define SI_FLAG_ST_INVERT_Y _BV(4)
#define SI_FLAG_ST_INVERT_Z _BV(5)

#define SI_FLAG_INVERT_BITMASK                                                 \
    (SI_FLAG_ST_INVERT_X | SI_FLAG_ST_INVERT_Y | SI_FLAG_ST_INVERT_Z)

// device flags
#define SI_FLAG_SEND_DATA         _BV(0)
#define SI_FLAG_RESET_ORIENTATION _BV(1)
#define SI_FLAG_APPLY_OFFSETS     _BV(2)

#define SI_SERIAL_SYNC_CODE 0x23
#define SI_SERIAL_BAUD_RATE 115200

extern uint8_t si_serial_msg_lengths[];

typedef enum si_gy_parser_state {
    SI_PARSER_FIND_SYNC = 1,
    SI_PARSER_SYNCING,
    SI_PARSER_READ_VALUE_TYPE,
    SI_PARSER_MESSAGE_TYPE,
    SI_PARSER_READ_VALUE
} si_gy_parser_state_t;

typedef enum si_gy_values {
    SI_GY_VALUES_MIN = 0,
    SI_GY_QUATERNION,
    SI_GY_SRATE,
    SI_GY_ALIVE,
    SI_GY_ENABLE,
    SI_GY_CONNECTED,
    SI_GY_FOUND,
    SI_GY_VERSION,
    SI_GY_HELLO,
    SI_GY_RESET,
    SI_GY_INV,
    SI_GY_RESET_ORIENTATION,
    SI_GY_INT_COUNT,
    SI_GY_VALUES_MAX
} si_gy_values_t;

typedef enum si_gy_message_types {
    SI_GY_MSG_TY_MIN = 10,
    SI_GY_GET,
    SI_GY_SET,
    SI_GY_NOTIFY,
    SI_GY_RESP,
    SI_GY_ACK,
    SI_GY_MSG_TY_MAX
} si_gy_message_types_t;

typedef const uint8_t* (*si_serial_get_handler_t)(void*,
                                                  si_gy_values_t,
                                                  const uint8_t*);
typedef void (*si_serial_set_handler_t)(void*, si_gy_values_t, const uint8_t*);
typedef void (*si_serial_notify_handler_t)(void*, si_gy_values_t);

typedef struct si_serial_handlers {
    si_serial_get_handler_t geth;
    si_serial_set_handler_t seth;
    si_serial_notify_handler_t notifyh;
} si_serial_handlers_t;

typedef struct si_serial {

    uint8_t buffer[32];

    HardwareSerial* uart;

    si_serial_get_handler_t get;
    si_serial_set_handler_t set;
    si_serial_notify_handler_t notify;
    void* userdata;

    si_gy_message_types_t msg_ty;

    si_gy_parser_state_t state;
    si_gy_values_t cval;
    int scount;

} si_serial_t;

char si_serial_read(si_serial_t*);

int si_serial_available(si_serial_t*);

void si_serial_init(HardwareSerial*, si_serial_t*, void*, si_serial_handlers_t);

void si_serial_reset(si_serial_t* parser);

void si_serial_read_next_byte(si_serial_t* parser, char dat);

void si_serial_find_sync(si_serial_t* serial, char dat);

void si_serial_sync(si_serial_t* serial, char dat);

void si_serial_read_valtype(si_serial_t* serial, char dat);

void si_serial_read_msg_ty(si_serial_t* serial, char dat);

void si_serial_call_handler(si_serial_t* serial);

void si_serial_read_value(si_serial_t* serial, char dat);

void si_serial_write_message(si_serial_t* serial,
                             si_gy_message_types_t ty,
                             si_gy_values_t val,
                             const uint8_t* data);

void si_serial_set_value(si_serial_t* serial,
                         si_gy_values_t value,
                         const uint8_t* data);
void si_serial_req_value(si_serial_t* serial, si_gy_values_t val);
void si_serial_notify(si_serial_t* serial, si_gy_values_t value);

void si_serial_run(si_serial* parser);
