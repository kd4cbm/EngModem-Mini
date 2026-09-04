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

- **40-50% infill**, gyroid or cubic pattern.
- **4+ perimeter walls, 3+ solid top/bottom layers** - more important
  than infill % here. The thin webs between adjacent drill-guide holes
  (as tight as ~1.5-2mm at this spacing) are almost entirely perimeter
  shell, not infill lattice, and the top plate needs to stay flat and
  accurate under repeated drill pressure.
- **PLA is fine for the drill guides. Consider PETG for the router
  template specifically** - cheap insurance given the heat concern
  above; PETG's heat-deflection temp (~70-80°C) is meaningfully higher
  than PLA's (~50-60°C) for a small material/print-time cost.

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

LEDs: pilot-center only (not full perimeter) - 3mm THT LEDs with 5mm
panel-mount snap-in clips, drilled to full size afterward at the
marked centers.

### Rear panel (DB9, draft)
- `jigs/rear_panel_drill_guide_db9.stl` - 13 holes around the DB9
  cutout perimeter.
- `jigs/rear_panel_drill_guide_db9_offset.stl` - 13 offset holes.
- `jigs/rear_panel_router_template_db9.stl` - exact-shape template
  opening.

DB9 is the sole feature on this panel, centered both axes. Cutout
profile sourced from Winford Engineering's "Recommended D-Sub
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

## Renders

`renders/vfd_jig1_layout.png` / `renders/vfd_interleave.png` /
`renders/db9_shape.png` / `renders/db9_layout.png` - schematic
hole-position layouts used to verify each jig's geometry before
export, not photos of a printed/fitted jig.
