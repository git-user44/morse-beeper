# Morse code beeper for the Raspberry Pi

This is a simple morse code beeper for the Raspberry Pi. It works on the Raspberry Pi 3 Model B+. For other models you may well have to change the name of the GPIO chip controller which is currently hardwired to be 'pinctrl-bcm2835'. It works with a passive beeper/buzzer so you can easily change the tone of the buzzer.

I have found that some buzzers sound louder at certain frequencies.

There seem to be several programs out there that do similar thing but appear to use the WiringPi library which is now obsolete, unsupported, and substantially replace by the kernel library libgpio.

The program was developed to run on a headless mobile robot, to enable various status messages to be beeped out.

The program takes text from standard input and beeps it out, e.g.

`
# echo -n "low battery" | morse
`

If you want spoken word messages, you may want to ask Google about "festival speech".


## Enjoy.
