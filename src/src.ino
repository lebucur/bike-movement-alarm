/* * * * * * * * * * * * * * * * * * * * *
 * Anti theft alarm for bikes.
 * * * * * * * * * * * * * * * * * * * * *
 * Author: lebucur
 * Last modified: 2017 May
 * * * * * * * * * * * * * * * * * * * * */

/* TODO
 * don't lose armed state on reset (use EEPROM)
 */

#define DEBUG 1 //use UART for message logging
#define off false
#define on true
#define BAUD_RATE 38400
#define UID_SIZE 4 //there are also some tags with 7 bytes

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

MFRC522 rfid(Pins.rfid_ss, Pins.rfid_rst); //RFID

bool armed = false;
byte uid[4] = {0x0E, 0x75, 0xFA, 0x71};

void setup()
{
    pinMode(Pins.vibration, INPUT);
    pinMode(Pins.alarm, OUTPUT);
    
    alarm(off);
    
    DBG_begin(BAUD_RATE);
    DBG_printsln("START");

    SPI.begin();
    rfid.PCD_Init();

    armed = false; //TODO read value from non-volatile memory
    DBG_println(is_armed());
    print_commands();
}

void loop()
{
    if (armed && check_movement()) {
        //siren(100); //for testing only
        alarm(on); //once deployed it should sound until deactivated
    }

    if (rfid.PICC_IsNewCardPresent()) {
        if (rfid.PICC_ReadCardSerial()) {
            //rfid.PICC_DumpToSerial(&(rfid.uid)); //takes a while
            boolean match = true;
            for (int i = 0; i < UID_SIZE; i++) {
                //DBG_printf(rfid.uid.uidByte[i], HEX);
                if (uid[i] != rfid.uid.uidByte[i]) {
                    match = false;
                }
            }
            if (match) {
                toggle_armed(); //this will also turn off the alarm if it was on already
                delay(2000);
            }
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
    DBG_printsln("Commands:");
    DBG_printsln("  a: toggle armed state");
    DBG_printsln("  s: siren on for 1s");    
    DBG_printsln("  m: show free RAM");
}

//called at the end of each loop() execution if new bytes arrive in RX
//it takes a little while until it is executed again
void serialEvent()
{
#if DEBUG
    switch (Serial.read()) //one ASCII character command
    {
        case 'a':
            toggle_armed();
        break;

        case 'm':
            DBG_prints("Free RAM: ");
            DBG_print(get_free_ram());
            DBG_printsln(" bytes");
        break;

        case 's':
            siren(1000);
        break;

        default:
            print_commands();
        break;
    }
#endif
}
