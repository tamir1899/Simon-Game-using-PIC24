# Simon Game - PIC24 Status: Beta
- Core game functionality working (LED sequence, button input, audio feedback, difficulty scaling)
- High score storage via EEPROM in development

## Hardware
- PIC24FJ64GA002
- 4 LEDs (Red, Blue, Yellow, Green)
- 4 Push buttons
- Buzzer
- I2C LCD

## Known Issues
- High score resets on power-off (EEPROM write not yet functional)

## Schematic (coming soon)

## Next Steps
- Fix EEPROM write/read
- Move to stable folder when complete
- Design PCB in Altium
