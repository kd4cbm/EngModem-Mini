# Enclosure Panel Jigs

## ⚠️ Preliminary - not yet used on real hardware

**These jigs have not been printed, fitted, or test-drilled on a real
Hammond enclosure panel yet.** All positions are derived from real
sourced data (the actual Noritake VFD mechanical drawing, the real
Hammond 1455U2201BK STEP-derived geometry, a standard reference
drawing for the DB9 cutout), not guesses - but "derived from real
data" is not the same as "verified against physical hardware." In
particular, **the DB9 cutout profile involved one genuine interpretive
call** reconciling two sub-dimensions on the reference drawing that
didn't cleanly agree pixel-for-pixel (see below) - test-fit it against
the actual motherboard DB9 shell before committing to final material.
Treat everything here as a first-pass draft.

## Enclosure

**Hammond 1455U2201BK** - extruded aluminum, card-slot mount, black
finish. 220 x 191 x 66mm overall, max PC board size 220 x 186.48mm.
Real geometry (`1455U2201_REAL_end_panel.stl`, `1455U2201_REAL_bezel.stl`,
included here) was extracted directly from Hammond's own factory STEP
file, not estimated from the dimensioned PDF drawing alone.

## Tool spec

**1/8" (3.175mm) bit for both drilling and routing** - the same
physical bit does both jobs. The router bit has **no bearing**: the
non-cutting shaft rides the template edge directly, shaft diameter
equals cutting diameter, so template openings are cut to the *exact*
final dimensions with no offset compensation.

**Workflow, in order:**
1. Register the jig on the panel using the wrap-around registration
   pocket (sized to the panel's real outer edge, ~0.15mm clearance for
   a snug slip-fit).
2. Drill all guide holes in "jig 1" (main pass).
3. Swap to "jig 2" (same registration, offset holes) and drill those
   too - the two passes interleave to roughly half the effective hole
   spacing, so adjacent holes overlap and the perimeter should
   mostly punch free by hand with minimal filing.
4. Punch/break out the remaining thin webs between holes by hand.
5. Swap to the router template jig. With the panel still registered
   in the same pocket, run the router bit's shaft along the template's
   inner edge to smooth the already-open edge to the final line.
   **This is a light finishing pass on already-open material, not a
   through-cut** - the two-step drill-then-route sequence exists
   specifically so the router bit is never cutting through solid
   material while its shaft rides directly against the printed jig,
   which could otherwise generate enough friction heat to soften the
   jig itself.

## Print settings

Geometry settings below apply regardless of material. Material-specific
temperature/cooling/retraction profiles for PrusaSlicer follow after.

- **40-50% infill**, gyroid or cubic pattern.
- **4+ perimeter walls, 3+ solid top/bottom layers** - more important
  than infill % here. The thin webs between adjacent drill-guide holes
  (as tight as ~1.5-2mm at this spacing) are almost entirely perimeter
  shell, not infill lattice, and the top plate needs to stay flat and
  accurate under repeated drill pressure.
- **Material**: PLA (or PLA Pro+/PLA+) is fine for the drill guides -
  no heat exposure there. **Use PETG for the router template
  specifically** - cheap insurance given the tool-shaft friction-heat
  concern above; PETG's heat-deflection temp (~70-80°C) is meaningfully
  higher than standard PLA's (~50-55°C) or PLA Pro+/PLA+'s (~55-60°C).
  A single PLA Pro+ spool for all six jigs is a reasonable simplification
  if you'd rather not swap filament, given its HDT edge over plain PLA -
  PETG is still the safer choice for the router template alone.

### Dimensional accuracy - read this before printing either material

The offset drill guides have very little built-in clearance between the
guide hole and the actual bit/hardware (down to ~0.05mm radial on some
holes), and FDM printers reliably print round holes **undersized** -
typically 0.1-0.3mm on a 0.4mm nozzle, from melt-flow rounding at the
perimeter/infill transition. Left uncorrected, the bit may not seat in
the guide holes at all.

