#pragma once

/**
 * Register all system drivers
 */
void register_drivers(void);

/**
 * Probe and initialize devices with registered drivers
 */
void probe_devices(void);
