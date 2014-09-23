/*
 * LEDTree.c
 *
 * Created: 24.08.2014 20:43:08
 *  Author: Maksim Dragun
 */ 


#include <avr/io.h>
#define F_CPU 1000000L
#include <util/delay.h>

#define SPI_DDR  DDRB
#define SPI_PORT PORTB
#define SPI_SS   PB2
#define SPI_MOSI PB3
#define SPI_MISO PB4
#define SPI_SCK  PB5
#define ADC_DDR  DDRC
#define ADC_PORT PORTC
#define ADC_INP  PC0

const uint32_t ALONE = 0b1;
const uint32_t MAX = 0xFFFFFFFE;

enum mode { THIN_RUNNER, THICK_RUNNER } work_mode = THIN_RUNNER;
uint32_t data = 0;

void device_init()
{
	data = ALONE;
}

void adc_init() 
{
	ADC_DDR = 0x00;
	ADC_PORT = 0xFF;
	ADMUX = (1 << ADLAR);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (0 << ADPS1) | (1 << ADPS0);
}

void spi_init()
{
	//настраиваем выводы MOSI, SCL, SS на выход
	SPI_DDR = ( 1 << SPI_MOSI) | ( 1 << SPI_SCK) | ( 1 << SPI_SS );
	//выставляем SS в 1
	SPI_PORT |= ( 1 << SPI_SS );
	// разрешаем SPI, Master, режим 0, частота 1/4 от F_CPU, LSB first
	SPCR = ( 1 << SPE ) | ( 1 << MSTR ) | (1 << DORD);
	SPSR = ( 1 << SPI2X ); //удвоение частоты SPI
}

void spi_reset()
{
	SPI_PORT &= ~(1 << SPI_SS );  
}

void spi_confirm()
{
	SPI_PORT |= ( 1 << SPI_SS ); 
}

void spi_send_byte(unsigned char b)
{
	SPDR = b;                           //передаваемые данные
	while( !( SPSR & ( 1 << SPIF ) ) ); //ждем окончания передачи
}

// 0000 0000  0000 0000  0000 0000  0000 0000
void spi_send_int(uint32_t data) {
	spi_reset();
	unsigned char byte = (unsigned char) data;
	spi_send_byte(byte);
	byte = (unsigned char) (data >> 8);
	spi_send_byte(byte);
	byte = (unsigned char) (data >> 16);
	spi_send_byte(byte);
	byte = (unsigned char) (data >> 24);
	spi_send_byte(byte);
	spi_confirm();
}

unsigned long ROL (uint32_t a, int offset)
{
	return a << offset | a >> (32 - offset);
}

unsigned long ROR (uint32_t a, int offset)
{
	return a >> offset | a << (32 - offset);
}

void adc_start()
{
	ADCSRA |= (1 << ADSC);
}
	
uint8_t get_mode()
{
	adc_start();
	while(!(ADCSRA & (1 << ADIF)));
	return ADCH / 128;
}

void change_mode(enum mode new_mode)
{
	if (work_mode != new_mode)
	{
		work_mode = new_mode;
		switch(work_mode)
		{
			case THIN_RUNNER:
				data = ALONE;
				break;
			case THICK_RUNNER:
				data = MAX;
				break;
		}
	}
}

int main(void)
{
	spi_init();
	adc_init();
	device_init();

    while(1)
    {
		change_mode(get_mode());
		spi_send_int(data);
		_delay_ms(100);
		data = ROR(data, 1);
    }
}