- **PrusaSlicer -> Print Settings -> Advanced -> Slicing -> XY size
  compensation**: leave *Contour* at 0 (don't touch it, or the
  registration-pocket fit against the panel edge gets loose); set
  **Hole compensation to +0.10 to +0.15mm** for PETG, **+0.08 to
  +0.12mm** for PLA/PLA Pro+ (PLA holds circular holes slightly truer
  than PETG at the same nozzle temp).
- **Elephant's foot compensation**: 0.15-0.2mm for PETG, 0.10-0.15mm
  for PLA Pro+ - PETG's first layer squishes more, which shows up as
  undersized short features like the LED pilot holes.
- **Print a small test coupon first** - a plate with a few holes at the
  actual nominal diameters used here (2.50mm, 3.175mm, 3.275mm) -
  before committing to a full jig print. Test-fit the real bit/hardware
  and adjust the hole-compensation number from that measurement rather
  than trusting the figures above blindly; every printer/filament
  combo shifts this slightly.

### PETG (PrusaSlicer)

- **Nozzle**: 235°C first layer, 230°C other layers (typical generic
  PETG range 230-245°C - start at 230 and only raise it if layers
  aren't bonding; higher temps string more).
- **Bed**: 80-85°C first layer, 80°C other layers.
- **Cooling**: 0% fan for layers 1-2, ramp to 30-40% by layer 4-5, cap
  around 40-50% max - PETG wants less aggressive cooling than PLA.
  Min layer time 15-20s.
- **Retraction**: direct drive 0.8-1.2mm @ 25-35mm/s; Bowden 4-6.5mm @
  25-40mm/s. Enable "wipe while retracting" if available - PETG
  strings badly if this is loose.
- **Speed**: perimeters 40mm/s, external perimeters 25-30mm/s, infill
  50-60mm/s, first layer 20mm/s.
- **Bed adhesion**: PETG bonds aggressively to bare PEI - use a thin
  glue-stick layer or you risk peeling the sheet coating on removal.
  5mm brim recommended on the router template and larger drill guides.
- **Dry the filament first** if it's been open/humid - 55-65°C for
  4-6 hrs. PETG is hygroscopic; moisture shows up as weak layers,
  worst place to have it is the router template.

### PLA Pro+ / PLA+ (PrusaSlicer)

- **Nozzle**: 215-220°C first layer, 210-215°C other layers (generic
  "Pro"/"+" PLA blends run ~10-15°C hotter than plain PLA for their
  improved toughness/HDT - check your specific spool's TDS if you have
  one, but this range covers most eSUN PLA+/Polymaker PolyLite PLA
  Pro/Overture PLA+ style filaments).
- **Bed**: 60°C first layer, 55-60°C other layers.
- **Cooling**: 100% fan from layer 2 onward (drop to ~60% only if you
  see poor interlayer bonding) - PLA-family filaments want aggressive
  cooling, the opposite of PETG.
- **Retraction**: direct drive 0.6-0.8mm @ 35-45mm/s; Bowden 3-5mm @
  40-60mm/s - PLA strings much less than PETG, so this is more
  forgiving to tune.
- **Speed**: perimeters 45-55mm/s, external perimeters 30-35mm/s,
  infill 60-80mm/s, first layer 20mm/s.
- **Bed adhesion**: less aggressive to PEI than PETG - plain glue
  stick or even bare textured PEI is usually fine. 5mm brim still
  recommended on the larger flat jigs for warp resistance.
- **Note**: PLA Pro+'s ~55-60°C HDT is still well below PETG's
  ~70-80°C - fine for the drill guides (no heat exposure), but PETG
  remains the better choice specifically for the router template given
  the friction-heat concern in the workflow above.

## Jigs

### Front panel (VFD + LEDs + mounting holes)
- `jigs/front_panel_drill_guide.stl` - 46 holes: 34 around the VFD
  viewing window perimeter, 8 LED center-pilots, 4 VFD mounting-hole
  pilots.
- `jigs/front_panel_drill_guide_2_offset.stl` - 34 offset holes,
  VFD window perimeter only (no LEDs), for the interleave pass.
- `jigs/front_panel_router_template_vfd.stl` - exact 82.80 x 11.50mm
  window opening (verified against Noritake's real CU24025ECPB-W1J
  mechanical drawing, DOC NO. 37813 - not the datasheet text, the
  actual dimensioned drawing).

VFD mounting holes: 4x ⌀2.50mm, 113.00mm horizontal x 31.00mm vertical
spacing (center-to-center), each hole 2.50mm inset from its nearest
edge of the VFD's 125x36mm envelope. Cross-checked against a direct
caliper measurement on the physical unit during development - vertical
spacing matched exactly; horizontal spacing required correcting an
initial misreading of the drawing (see PROJECT_HANDOFF.md in the main
repo for the full trace of that correction) before it matched too.

**Mounting hole diameter - unresolved discrepancy, verify before ordering
hardware**: three different figures have turned up across sources - 2.50mm
(the doc-37813 mechanical drawing used for the jig above), "3.5mm dia"
(a text note on an older Noritake datasheet PDF for the same part), and
2.25mm (an earlier direct caliper measurement on the physical unit).
Re-check with calipers on the real hole before buying standoffs/screws.

**Standoff length: 7.74mm.** Found on that same older datasheet (not on
the doc-37813 drawing, which doesn't dimension module depth at all) - a
side cross-section shows 1.6mm PCB thickness, then 7.74mm from the PCB's
top surface up to the bottom of the display housing's own internal
support legs, then the housing block itself above that (13.0mm total,
PCB back to housing front face). The 7.74mm figure is the one that
matters for an external standoff: since the front-view drawing shows the
4 corner mounting holes on the same outline as the housing/bezel frame
(not a separate smaller PCB shape), the screw most likely runs through
the PCB from behind with the housing's front bezel referencing the panel
directly - meaning the standoff just needs to reproduce the gap the
module's own legs already dictate. 7.74mm isn't a stock standoff length
(common metric standoffs run 5/6/8/10mm) - either use an 8mm standoff
plus a ~0.25mm shim, or 3D-print a custom 7.74mm spacer.

LEDs: pilot-center only (not full perimeter) - 3mm THT LEDs with 5mm
panel-mount snap-in clips, drilled to full size afterward at the
marked centers. See [`LED_LABELS.md`](LED_LABELS.md) for the 8 LED
functions, their real left-to-right order, and the CNC-engraving plan
(F-Engrave settings, V-bit choice, per-label coordinates) for the 2-letter
labels under each one.

### Rear panel (DB9, USB-C, power, switches, antenna)
- `jigs/rear_panel_drill_guide.stl` - DB9 + USB-C perimeters, plus
  pilot-center holes for boot, reset, power switch, power jack, and the
  external antenna connector.
- `jigs/rear_panel_drill_guide_2_offset.stl` - offset interleave pass
  for the DB9/USB-C perimeters.
- `jigs/rear_panel_router_template.stl` - both non-round openings (DB9
  window, USB-C window) in one template, single registration.

Layout (see `renders/rear_panel_layout.png` for the full dimensioned
diagram), left to right:

| Feature | Size | X position (mm, panel-local) |
|---|---|---|
| USB-C (TC005 panel-mount jack) | 10.5 x 5.2mm opening, 2x⌀2.0mm mount holes @14.5mm spacing | 30.0 |
| Reset | pilot, finish to ⌀7mm | 52.0 |
| Boot | pilot, finish to ⌀7mm | 68.0 |
| DB9 | standard DE-9 cutout | 95.5 (centered) |
| Power switch | pilot, finish to ⌀6mm | 134.0 |
| Power jack | pilot, finish to ⌀11mm | 155.0 |
| SMA/RP-SMA antenna | pilot, finish to ⌀9.5mm (3/8") | 175.0 |

All items sit on the panel's shared Y=33mm centerline. Checked against
the enclosure's real internal floor rail (a 152.4 x 4.29mm structural
rail extracted from Hammond's own STEP file, sitting along the very
bottom edge of the panel) - everything above is 27mm+ clear of it
vertically, which is what actually matters here (the rail only occupies
the bottom 4.29mm of the panel's 66mm height, so X-position relative to
it is not the real constraint). Corner mounting-screw-boss positions
were not independently extracted from the STEP file - generous edge
margins (9mm+ between adjacent features, 20mm+ at the outer edges) are
used as a conservative stand-in.

**USB-C, Boot, and Power switch/jack are grouped deliberately, not just
for space**: USB-C sits with Reset/Boot as a "programming cluster" since
flashing an ESP32 means holding BOOT, tapping RESET, and having USB
connected all at once - keeping them adjacent mirrors common dev-board
layout. The power switch/jack form their own cluster on the opposite
side. SMA sits outboard of the power cluster, isolated with its own
clearance - **U2 (the ESP32-S3 module, carrying the U.FL antenna feed)
sits ~48mm away on the opposite, front-panel-facing edge of the PCB**,
confirmed via a direct pcbnew position query - so the U.FL pigtail runs
most of the board's length regardless of exactly where on the rear panel
the antenna connector sits; this board's linear (not switching)
regulators mean there's no significant local EMI source near the power
jack to steer away from either.

DB9 cutout profile sourced from Winford Engineering's "Recommended D-Sub
Connector Panel Cutouts" reference drawing (standard generic DE-9,
matching a standard motherboard DB9 connector) - a compound shape, not
a plain rectangle: a rounded-rect main body (20.574 x 11.43mm, corner
radius 3.81mm) with two small jackscrew-clearance lobes (3.175mm dia)
merged onto its sides, positioned so their outer edge reaches the
drawing's explicit 24.892mm mounting-hole span.

**The interpretive call**: two of the reference drawing's sub-dimensions
(0.81in body width, 0.51in a secondary width) didn't cleanly reconcile
with the visible lobe geometry when read pixel-for-pixel off the
rendered PDF. The lobe placement used here reconciles them by anchoring
the lobes' *outer* edge to the drawing's unambiguous 0.98in mounting-hole
span - a reasonable, documented reconstruction, but not verified against
a real DB9 shell the way the VFD dimensions were against the real
Noritake drawing and a physical caliper check. **Test-fit before
cutting final material.**

**Also flagged**: the same reference notes 0.062in (1.57mm) max panel
thickness for standard-length jackscrews. This panel (~1.7mm) is at or
slightly past that - extra-long jackscrews (the reference calls out
Winford's own DJSK-35.0-A) may be needed instead of whatever ships
with the connector.

**USB-C is a TC005**, a generic panel-mount USB-C connector
("TYPE-C-16P" marking, 16-pin/full-data variant of this shell; the same
TC005 body is also sold with 2-pin power-only and 5-pin data-only
contact boards, panel footprint identical across all of them). Confirmed
via two independent images - an initial low-resolution supplier photo,
then a full multi-view dimensioned factory drawing (portrait-oriented,
same numbers, axes swapped) that matched every number exactly. The
opening's corner radius (flat sides, eased corners only - not the oval
shape of a bare USB-C connector tongue) is a visual estimate, not an
explicit drawing dimension - verify against the real part before
routing final material.

**SMA/RP-SMA note**: RP-SMA (reverse-polarity, the far more common type
on consumer WiFi antennas/pigtails due to FCC Part 15 rules) uses the
identical mechanical bulkhead shell and 3/8"-32 UNEF thread as standard
SMA - the "reverse polarity" only swaps which side carries the male/
female center pin. Same 9.5mm panel hole either way; just confirm the
pigtail you order is genuinely RP-SMA to mate with your antenna.

## Renders

- `renders/rear_panel_layout.png` / `renders/front_panel_layout.png` -
  dimensioned 2D layout diagrams for each panel, used to verify hole
  positions and clearances before generating the jig STLs above.
- `renders/led_label_layout.png` - placement check for the 8 two-letter
  LED labels (see [`LED_LABELS.md`](LED_LABELS.md)) - centered under
  each LED hole, ready to reference in F-Engrave.
- `renders/vfd_jig1_layout.png` / `renders/vfd_interleave.png` /
  `renders/db9_shape.png` / `renders/db9_layout.png` - earlier
  schematic hole-position layouts from initial jig development.

None of the above are photos of a printed/fitted jig.
