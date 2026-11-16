#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

// --- MAPEAMENTO DE PINOS CONFIRMADO ---
#define PIN_FRENTE_ESQ  1  // PC1 (A1)
#define PIN_TRAS_ESQ    0  // PC0 (A0)
#define PIN_FRENTE_DIR  2  // PC2 (A2)
#define PIN_TRAS_DIR    3  // PC3 (A3)

#define VEL_LENTA       90
#define VEL_ATAQUE      150
#define VEL_FUGA        150
#define DIST_DETECAO    20
#define LIMITE_PRETO    450  // Acima disto é a borda (Preta)

uint32_t milis_total = 0;

// --- MOVIMENTOS ---
void parar() { OCR0A = 0; OCR0B = 0; OCR2B = 0; OCR1B = 0; }
void andar_frente(uint8_t vel) { OCR0A = vel; OCR0B = 0; OCR2B = vel; OCR1B = 0; }
void andar_tras(uint8_t vel) { OCR0A = 0; OCR0B = vel; OCR2B = 0; OCR1B = vel; }
void rodar_esquerda(uint8_t vel) { OCR0A = 0; OCR0B = vel; OCR2B = vel; OCR1B = 0; }

void gerir_led(uint16_t incremento_ms) {
	milis_total += incremento_ms;
	if ((milis_total % 1000) < 500) PORTB |= (1 << PB1);
	else PORTB &= ~(1 << PB1);
}

uint16_t ler_linha(uint8_t canal) {
	ADMUX = (1 << REFS0) | (canal & 0x07);
	_delay_us(50);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

uint16_t get_distancia() {
	PORTB |= (1 << PB0);
	_delay_us(12);
	PORTB &= ~(1 << PB0);
	uint32_t timeout = 35000;
	while (!(PIND & (1 << PD2)) && timeout--) ;
	if (timeout == 0) return 999;
	uint32_t count = 0;
	while ((PIND & (1 << PD2)) && count < 30000) {
		_delay_us(1);
		count++;
	}
	return count / 58;
}

int main(void) {
	DDRD |= (1 << PD5) | (1 << PD6) | (1 << PD3);
	DDRB |= (1 << PB2) | (1 << PB1) | (1 << PB0);
	DDRD &= ~(1 << PD2);

	TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM00) | (1 << WGM01);
	TCCR0B = (1 << CS01);
	TCCR2A = (1 << COM2B1) | (1 << WGM20) | (1 << WGM21);
	TCCR2B = (1 << CS21);
	TCCR1A = (1 << COM1B1) | (1 << WGM10);
	TCCR1B = (1 << WGM12) | (1 << CS11);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);

	parar();
	_delay_ms(1000);

	while (1) {
		// --- 1. SENSORES DA FRENTE (Borda Preta detetada) ---
		if (ler_linha(PIN_FRENTE_ESQ) > LIMITE_PRETO || ler_linha(PIN_FRENTE_DIR) > LIMITE_PRETO) {
			parar(); _delay_ms(50);
			andar_tras(VEL_FUGA);
			for(int i=0; i<15; i++) { _delay_ms(20); gerir_led(20); }
			rodar_esquerda(VEL_FUGA);
			for(int i=0; i<42; i++) { _delay_ms(20); gerir_led(20); }
			continue;
		}

		// --- 2. SENSORES DE TRÁS (Borda Preta detetada) ---
		if (ler_linha(PIN_TRAS_ESQ) > LIMITE_PRETO || ler_linha(PIN_TRAS_DIR) > LIMITE_PRETO) {
			andar_frente(VEL_FUGA);
			for(int i=0; i<30; i++) { _delay_ms(20); gerir_led(20); }
			continue;
		}

		// --- 3. COMBATE ---
		uint16_t dist = get_distancia();

		if (dist > 0 && dist < DIST_DETECAO) {
			andar_frente(VEL_ATAQUE);
			for(int i = 0; i < 5; i++) {
				_delay_ms(20);
				gerir_led(20);
				// Proteção: verifica se atingiu a borda preta durante o ataque
				if (ler_linha(PIN_FRENTE_ESQ) > LIMITE_PRETO || ler_linha(PIN_FRENTE_DIR) > LIMITE_PRETO) break;
			}
			} else {
			andar_frente(VEL_LENTA);
			_delay_ms(50);
			gerir_led(50);
		}
	}
}