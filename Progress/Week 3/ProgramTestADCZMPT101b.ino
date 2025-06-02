#include <avr/io.h>
#include <util/delay.h>

// Variabel global
volatile uint16_t vmax = 0;
volatile uint32_t startMillis = 0;
const uint16_t interval = 20; // dalam milidetik

// Inisialisasi ADC
void ADC_init() {
    ADMUX = (1 << REFS0); // Referensi AVcc, input ADC0
    ADCSRA = (1 << ADEN)  // Enable ADC
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128
}

// Membaca nilai ADC dari channel 0
uint16_t read_ADC() {
    ADCSRA |= (1 << ADSC); // Start conversion
    while (ADCSRA & (1 << ADSC)); // Tunggu hingga selesai
    return ADC;
}

// Inisialisasi UART
void UART_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << TXEN0);  // Enable transmitter
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data
}

// Mengirim satu karakter via UART
void UART_transmit(char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

// Mengirim string via UART
void UART_print(const char* str) {
    while (*str) {
        UART_transmit(*str++);
    }
}

// Mengubah float menjadi string dan kirim
void UART_print_float(float val) {
    char buffer[20];
    int int_part = (int)val;
    int decimal = (int)((val - int_part) * 100); // 2 angka desimal
    if (decimal < 0) decimal *= -1;
    snprintf(buffer, sizeof(buffer), "%d.%02d", int_part, decimal);
    UART_print(buffer);
}

// Fungsi millis (asumsi timer0 overflows setiap 1ms dengan prescaler 64)
volatile uint32_t millis_counter = 0;
ISR(TIMER0_OVF_vect) {
    millis_counter++;
}

void Timer0_init() {
    TCCR0A = 0x00;
    TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler 64
    TIMSK0 = (1 << TOIE0); // Enable overflow interrupt
    TCNT0 = 0;
    sei();
}

uint32_t millis() {
    return millis_counter;
}

int main(void) {
    ADC_init();
    UART_init(103);  // 9600 bps pada 16MHz
    Timer0_init();
    startMillis = millis();

    while (1) {
        uint16_t val = read_ADC();
        if (val > vmax) {
            vmax = val;
        }

        if (millis() - startMillis >= interval) {
            float vrms = (0.9743012 * vmax) - 499.81;
            UART_print("vrms = ");
            UART_print_float(vrms);
            UART_print("\r\n");

            vmax = 0;
            startMillis = millis();
        }
    }
}
