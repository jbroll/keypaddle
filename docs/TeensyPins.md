# Teensy 2.0 Complete Pin Mapping Reference

## Arduino Pin to ATmega32U4 Port/Pin Mapping

| Arduino Pin | Port.Pin | Functions | Notes |
|-------------|----------|-----------|-------|
| **0** | **PB0** | SS | SPI Slave Select |
| **1** | **PB1** | SCLK | SPI Clock |
| **2** | **PB2** | MOSI | SPI Master Out |
| **3** | **PB3** | MISO | SPI Master In |
| **4** | **PB7** | PWM, RTS, OC1C, OC0A | PWM capable |
| **5** | **PD0** | SCL, PWM, INT0, OC0B | I2C Clock, PWM, Interrupt |
| **6** | **PD1** | SDA, INT1 | I2C Data, Interrupt |
| **7** | **PD2** | RX, INT2, RXD1 | UART Receive, Interrupt |
| **8** | **PD3** | TX, INT3, TXD1 | UART Transmit, Interrupt |
| **9** | **PC6** | PWM, OC3A, OC4A | PWM capable |
| **10** | **PC7** | PWM, OC4A, ICP3 | PWM capable |
| **11** | **PD6** | PWM, T1, OC4D, ADC9 | **LED on pin 11**, PWM |
| **12** | **PD7** | PWM, ADC10, T0, OC4D | PWM capable |
| **13** | **PB4** | PWM, ADC11 | PWM capable |
| **14** | **PB5** | PWM, ADC12, OC1A, OC4B | PWM capable |
| **15** | **PB6** | PWM, ADC13, OC1B, OC4B | PWM capable |
| **16** | **PF7** | A0, ADC7 | Analog input A0 |
| **17** | **PF6** | A1, ADC6 | Analog input A1 |
| **18** | **PF5** | A2, ADC5 | Analog input A2 |
| **19** | **PF4** | A3, ADC4 | Analog input A3 |
| **20** | **PF1** | A4, ADC1 | Analog input A4 |
| **21** | **PF0** | A5, ADC0 | Analog input A5 |
| **22** | **PD4** | A6, ADC8, ICP1 | Analog input A6 |
| **23** | **PD5** | A7, CTS, XCK1 | Analog input A7 |

## Port Summary

### PORTB (8 pins available)
- **PB0** → Pin 0 (SS)
- **PB1** → Pin 1 (SCLK) 
- **PB2** → Pin 2 (MOSI)
- **PB3** → Pin 3 (MISO)
- **PB4** → Pin 13 (PWM)
- **PB5** → Pin 14 (PWM)
- **PB6** → Pin 15 (PWM)
- **PB7** → Pin 4 (PWM)

### PORTC (2+ pins available)
- **PC6** → Pin 9 (PWM)
- **PC7** → Pin 10 (PWM)
- Additional interior pins may be available

### PORTD (8 pins available)
- **PD0** → Pin 5 (SCL, PWM, INT0)
- **PD1** → Pin 6 (SDA, INT1)
- **PD2** → Pin 7 (RX, INT2) - *Not USB D+*
- **PD3** → Pin 8 (TX, INT3) - *Not USB D-*
- **PD4** → Pin 22 (A6)
- **PD5** → Pin 23 (A7)
- **PD6** → Pin 11 (PWM, LED)
- **PD7** → Pin 12 (PWM)

### PORTE (1+ pins available)
- **PE6** → Interior pin (INT6, AIN0)
- Additional pins may be accessible

### PORTF (6 pins available)
- **PF0** → Pin 21 (A5, ADC0)
- **PF1** → Pin 20 (A4, ADC1)
- **PF4** → Pin 19 (A3, ADC4)
- **PF5** → Pin 18 (A2, ADC5)
- **PF6** → Pin 17 (A1, ADC6)
- **PF7** → Pin 16 (A0, ADC7)
- **Note:** PF2 and PF3 do not exist on ATmega32U4

## Switch Input Capabilities

### Total Available Digital Inputs
- **24 easily accessible pins** (Arduino pins 0-23)
- **All pins can be configured as digital inputs with internal pull-ups**
- **Additional interior pins may be available for expert users**

### Special Considerations
- **Pin 11 (PD6)**: Has onboard LED - still usable but LED will respond to pin state
- **USB pins**: D+ and D- are used internally but don't affect numbered pin availability
- **Analog pins**: A0-A7 (pins 16-23) can be used as digital inputs when ADC not needed
- **PWM pins**: All PWM-capable pins can be used as digital inputs

### Code Reference for Port Access
```c
// Direct port manipulation examples
DDRB &= ~(1 << PB0);    // Set PB0 (pin 0) as input
PORTB |= (1 << PB0);    // Enable pull-up on PB0

DDRC &= ~(1 << PC6);    // Set PC6 (pin 9) as input  
PORTC |= (1 << PC6);    // Enable pull-up on PC6

DDRD &= ~(1 << PD2);    // Set PD2 (pin 7) as input
PORTD |= (1 << PD2);    // Enable pull-up on PD2

DDRF &= ~(1 << PF7);    // Set PF7 (pin 16/A0) as input
PORTF |= (1 << PF7);    // Enable pull-up on PF7
```

## Key Insights
1. **Teensy 2.0 maximizes pin utilization** - nearly every ATmega32U4 I/O pin is accessible
2. **24+ switches are definitely supported** - contrary to some limitations suggested in documentation
3. **USB functionality doesn't reduce available pins** - excellent design decision by PJRC
4. **Mix of ports required** - optimal switch scanning would use multiple ports for efficiency
5. **Interior pins expand possibilities** - expert users may access additional pins