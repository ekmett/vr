// -*- C++ -*-
/* Copyright 1999
 * Thu Apr 15 15:17:17 1999  Brian Smits  (bes@phoenix.cs.utah.edu)
 * 
 * RiSunConstants.C
 * 
 *	
 * 
 * $Id: RiSunConstants.C,v 1.2 1999/07/15 00:07:33 bes Exp $ 
 * 
 */
#ifndef RICOMMON_H
#include <RiCommon.H>
#endif

#ifndef RISUNSKY_H
#include <RiSunSky.H>
#endif

#ifndef RICOLORXYZV_H
#include <RiColorXYZV.H>
#endif

#ifndef RISPECTRALCURVE_H
#include <RiSpectralCurve.H>
#endif

/* All data lifted from MI */
/* Units are either [] or cm^-1. refer when in doubt MI */

// k_o Spectrum table from pg 127, MI.
static RiReal k_oWavelengths[64] = {
300, 305, 310, 315, 320,
325, 330, 335, 340, 345,
350, 355,

445, 450, 455, 460, 465,
470, 475, 480, 485, 490,
495,

500, 505, 510, 515, 520,
525, 530, 535, 540, 545,
550, 555, 560, 565, 570,
575, 580, 585, 590, 595,

600, 605, 610, 620, 630,
640, 650, 660, 670, 680,
690,

700, 710, 720, 730, 740,
750, 760, 770, 780, 790,
};




static RiReal k_oAmplitudes[65] = {
  10.0,
  4.8,
  2.7,
  1.35,
  .8,
  .380,
  .160,
  .075,
  .04,
  .019,
  .007,
  .0,
  
  .003,
  .003,
  .004,
  .006,
  .008,
  .009,
  .012,
  .014,
  .017,
  .021,
  .025,

  .03,
  .035,
  .04,
  .045,
  .048,
  .057,
  .063,
  .07,
  .075,
  .08,
  .085,
  .095,
  .103,
  .110,
  .12,
  .122,
  .12,
  .118,
  .115,
  .12,

  .125,
  .130,
  .12,
  .105,
  .09,
  .079,
  .067,
  .057,
  .048,
  .036,
  .028,
  
  .023,
  .018,
  .014,
  .011,
  .010,
  .009,
  .007,
  .004,
  .0,
  .0
};


// k_g Spectrum table from pg 130, MI.
static RiReal k_gWavelengths[4] = {
  759,
  760,
  770,
  771
};

static RiReal k_gAmplitudes[4] = {
  0,
  3.0,
  0.210,
  0
};

// k_wa Spectrum table from pg 130, MI.
static RiReal k_waWavelengths[13] = {
  689,
  690,
  700,
  710,
  720,
  730,
  740,
  750,
  760,
  770,
  780,
  790,
  800
};

static RiReal k_waAmplitudes[13] = {
  0,
  0.160e-1,
  0.240e-1,
  0.125e-1,
  0.100e+1,
  0.870,
  0.610e-1,
  0.100e-2,
  0.100e-4,
  0.100e-4,
  0.600e-3,
  0.175e-1,
  0.360e-1
};


// 380-750 by 10nm
static float solAmplitudes[38] = {
    165.5, 162.3, 211.2, 258.8, 258.2,
    242.3, 267.6, 296.6, 305.4, 300.6,
    306.6, 288.3, 287.1, 278.2, 271.0,
    272.3, 263.6, 255.0, 250.6, 253.1,
    253.5, 251.3, 246.3, 241.7, 236.8,
    232.1, 228.2, 223.4, 219.7, 215.3,
    211.0, 207.3, 202.4, 198.7, 194.3,
    190.7, 186.3, 182.6
};



/**********************************************************
// Sunlight Transmittance Functions
// 
// ********************************************************/

/* Most units not in SI system - For units, refer MI */
RiSpectrum RiSunSky::ComputeAttenuatedSunlight(RiReal theta, int turbidity)
{
    RiIrregularSpectralCurve k_oCurve(k_oAmplitudes, k_oWavelengths, 64);
    RiIrregularSpectralCurve k_gCurve(k_gAmplitudes, k_gWavelengths, 4);
    RiIrregularSpectralCurve k_waCurve(k_waAmplitudes, k_waWavelengths, 13);
    RiRegularSpectralCurve   solCurve(solAmplitudes, 380, 750, 38);  // every 10 nm  IN WRONG UNITS
				                                     // Need a factor of 100 (done below)
    RiReal  data[91];  // (800 - 350) / 5  + 1

    RiReal beta = 0.04608365822050 * turbidity - 0.04586025928522;
    RiReal tauR, tauA, tauO, tauG, tauWA;

    RiReal m = 1.0/(cos(theta) + 0.15*pow(93.885-theta/RiPi*180.0,-1.253));  // Relative Optical Mass
				// equivalent  
//    RiReal m = 1.0/(cos(theta) + 0.000940 * pow(1.6386 - theta,-1.253));  // Relative Optical Mass

    int i;
    RiReal lambda;
    for(i = 0, lambda = 350; i < 91; i++, lambda+=5) {
				// Rayleigh Scattering
				// Results agree with the graph (pg 115, MI) */
	tauR = exp( -m * 0.008735 * pow(lambda/1000, RiReal(-4.08)));

				// Aerosal (water + dust) attenuation
				// beta - amount of aerosols present 
				// alpha - ratio of small to large particle sizes. (0:4,usually 1.3)
				// Results agree with the graph (pg 121, MI) 
	const RiReal alpha = 1.3;
	tauA = exp(-m * beta * pow(lambda/1000, -alpha));  // lambda should be in um

				// Attenuation due to ozone absorption  
				// lOzone - amount of ozone in cm(NTP) 
				// Results agree with the graph (pg 128, MI) 
	const RiReal lOzone = .35;
	tauO = exp(-m * k_oCurve(lambda) * lOzone);

				// Attenuation due to mixed gases absorption  
				// Results agree with the graph (pg 131, MI)
	tauG = exp(-1.41 * k_gCurve(lambda) * m / pow(1 + 118.93 * k_gCurve(lambda) * m, 0.45));

				// Attenuation due to water vapor absorbtion  
				// w - precipitable water vapor in centimeters (standard = 2) 
				// Results agree with the graph (pg 132, MI)
	const RiReal w = 2.0;
	tauWA = exp(-0.2385 * k_waCurve(lambda) * w * m /
		    pow(1 + 20.07 * k_waCurve(lambda) * w * m, 0.45));

	data[i] = 100 * solCurve(lambda) * tauR * tauA * tauO * tauG * tauWA;  // 100 comes from solCurve being
	                                                                       // in wrong units. 

    }
    return RiRegularSpectralCurve(data, 350,800,91);  // Converts to RiSpectrum
}

