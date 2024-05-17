#include <Arduino.h>
#include <USB-MIDI.h>
// #include <MIDI.h>


const int PIN_POT = A0;
const int PIN_UP_BTN = 20;
const int PIN_DN_BTN = 19;
const int LED_PINS[7] = { 10, 3, 5, 6, 9, 7, 8 };

int pot_raw_value = 0;
int pot_prev_value = 0;
#ifdef ESP32
int pot_deadband = 4096 / 128;  // 32 ADC counts, for 12-bit ESP32 input
#else
int pot_deadband = 1024 / 128;  // 8 ADC counts, for 10-bit ATmega input
#endif

bool up_state = HIGH;
bool up_prev_state = HIGH;
bool dn_state = HIGH;
bool dn_prev_state = HIGH;

bool cc_value_latch;
byte debounce_period = 30;  // ms

unsigned long t;
unsigned long t_up;
unsigned long t_dn;
unsigned long t_mode;

byte midi_channel = 1;

byte cc_active = 20;
byte cc_values[127];

enum MODE { CC, CC_VAL, ERROR };
MODE LED_mode = CC;
bool LED_mode_latch = false;
uint16_t LED_mode_debounce_period = 500;


void read_pot();
void update_cc_value(byte);
void read_buttons();
void change_LED_mode();
void change_cc(int);
void update_LEDs();


// MIDI_CREATE_DEFAULT_INSTANCE();  // Arduino TTL MIDI, etc.

// MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);  // ESP32 (NodeMCU-32S) UART2 TTL MIDI

// struct CustomBaudRateSettings : public MIDI_NAMESPACE::DefaultSerialSettings {  // ESP32 (NodeMCU-32S) USB-to-serial, for use with Hairless MIDI
//   static const long BaudRate = 115200;
// };
// MIDI_NAMESPACE::SerialMIDI<HardwareSerial, CustomBaudRateSettings> serialMIDI(Serial);
// MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial, CustomBaudRateSettings>> MIDI((MIDI_NAMESPACE::SerialMIDI<HardwareSerial, CustomBaudRateSettings>&)serialMIDI);

USBMIDI_CREATE_DEFAULT_INSTANCE();  // SparkFun Pro Micro native USB MIDI


void setup() {
/* set pin modes */
    pinMode(PIN_UP_BTN, INPUT_PULLUP);
    pinMode(PIN_DN_BTN, INPUT_PULLUP);
    for (int i = 0; i < 7; i++) {
        pinMode(LED_PINS[i], OUTPUT);
    }

/* initialize CC values */
    for (int i = 0; i < 127; i++) {
        cc_values[i] = 64;
    }

/* start midi listener */
    MIDI.begin(midi_channel);

/* initialize timers */
    t = t_up = t_dn = t_mode = millis();
}


void loop() {
    t = millis();
    read_pot();
    read_buttons();
    update_LEDs();
}


void read_pot() {
    analogRead(PIN_POT);  // charge the input
    pot_raw_value = analogRead(PIN_POT);
    if (abs(pot_prev_value - pot_raw_value) > pot_deadband) {  // prevent ADC jitter and noise
        pot_prev_value = pot_raw_value;
        #ifdef ESP32
        byte pot_new_value = (byte)(pot_raw_value >> 5);  // shift 12-bit to 7 bit resolution
        #else
        byte pot_new_value = (byte)(pot_raw_value >> 3);  // shift 10-bit to 7 bit resolution
        #endif
        update_cc_value(pot_new_value);
    }
}


void update_cc_value(byte val) {
    if (cc_values[cc_active] == val) cc_value_latch = false;  // don't change CC value after changing CC number until fader matches CC value (prevents large jumps)
    if (!cc_value_latch) {
        cc_values[cc_active] = val;
        MIDI.sendControlChange(cc_active, cc_values[cc_active], midi_channel);
    }
}


void read_buttons() {
    up_state = !digitalRead(PIN_UP_BTN);
    dn_state = !digitalRead(PIN_DN_BTN);

    if (up_state && dn_state) {  // debounce and update LED mode
        if (!LED_mode_latch) {
            LED_mode_latch = true;
            t_mode = millis();
            change_LED_mode();
            return;
        }
    } else {
        if ((t - t_mode) > LED_mode_debounce_period) LED_mode_latch = false;
    }

    if ((up_state != up_prev_state) && ((t - t_up) > debounce_period)) {  // debounce and increase CC
        t_up = t;
        up_prev_state = up_state;
        if (up_state) {
            change_cc(1);
            return;
        }
    }

    if ((dn_state != dn_prev_state) && ((t - t_dn) > debounce_period)) {  // debounce and decrease CC
        t_dn = t;
        dn_prev_state = dn_state;
        if (dn_state) {
            change_cc(-1);
            return;
        }
    }
}


void change_LED_mode() {
    switch (LED_mode) {
        case CC:
            LED_mode = CC_VAL;
            break;
        case CC_VAL:
            LED_mode = CC;
            break;
        default:  // error
            LED_mode = CC;
            break;
    }
}


void change_cc(int inc) {
    int cc = cc_active;  // dummy value for clean inc/dec
    cc += inc;
    if (cc < 0) cc = 127;  // wrap around
    if (cc > 127) cc = 0;
    cc_active = (byte)cc;
    cc_value_latch = true;  // don't change value of new CC after changing, until fader matches value
}


void update_LEDs() {
    switch (LED_mode) {
        case CC:  // show 7-bit binary representation of MIDI Control number
            for (int i = 0; i < 7; i++) {
                digitalWrite(LED_PINS[i], (cc_active & (1 << i)));
            }
            break;

        case CC_VAL:  // show "dB meter" representation of MIDI Control value
            for (int i = 0; i < 7; i++) {
                if(cc_values[cc_active] > (i * 18)) {  // (128 / 7) = 18
                    digitalWrite(LED_PINS[i], HIGH);
                } else {
                    digitalWrite(LED_PINS[i], LOW);
                }
            }
            Serial.println();
            break;

        default:  // error
            break;
    }

}
