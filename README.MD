## Fri3d Badge 2024 Shitty Hat On
This is a PlatformIO project.

The (doc) folder contains the schematic and the attiny85 micro pinout
The blue colored pin numbers can be used in the source

## fuses
https://www.engbedded.com/fusecalc/
https://eleccelerator.com/fusecalc/fusecalc.php?chip=attiny85

**do not forget to program the fuses**  
if the leds are all white, and no pattern is displayed  
then the fuses are at their default value and this is missing in `main::setup`
```
    // Force clock prescaler to 1 (8MHz) even if CKDIV8 fuse is set
    CLKPR = (1 << CLKPCE);
    CLKPR = 0;
```

### verify the fuses

check them with 
```
~/.platformio/packages/tool-avrdude/avrdude -C ~/.platformio/packages/tool-avrdude/avrdude.conf -c stk500v1 -p attiny85 -P /dev/ttyUSB0 -b 19200 -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h 2>&1 | grep -E '^0x|safemode'
```

### default values
| Fuse | Value |
|-------|-------|
| lfuse | 0x62 |
| hfuse | 0xDF |
| efuse | 0xFF |


### required values
program the fuses with the following command
```
~/.platformio/penv/bin/platformio run --target fuses
```

| Fuse | Before | **Required** |
|-------|-------|-----|
| lfuse | 0x62 | **0xE2** |
| hfuse | 0xDF | **0xDF** |
| efuse | 0xFF | **0xFF** |
