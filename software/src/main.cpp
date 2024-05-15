#include <Arduino.h>
#include <MIDI.h>


const int PIN_POT = 36;
const int PIN_UP_BTN = 5;  // has internal pullup resistor
const int PIN_DN_BTN = 14;  // has internal pullup resistor

bool up_state = HIGH;
bool up_prev_state = HIGH;
bool dn_state = HIGH;
bool dn_prev_state = HIGH;

int pot_raw_value = 0;
int pot_prev_value = 0;
int pot_deadband = 4096 / 128;  // 32 ADC counts, for 12-bit ESP32 input

bool cc_value_latch;
int debounce_period = 20;  // ms

unsigned long t;
unsigned long t_up;
unsigned long t_dn;

byte midi_channel = 16;

byte cc_active = 20;
byte cc_values[127] = {64};


void read_pot();
void update_cc_value(byte);
void read_button();
void change_cc(int);
void show_cc();


// MIDI_CREATE_DEFAULT_INSTANCE();  // Arduino, etc.

// MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);  // ESP32 (NodeMCU-32S) serial 2 UART

struct CustomBaudRateSettings : public MIDI_NAMESPACE::DefaultSerialSettings {  // ESP32 (NodeMCU-32S) USB serial, for use with Hairless MIDI
  static const long BaudRate = 115200;
};
MIDI_NAMESPACE::SerialMIDI<HardwareSerial, CustomBaudRateSettings> serialMIDI(Serial);
MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial, CustomBaudRateSettings>> MIDI((MIDI_NAMESPACE::SerialMIDI<HardwareSerial, CustomBaudRateSettings>&)serialMIDI);


void setup() {
    pinMode(PIN_UP_BTN, INPUT_PULLUP);
    pinMode(PIN_DN_BTN, INPUT_PULLUP);
    // Serial.begin(9600);  // for serial debugging
    MIDI.begin(midi_channel);
    t, t_up, t_dn = millis();
}


void loop() {
    t = millis();
    read_pot();
    read_button();
    show_cc();
}


void read_pot() {
    int useless = analogRead(PIN_POT);
    pot_raw_value = analogRead(PIN_POT);
    if (abs(pot_prev_value - pot_raw_value) > pot_deadband) {  // prevent ADC jitter and noise
        pot_prev_value = pot_raw_value;
        byte pot_new_value = (byte)(pot_raw_value >> 5);  // shift 12-bit to 7 bit resolution
        update_cc_value(pot_new_value);
    }
}


void update_cc_value(byte val) {
    if (cc_values[cc_active] == val) cc_value_latch = false;  // don't change CC value after changing CC number until fader matches CC value (prevents large jumps)
    if (!cc_value_latch) {
        cc_values[cc_active] = val;
        MIDI.sendControlChange(cc_active, cc_values[cc_active], midi_channel);
        // Serial.print("CC: "); Serial.print(cc_active);
        // Serial.print(", Val: "); Serial.print(cc_values[cc_active]);
        // Serial.print(", Channel: "); Serial.println(midi_channel);
    }
}


void read_button() {
    up_state = !digitalRead(PIN_UP_BTN);
    dn_state = !digitalRead(PIN_DN_BTN);

    if (up_state && dn_state) return;  // do nothing if both buttons are pushed together

    if ((up_state != up_prev_state) && ((t - t_up) > debounce_period)) {
        t_up = t;
        up_prev_state = up_state;
        if (up_state) {
            change_cc(1);
            return;
        }
    }

    if ((dn_state != dn_prev_state) && ((t - t_dn) > debounce_period)) {
        t_dn = t;
        dn_prev_state = dn_state;
        if (dn_state) {
            change_cc(-1);
            return;
        }
    }
}


void change_cc(int inc) {
    int cc = cc_active;
    cc += inc;
    if (cc < 0) cc = 0;
    if (cc > 127) cc = 127;
    cc_active = (byte)cc;
    cc_value_latch = true;  // don't change value of new CC after changing until fader matches value
    // Serial.print("new cc: "); Serial.println(cc);
}


void show_cc() {  // for future use, with 7-segment display or I2C LCD
    switch (cc_active) {
        default:
            break;
    }
}
