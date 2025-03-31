#pragma once

#ifndef HWT905SETTINGS_H
#define HWT905SETTINGS_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/queue.h>

#include <stdint.h>
#include <signal.h>




enum REGISTER_ADDRESS {
    SAVE = 0x00, // Сохранить
    CALSW = 0x01, // Калибровка
    RSW = 0x02, // Вернуть содержимое данных
    RATE = 0x03, // Скорость возврата данных
    BAUD = 0x04, // Скорость передачи данных
    AXOFFSET = 0x05, // Смещение ускорения по оси X
    AYOFFSET = 0x06, // Смещение ускорения по оси Y
    AZOFFSET = 0x07, // Смещение ускорения по оси Z
    GXOFFSET = 0x08, // Смещение угловой скорости по оси X
    GYOFFSET = 0x09, // Смещение угловой скорости по оси Y
    GZOFFSET = 0x0a, // Смещение угловой скорости по оси Z
    HXOFFSET = 0x0b, // Магнитное смещение по оси X
    HYOFFSET = 0x0c, // Магнитное смещение по оси Y
    HZOFFSET = 0x0d, // Магнитное смещение по оси Z
    MMYY = 0x30, // Месяц и год
    HHDD = 0x31, // Час день
    SSMM = 0x32, // Секунда миниута
    MS = 0x33, // Милисекунды
    AX = 0x34, // Ускорение по оси X
    AY = 0x35, // Ускорение по оси Y
    AZ = 0x36, // Ускорение по оси Z
    GX = 0x37, // Угловая скорость по оси X
    GY = 0x38, // Угловая скорость по оси Y
    GZ = 0x39, // Угловая скорость по оси Z
    HX = 0x3a, // Магнитная поле по оси X
    HY = 0x3b, // Магнитная поле по оси Y
    HZ = 0x3c, // Магнитная поле по оси Z
    Roll = 0x3d, // Угол наклона оси X
    Pitch = 0x3e, // Угол наклона оси Y
    Yam = 0x3f, // Угол наклона оси Z
    TEMP = 0x40, // Температура
    Q0 = 0x51, // Кватернион Q0
    Q1 = 0x52, // Кватернион Q1
    Q2 = 0x53, // Кватернион Q2
    Q3 = 0x54  // Кватернион Q3
};


enum REGISTERS {
    START = 0x55,
    TIME = 0x50,
    ACCELERATION = 0x51,
    ANGULAR_VELONCY = 0x52,
    ANGLE = 0x53,
    MAGNETIC = 0x54,
    QUATERION = 0x59
};

enum REQUEST_REGISTERS {
    REQUEST_PREFIX = 0xFF,
    SECOND_REGISTER = 0xAA,
    TIME_REQ = 0x01,
    ACCELERATION_REQ = 0x02,
    ANGULAR_VELONCY_REQ = 0x04,
    ANGLE_REQ = 0x08,
    MAGNETIC_REQ = 0x10
};


typedef struct 
{
    uint8_t YY;
    uint8_t MM;
    uint8_t DD;
    uint8_t hh;
    uint8_t mm;
    uint8_t ss;
    uint16_t ms;
    double acceleration[3];
    double angularVelocity[3];
    double temperature;
    float angle[3];
    uint16_t magneta[3];
    double quaterion[4];
    uint16_t version;
}hwt905_values;

#endif