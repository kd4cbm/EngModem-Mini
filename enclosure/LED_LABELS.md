# Front panel LED labels - CNC engraving plan (draft)

> **Note on what the labels mean electrically (added after bring-up):** every LED lights when its drive
> signal is HIGH, and five of them (MR, TR, SD, RD, CD) go through a non-inverting buffer from active-low
> signals, so on Rev5 as built they read inverted (for example CD is lit when there is *no* carrier). The
> labels below name the *function*, which is unaffected, but see
> [`../docs/ERRATA.md`](../docs/ERRATA.md#e11-most-front-panel-leds-read-inverted) before deciding what
> lit/dark should mean on your panel. The left-to-right order was confirmed on the built unit.

**Preliminary - not yet cut.** Positions are derived directly from the
front panel drill guide's own geometry (`build_jig1_vfd_drill.py`), not
re-estimated separately. Text height/V-bit choice are sizing/tooling
recommendations, not yet verified by a physical test cut.

## The 8 labels

The 3mm THT LEDs sit at a fixed 15.625mm pitch (125mm VFD width / 8),
evenly spaced under the VFD window. Only 3 of the 8 LEDs had a documented
function before this (`LED-AA`, `LED-HS`, `LED-OH` net labels in the
schematic) - the other 5 route through U4 (a 74HCT245 buffer) from real
RS-232/TTL signals with no descriptive net name of their own. Traced via
`pcbnew` (each LED's anode -> series resistor -> either a named net or a
U4 output pin -> matched to U4's real input pin on the same physical
channel) rather than assumed from the generic historical Hayes-modem LED
set - it happens to match that set exactly, which is a good independent
confirmation the trace is right, not the reason it was chosen.

| Position (L->R) | Label | Real signal | Driven via |
|---|---|---|---|
| 1 | **MR** | DSR (Modem Ready) | U4 (74HCT245) |
| 2 | **TR** | DTR (Terminal Ready) | U4 (74HCT245) |
| 3 | **SD** | TXD (Send Data) | U4 (74HCT245) |
| 4 | **RD** | RXD (Receive Data) | U4 (74HCT245) |
| 5 | **OH** | Off-Hook | direct GPIO (GPIO12) |
| 6 | **CD** | DCD (Carrier Detect) | U4 (74HCT245) |
| 7 | **AA** | Auto-Answer | direct GPIO (GPIO10) |
| 8 | **HS** | High-Speed | direct GPIO (GPIO11) |

## Text sizing

LED pitch is 15.625mm center-to-center. Reserving a 3mm gutter between
adjacent labels leaves ~12.6mm usable width per 2-character label.
For a monospace single-stroke engraving font, 2-character width is
roughly 1.5x the text height (0.6-0.7x height per character + inter-
character gap) - so ~8.4mm is the geometric ceiling at full width.

**Recommended: 5-6mm text height** - well inside that ceiling, leaves
real margin between labels, reads as a properly proportioned indicator
label rather than one straining to fill its slot. Verify the actual
character width for your specific .cxf font in F-Engrave's own live
preview before cutting - exact metrics vary by font file.

## Per-label placement (F-Engrave)

Coordinates below use the same design convention as the drill-guide jig
scripts: **X=0 at the panel's left edge, Y=0 at the panel's TOP edge,
increasing downward.** Most CNC controllers zero at the bottom-left with
Y increasing upward instead - flip with `Y_machine = 66 - Y_design` if
that's your setup, or you'll get the label row mirrored top-to-bottom.

In F-Engrave, set the **Justify/origin anchor** to a **center column**
(Top-Center, Middle-Center, or Bottom-Center) before entering X - by
default the anchor is left-aligned, so the X value means "left edge of
text" instead of "center," which is the main way this goes wrong.

| Label | X-Origin (mm) | Y-Origin, design conv. (mm) | Y-Origin, machine conv. (mm) |
|---|---|---|---|
| MR | 40.8125 | 56.75 | 9.25 |
| TR | 56.4375 | 56.75 | 9.25 |
| SD | 72.0625 | 56.75 | 9.25 |
| RD | 87.6875 | 56.75 | 9.25 |
| OH | 103.3125 | 56.75 | 9.25 |
| CD | 118.9375 | 56.75 | 9.25 |
| AA | 134.5625 | 56.75 | 9.25 |
| HS | 150.1875 | 56.75 | 9.25 |

(Y-Origin above assumes a center-anchor and 6mm text height, labels sitting
below the LED row with a 1mm gap off the hole edge - recompute if you pick
a different text height or anchor point.)

F-Engrave generates one G-code file per text block - run this once per
label (8 times total), then queue/concatenate the resulting files. Use
the same CNC work-zero (registered to the same panel reference the
drill/router jigs use) for all 8 so these coordinates line up physically.

See `renders/led_label_layout.png` for a visual placement check of all
8 labels under their LEDs before cutting.

## V-bit

Panel is anodized aluminum, not wood/acrylic - filter for bits explicitly
rated for metal. **Recommended: 20-30 degree carbide V-bit, 0.1-0.2mm tip,
shank to match your CNC's collet (1/8"/3.175mm or 1/4"):**

- EnPoint 20 deg, 0.2mm tip, 1/8" shank - aluminum-rated, pack of 5. One
  real reported data point: successfully engraved anodized aluminum at
  16,000 RPM / 25 in/min / 0.003" depth of cut per pass.
- EnPoint 30 deg, 0.1mm tip, 1/8" shank - finer tip, more fragile in metal;
  use the 20 deg/0.2mm option if this is a first aluminum-engraving run.
- SpeTool 20 deg, ~0.13mm tip, 1/4" shank, TAC-coated - more durable for
  repeated small-batch jobs if your collet takes 1/4" shanks.

A narrower angle (20-30 deg vs the common 60-90 deg signmaking bits) is
the right call at this text height - it reaches the fine stroke widths
2-letter labels at 5-6mm need without requiring near-impossible depth
precision. Enter the *exact* angle and tip diameter of whichever bit you
actually buy into F-Engrave's tool settings - it uses those two numbers
directly to compute plunge depth for the target line width.
