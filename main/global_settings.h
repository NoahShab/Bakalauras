#ifndef __GLOBAL_SETTINGS_H__
#define __GLOBAL_SETTINGS_H__

#include "driver/uart.h"

#define TESTING false
#define PRINT_UART true

#define SLEEP_UZRAKINTAS_S 150
#define SLEEP_ATRAKINTAS_S 150
#define SLEEP_VOGIAMAS_S 180
#define ACTIVE_TIME 30

// Pinai GPIO ir ADC
#define INTERRUPT_PIN   GPIO_NUM_7
#define PIN_SDA         GPIO_NUM_6
#define PIN_SCL         GPIO_NUM_5
#define PWRKEY          GPIO_NUM_4
#define GNSS_ANT        GPIO_NUM_3
#define SCOOTER         GPIO_NUM_10
#define ADC_CH0         GPIO_NUM_0
#define ADC_CH1         GPIO_NUM_1

typedef enum {
    STATE_LOCKED,
    STATE_UNLOCKED,
    STATE_BEING_STOLEN
} system_state_t;

extern system_state_t current_state;

extern bool vogiamas;
extern bool uzrakintas;

#define PHONE_NUMBER "+37067940039"
#define DEFAULT_PASSWORD "slaptazodis" 
#define BLE_DEVICE_NAME "El. paspirtukas"
#define BLE_ADV_DURATION_MS 300000    


#define VogiamasSleepCycle "0101" //"0010" - 20.48s; "0101" 81.92 seconds; "0000" - 5.12s      pagal duomenu lapa

/*
000 10min
001 1h 
010 10h 
011 2sec 
100 30sec 
101 1min 
110 320h
kart:
kiek kartų 00000

30s = 10000001 
2min = 10100010
14s = 01100111

Low power application duomenų 10 lapas


*/


// UART
#define UART_PORT       UART_NUM_1
#define UART_BAUD       115200
#define UART_BUF_SIZE   2048
#define UART_TX         GPIO_NUM_21
#define UART_RX         GPIO_NUM_20

#endif