# MOS2 TV-style 3D-printable case

[简体中文](3D_PRINT_CASE.md)

This project includes a TV-style enclosure for the MOS2 board and its 240×240 display. The model is split into a front case, rear cover, antenna, base, cover plate and two knobs for separate printing and assembly.

[Buy the compatible MOS2 board on Taobao](https://item.taobao.com/item.htm?id=854776370120)

![MOS2 TV-style case renders](../hardware/MOS2-TV-3D-PRINT-CASE/render-preview.png)

## Download

- [Complete STL archive](../hardware/MOS2-TV-3D-PRINT-CASE/MOS2-TV-3D-PRINT-CASE.zip)

The archive contains 11 STL models. The rear cover, antenna, cover plate and two knobs are shared by every option. Select the front case and stand as described below.

## Front case and stand options

### Standard front case with a separate stand

Print `1-Front-Case.stl`, then select one stand:

- `4-Base.stl`: compact stand.
- `4-Base-Retro-Four-Leg-10deg.stl`: retro TV four-leg stand with a fixed 10° viewing angle.
- `4-Base-Retro-Four-Leg-15deg.stl`: retro TV four-leg stand with a fixed 15° viewing angle.

![Retro four-leg stands at 10 and 15 degrees](../hardware/MOS2-TV-3D-PRINT-CASE/retro-four-leg-base-preview.png)

### Front case with integrated legs

No separate stand is required. Select one front case with integrated legs:

- `1-Front-Case_Four-Leg-5deg.stl`: integrated short legs with a 5° viewing angle.
- `1-Front-Case_Four-Leg-8deg.stl`: integrated short legs with an 8° viewing angle.

![Front cases with integrated legs at 5 and 8 degrees](../hardware/MOS2-TV-3D-PRINT-CASE/integrated-four-leg-preview.png)

### Shared parts

- `2-Rear-Cover.stl`: rear cover.
- `3-Antenna.stl`: antenna.
- `5-Cover-Plate.stl`: cover plate.
- `6-Knob-1.stl`: upper knob.
- `7-Knob-2.stl`: lower knob.

## Printing notes

- Import the front case, stand and shared parts required by your selected option, then choose orientation, supports, layer height and infill for your printer and material.
- Different filaments can be used for the enclosure, antenna and knobs. The six renders are color references only.
- Verify dimensions in the slicer before printing and avoid automatic model scaling.
- Remove supports and rough edges, then test-fit the display, board and controls before securing the rear cover.

The files contain the enclosure only. The MOS2 board, display, buzzer and wiring are not included.
