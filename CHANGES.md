# Changes

Hardware-side changelog for EngModem Mini. For firmware changes, see the
[Zimodem-VFD-Mini](https://github.com/kd4cbm/Zimodem-VFD-Mini) repo's own
`CHANGES.md`.

## Manufacturing outputs: U3/U5 regulator substitution

JLCPCB's PCBA placement preview surfaced a real footprint mismatch on
both of the board's D2PAK/TO-263-3 linear regulators - not a rotation or
rendering quirk, an actual lead-count/package mismatch:

- **U3** (5V rail): originally HGSEMI LM7805S2/TR (LCSC C2902505). LCSC's
  own listing shows this part as a 2-lead package, and JLCPCB's 3D model
  shows "TO263-5" case marking - neither matches the 3-lead footprint
  this board uses.
- **U5** (3.3V rail): originally STMicroelectronics LD1086D2T33TR (LCSC
  C7915). ST's own datasheet defines two different D²PAK mechanical
  variants (2-lead and 3-lead) under one ambiguous order code that
  doesn't disambiguate which ships; LCSC/JLCPCB both list this exact part
  as 2-lead.

Both were excluded from the JLCPCB PCBA order (`BOM_PCBA_JLCPCB.csv`,
`CPL_SMD.csv`) rather than risk a bad placement. Verified, datasheet-
confirmed replacements to hand-solder instead (sourced from Mouser/
DigiKey, not JLCPCB/LCSC):

- **U3 -> onsemi MC7805CD2TR4G** (D2PAK-3, CASE 936). Confirmed via
  onsemi's own MC7800 datasheet (the same one already cited in this
  schematic): Pin 1=Input, Pin 2=Ground=Tab, Pin 3=Output - an exact
  match to this footprint and the schematic's pin assignment.
- **U5 -> TI LM1086CSX-3.3/NOPB** (DDPAK/TO-263, package KTT). Confirmed
  via TI's own datasheet: Pin 1=ADJ/GND, Pin 2+Tab=Output, Pin 3=Input -
  an exact match, including the less-common tab-to-Output wiring this
  board's design already uses (verified correct independently, via ST's
  LD1086 datasheet, before the substitution was even needed).

Full detail in `manufacturing/BOM_full.csv` and
[`PIN_MAP.md`](PIN_MAP.md#power-regulation---u3-u5).

## Manufacturing outputs: Gerbers, BOM, CPL

Added for Rev4:

- Gerbers + Excellon drill files (`manufacturing/Gerbers_EngModem-Mini_Rev4.zip`)
- `BOM_PCBA_JLCPCB.csv` - SMD parts JLCPCB will assemble. Common passives
  list full specs with no pinned LCSC number (their basic-parts catalog
  isn't reliably scriptable); the ICs, module, inductor, and microSD
  socket carry LCSC part numbers verified individually against LCSC's own
  product pages, not just search results.
- `BOM_full.csv` - every part on the board, including through-hole
  connectors/headers/LED positions, for hand-assembly.
- `CPL_SMD.csv` - placement file for the JLCPCB-assembled SMD parts,
  generated from the PCB's real component coordinates.

Flagged in the README: JLCPCB's assembly preview renders whichever
specific LCSC library part it matches to a BOM row, using that part's own
reference point for both rotation and position - which doesn't always
agree with KiCad's footprint origin. Check their preview before ordering.

## Board renders

Replaced the initial flat F.Cu/B.Cu + silkscreen SVG plots with isometric
3D renders (`kicad-cli pcb render`, high quality) for the README.

## Revision history

| Rev | Notes |
|---|---|
| 0-1 | Original modular layout and placement baseline |
| 2 | SD card socket swapped from a pin header to the real microSD socket (Hirose DM3AT-SF-PEJM5); full reroute |
| 3 | Second SD placement experiment; full reroute; 69 net-derived pin-function silkscreen labels added across every header/LED footprint |
| 4 | **Current.** Dedicated 0.5mm net class for `/DC-IN`; solid (non-thermal-relief) zone connections on regulator/shield GND tabs; local `/VCC` copper pour + thermal vias around U5; ESP32 power-filter components (L1/C11/C12) relocated for shorter real routed-copper paths to U2 |

## Verification practice

Every revision was checked with KiCad's own tools before being carried
forward: ERC, DRC, footprint/schematic sync. Rev4 additionally got a full
pin-by-pin cross-check against the firmware's actual GPIO usage (18/18
matched). None of this substitutes for bringing up a real board - see the
disclaimer in the README. No physical unit has been built or tested.
