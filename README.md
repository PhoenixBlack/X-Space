![X-Space](http://i.imgur.com/FgmlT.png)

X-Space is an aerospace simulator based on X-Plane flight simulator. It is available
as a plugin, and as a dedicated server (for calculating spaceflight).


Status
--------------------------------------------------------------------------------
This project is no longer active. All the source code is re-licensed under BSD
license and provided for anybody who wants additional references when writing
X-Plane plugins.

The project is superceded by EVDS (External Vessel Dynamics Simulator) and
FoxWorks Aerospace Simulator for X-Plane.


Features
--------------------------------------------------------------------------------
 - Provides information about partial concentrations and densities of atmospheric gases
 - Clouds visible from orbit (procedural clouds based on realtime weather)
 - Different visual effects based on fuel used in the rocket engine
 - Detailed engine performance simulation (variation in thrust with external pressure)
 - Support for multiple stages/multi-part vessels
 - Visual effects from heating (glow due to heat radiation)
 - Earth gravitational field simulation (non-spherical gravity based on EGM96 model)
 - Support for extra fuel tanks (or additional weights)
 - Orbital flight simulation (uses custom physics engine to override X-Plane physics at high altitudes)
 - Detailed atmospheric model (including exosphere, high-altitude atmospheric drag)
 - Atmospheric scattering visible from orbit (for X-Plane 9, X-Plane 10 without HDR)
 - Support for any amount of custom engines (reaction control engines, additional rocket engines)
 - Advanced networking support (multiplayer)
 - Fuselage drag simulation (drag from capsules, hypersonic vessels)
 - Heating simulation (covers a wide range of external pressures and velocities)
 - Earth magnetic field simulation (fairly precise magnetic field data from WMM)
 - Material simulation (simulates properties of various materials the spacecraft is constructed from)
 - Extra camera views (relative to vessel reference point, vessel center of mass)
 - Vertical launch pads
 - Simulates planet rotation
 - Publishes a large amount of variables (datarefs) accessible by other plugins
 
![Re-entry heating model](http://i.imgur.com/KkeqX.png)


Installing
--------------------------------------------------------------------------------
Please remove the old version of X-Space if it's present.

Just copy contents of folders in this archive into corresponding folders in X-Plane 9
(or X-Plane 10) folder. The folders must be merged together (not replaced!).

To uninstall it just delete the following folders:
 - \X-Plane 9\Resources\plugins\x-space
 - \X-Plane 9\Aircraft\XSAG


Compiling
--------------------------------------------------------------------------------
 - Microsoft Visual Studio 2008 projects included
 - Linux makefiles included
 - MacOS makefiles included
 
All dependencies are available in zip files under dependencies folder (for each
supported OS).


Authorship
--------------------------------------------------------------------------------
 - X-Space (C) 2009-2012 by Black Phoenix
 - XGDC OS (C) 2008-2012  by Black Phoenix
 - Simple OGL Image Library by Jonathan Dummer
 - Scriptable Avionics Simulation Library by A.A.Babichev
 - Lua 5.1.4 (C) 1994-2011 PUC-Rio
 - WMM model implementation by Manoj C Nair and Adam Woods
 - NRLMSISE-00 model implementation by Dominik Brodowski
 - RV-550 Ares-1 with Orion capsule model by Curt Boyll
 - SV-200 UHLSS model by Jeff Scott
