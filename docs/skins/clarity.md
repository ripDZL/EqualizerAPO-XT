# Clarity High Contrast

Clarity is the accessibility-first work theme. It is deliberately calm and
plain: information gets contrast, spacing, and a stable shape before it gets
decoration.

## Contract

- Primary labels and values exceed 15:1 contrast on card surfaces.
- Secondary labels exceed 9:1 contrast on card surfaces.
- Dark and light modes are materially different, not a brightness toggle.
- Keyboard focus, selected choices, hover, and press each use a visibly
  different shape or fill; colour is never the only clue.
- Knobs show a thick travel arc, a contrasting pointer, a zero detent for
  bipolar values, and a strong numeric readout whenever one is available.
- Modern cards and Legacy Rows use the same token table and high-visibility
  knob/selector painters.
- Legacy Preamp uses the shared bipolar `AudioKnob`; it must never fall back
  to the platform `QDial`.

## Form language

The theme reuses Precision Minimal's orderly layout grammar, but it does not
reuse its hairline-only controls. Clarity controls have bold outlines, generous
row height, distinct selected cells, and no decorative texture. Blue is the
single action/focus colour; text and numeric values remain neutral black or
white.

## Do not

- Reduce contrast to preserve a colour mood.
- Encode an interactive state only with a colour change.
- Replace the thick knob arc or pointer with a purely decorative dial.
- Make values smaller than surrounding labels to save space.
- Let the Legacy Clarity gallery become optional; it is the regression gate
  for the active and disabled dark/light rows.
