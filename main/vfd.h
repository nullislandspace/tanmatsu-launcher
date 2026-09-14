// SPDX-License-Identifier: MIT
// Optional external VFD (NE-HCS12SS59T-R1, 12-character I2C VFD).
//
// Some Tanmatsu units carry this display jumpered to address 0x13 (the
// default 0x10 collides with an on-board device on the internal bus). Apps
// may switch it on; the launcher does not use it, so it switches the tube
// off during startup to save power.

#pragma once

// Probe for the VFD on the CATT port I2C bus, then on the internal I2C bus,
// and switch it off if found. Blocks for at most a few bus timeouts. A no-op
// when no VFD is present or on targets other than Tanmatsu.
void vfd_power_off(void);
