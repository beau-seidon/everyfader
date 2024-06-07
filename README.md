# EveryFader
MIDI control change fader with selectable CC number

<p align="center">
    <img src="other resources/images/20240606_223120.jpg" height="512" title="EveryFader">
</p>

- Buttons select MIDI CC number, fader adjusts value for selected CC.

- LEDs show the selected CC number in 7-bit binary representation, from 0 to 127. (0b0000000 to 0b1111111).

    - E.G. default CC (20) is 0b0010100, so green LEDs 3 and 5 will be lit at startup.

- Pressing both buttons simultaneously toggles between the two LED modes. The second mode shows the VALUE of the selected CC, rather than the CC number, with a "db meter"-like behavior.

- The CC values are stored after adjustment, and moving the fader after switching CC numbers will not transmit new values until the fader position matches the selected CC's stored value. This will prevent jumps in CC value after making adjustments on a different CC.


