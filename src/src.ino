/* * * * * * * * * * * * * * * * * * * * *
 * Anti theft alarm for bikes.
 * * * * * * * * * * * * * * * * * * * * *
 * Author: lebucur
 * Last modified: 2017 May
 * * * * * * * * * * * * * * * * * * * * */

/* TODO
 * bluetooth ?
 * pulldown 100k res for Pins.alarm
 * bluetooth
 * save armed state in EEPROM / flash
 * understand MFRC522 better
 * shrink size
 */

#define DEBUG 1 //use UART for message logging
#define off false
#define on true
#define BAUD_RATE 38400

#include <SPI.h>
#include <MFRC522.h>
#include "Debug.h" //free HW serial when not needed

struct {
    byte const alarm = 8;     //HIGH=ON LOW=OFF
    byte const rfid_ss = 10;  //SPI Slave Select
    byte const rfid_rst = 9;  //?
    byte const vibration = 4; //IN
} Pins;

enum {
    Armed,
    Disarmed,
    MsgCount
};

char * message[MsgCount] = {
    "Armed", 
    "Disarmed",
};

MFRC522 mfrc522(Pins.rfid_ss, Pins.rfid_rst); //RFID

bool armed = false; //TODO don't lose value on reset

void setup()
{
    pinMode(Pins.vibration, INPUT);
    pinMode(Pins.alarm, OUTPUT);
    digitalWrite(Pins.alarm, LOW);
    
    DBG_begin(BAUD_RATE);
    DBG_printsln("START");

    SPI.begin();
    mfrc522.PCD_Init();

    armed = false; //TODO read value from non-volatile memory
    DBG_println(is_armed());
    print_commands();
}

void loop()
{
    if (armed && check_movement()) {
        siren(100); //for testing only
        //alarm(on); //once deployed it should sound until deactivated
    }

    if (mfrc522.PICC_IsNewCardPresent()) {
        if (mfrc522.PICC_ReadCardSerial()) {
            mfrc522.PICC_DumpToSerial(&(mfrc522.uid)); //takes a while
            toggle_armed(); //this will also turn off the alarm if it was on already
        }
    }
}

void toggle_armed()
{
    if (armed) {
        armed = false;
        siren(100); delay(50); siren(100);
    } else {
        armed = true;
        siren(100);
    }
    DBG_println(is_armed());
}

char const * is_armed()
{
    if (armed) {
        return message[Armed];
    } else {
        return message[Disarmed];
    }
}

bool check_movement()
{
    return digitalRead(Pins.vibration);
}

void siren(unsigned timeout)
{
    digitalWrite(Pins.alarm, HIGH);
    delay(timeout);
    digitalWrite(Pins.alarm, LOW);
}

void alarm(bool state)
{
    digitalWrite(Pins.alarm, state);
}

void print_commands()
{
    //guard with DEBUG
    DBG_printsln("Commands:");
    DBG_printsln("  m: show free RAM");
}

//called at the end of each loop() execution if new bytes arrive in RX
//it takes a little while until it is executed again
void serialEvent()
{
#if DEBUG
    switch (Serial.read()) //one ASCII character command
    {
        case 'm':
            DBG_prints("Free RAM: ");
            DBG_print(get_free_ram());
            DBG_printsln(" bytes");
        break;

        default:
            print_commands();
        break;
    }
#endif
}
