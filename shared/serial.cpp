#include "serial.h"

// clang-format off
uint8_t si_serial_msg_lengths[] = { 
    0,
    16, // Quaternion
    1,  // Samplerate
    1,  // alive
    1,  // enable
    1,  // gy connected
    1,  // gy found
    3,  // gy software version
    5,  // "Hello" message
    0
};
// clang-format on

char si_serial_read(si_serial_t* serial)
{
    return serial->uart->read();
}

int si_serial_available(si_serial_t* serial)
{
    return serial->uart->available();
}

void si_serial_init(HardwareSerial* uart,
                    si_serial_t* parser,
                    void* userdata,
                    si_serial_handlers_t h)
{
    parser->get      = h.geth;
    parser->set      = h.seth;
    parser->notify   = h.notifyh;
    parser->userdata = userdata;

    parser->uart = uart;
    parser->uart->begin(9600);
    si_serial_reset(parser);
}

void si_serial_reset(si_serial_t* parser)
{
    parser->state  = SI_PARSER_FIND_SYNC;
    parser->scount = 0;
}

void si_serial_read_next_byte(si_serial_t* serial, char dat)
{
    // clang-format off
    switch (serial->state) {
        case SI_PARSER_FIND_SYNC: si_serial_find_sync(serial, dat); break;
        case SI_PARSER_SYNCING: si_serial_sync(serial, dat); break;
        case SI_PARSER_READ_VALUE_TYPE: si_serial_read_valtype(serial, dat); break;
        case SI_PARSER_MESSAGE_TYPE: si_serial_read_msg_ty(serial, dat);
        case SI_PARSER_READ_VALUE: si_serial_read_value(serial, dat);
    }
    // clang-format on
}

void si_serial_find_sync(si_serial_t* serial, char dat)
{
    if (dat == SI_SERIAL_SYNC_CODE) {
        serial->state  = SI_PARSER_SYNCING;
        serial->scount = 1;
    }
}

void si_serial_sync(si_serial_t* serial, char dat)
{
    if (serial->scount < 3) {

        if (dat == SI_SERIAL_SYNC_CODE)
            serial->scount = serial->scount + 1;
        else
            si_serial_reset(serial);
    }
    else if (serial->scount == 3) {

        serial->state  = SI_PARSER_READ_VALUE_TYPE;
        serial->scount = 0;
    }
    else
        si_serial_reset(serial);
}

void si_serial_read_valtype(si_serial_t* serial, char dat)
{
    if (dat > SI_GY_VALUES_MIN && dat < SI_GY_VALUES_MAX) {
        serial->cval  = (si_gy_values_t) dat;
        serial->state = SI_PARSER_MESSAGE_TYPE;
    }
    else
        si_serial_reset(serial);
}

void si_serial_read_msg_ty(si_serial_t* serial, char dat)
{
    if (dat > SI_GY_MSG_TY_MIN && dat < SI_GY_MSG_TY_MAX) {
        serial->msg_ty = (si_gy_message_types_t) dat;
        serial->state  = SI_PARSER_READ_VALUE;

        digitalWrite(LED_BUILTIN, HIGH);
    }
    else
        si_serial_reset(serial);
}


void si_serial_call_handler(si_serial_t* serial)
{
    switch (serial->msg_ty) {
        case SI_GY_NOTIFY:
            return serial->notify(serial->userdata, serial->cval);
        case SI_GY_SET:
            return serial->set(serial->userdata, serial->cval, serial->buffer);
        case SI_GY_GET:
            si_serial_write_message(
                serial,
                SI_GY_SET,
                serial->cval,
                serial->get(serial->userdata, serial->cval, serial->buffer));
            break;
        default: break;
    }
}

void si_serial_read_value(si_serial_t* serial, char dat)
{
    if (serial->scount < si_serial_msg_lengths[serial->cval - 1] - 1)
        serial->buffer[serial->scount++] = dat;
    else {
        serial->buffer[serial->scount] = dat;
        si_serial_call_handler(serial);
        si_serial_reset(serial);
    }
}

void si_serial_write_message(si_serial_t* serial,
                             si_gy_message_types_t ty,
                             si_gy_values_t val,
                             const void* data)
{
    size_t len = si_serial_msg_lengths[val];

    for (int i = 0; i < 4; ++i) Serial.write(SI_SERIAL_SYNC_CODE);

    Serial.write(val);
    Serial.write(ty);
    Serial.write((const char*) data, len);
}

void si_serial_set_value(si_serial_t* serial,
                         si_gy_values_t value,
                         const void* data)
{
    si_serial_write_message(serial, SI_GY_SET, value, data);
}

void si_serial_req_value(si_serial_t* serial, si_gy_values_t val)
{
    si_serial_write_message(serial, SI_GY_GET, val, serial->buffer);
}

void si_serial_notify(si_serial_t* serial, si_gy_values_t val)

{
    si_serial_write_message(serial, SI_GY_NOTIFY, val, serial->buffer);
}

void si_serial_run(si_serial_t* serial)
{
    if (int count = si_serial_available(serial)) {
        for (int i = 0; i < count; ++i) {
            char v = si_serial_read(serial);
            if (v != -1)
                si_serial_read_next_byte(serial, v);
            else
                break;
        }
    }
}