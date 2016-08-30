// -*- C++ -*-
#ifndef RISUNSKY_H
#define RISUNSKY_H
/* Copyright 1999 
 * Mon Apr 19 13:42:11 1999  Brian Smits  (bes@phoenix.cs.utah.edu)
 * 
 * RiSunSky.H
 * 
 *	
 * 
 * $Id: RiSunSky.H,v 1.1 1999/06/10 20:12:32 bes Exp $ 
 * 
 */

#ifndef RICOMMON_H
#include <RiCommon.H>
#endif

#ifndef RISPECTRUM_H
#include <RiSpectrum.H>
#endif

#ifndef RIVECTOR3_H
#include <RiVector3.H>
#endif

/***************************************************************
CLASS
    RiSunSky
     <one line summary> 

DESCRIPTION
     <Detailed description with any warnings>

****************************************************************/

class RiSunSky {
  public:
				// GROUP: Constructors and assignment
				//// Constructs an RiSunSky based on
				// [in] lat Latitude (0-360)
				// [in] long Longitude (-90,90) south to north
				// [in] sm  Standard Meridian
				// [in] jd  Julian Day (1-365)
				// [in] tod Time Of Day (0.0,23.99) 14.25 = 2:15PM
				// [in] turb  Turbidity (1.0,30+) 2-6 are most useful for clear days.
				// [in] initAtmEffects  if atm effects are not initialized, bad things will
				// happen if you try to use them....
    RiSunSky(RiReal lat, RiReal longi, int sm, int jd, RiReal tod, RiReal turb,bool initAtmEffects = true);
				// GROUP: Members
				////  Position (actual Direction) of the sun
				// South = +x,  East = +y, up = +z
    RiVector3  	GetSunPosition() const;
				//// [out] theta  Sun angle down? from straight above
				//   [out] phi    Sun angle around from South? 
    void 	SunThetaPhi(RiReal &theta, RiReal &phi) const;  
				////  Spectral radiance making it to the earth's surface
    RiSpectrum 	GetSunSpectralRadiance() const;
				////  Solid angle of the sun (constant
    RiReal	GetSunSolidAngle() const;
				//// Spectral radiance of the sky in the specified direction
    RiSpectrum  GetSkySpectralRadiance(const RiVector3 &v) const;
				//// An alternate way of getting the sky's spectral radiance
    RiSpectrum  GetSkySpectralRadiance(RiReal theta, RiReal phi) const;
				////  Compute Atmospheric Perspective based on
				// [in] viewer  Position of the viewer
				// [in] source  Position of the distant point
				// [out] atmAttenuation Attenuation due to atmosphere
				// [out] atmInscatter  Amount of light scattered into the viewing direction
    void GetAtmosphericEffects(const RiVector3 &viewer,
			       const RiVector3 &source,
			       RiSpectrum &atmAttenuation,
			       RiSpectrum &atmInscatter ) const;
  private:
				//// Copy Constructor (unimplemented)
    RiSunSky(const RiSunSky &);
				//// Assignment (unimplemented)
    RiSunSky &operator=(const RiSunSky &);
				//// Compute the sun's position based on IES Sunlight Publication ????
    void 	InitSunThetaPhi();
				//// Transmittance for the sun weighted by sun's spectrum.
    RiSpectrum ComputeAttenuatedSunlight(RiReal theta, int turbidity);
				//// Conversion from xy to a Spectrum
    RiSpectrum ChromaticityToSpectrum(RiReal x, RiReal y) const;
				// GROUP:  RiAtmPerspective.C Functions
				////
    RiSpectrum AttenuationFactor(RiReal h0, RiReal thetav, RiReal s) const;
				////
    RiSpectrum InscatteredRadiance(RiReal h0, RiReal thetav, RiReal
				   phiv, RiReal s) const;
				////
    RiSpectrum GetNeta(RiReal theta, RiReal v) const;
				////
    void CalculateA0(RiReal thetav, RiReal phiv, RiSpectrum& A0_1, RiSpectrum& A0_2) const;
				////
    void CreateConstants();
				//// Preprocessing for calculating ambient scattered light for all
				//  directions
    void InitA0() const;
				////
    RiReal PerezFunction(const RiReal *lam, RiReal theta, RiReal phi, RiReal lvz) const;
				//Group: Data
    RiReal 	latitude, longitude;
    int 	julianDay;
    RiReal 	timeOfDay;
    int 	standardMeridian;
    RiReal 	turbidity;
				//// Sun Position Vector
    RiVector3 	toSun;
				//// Sun Position
    RiReal	thetaS, phiS;
				//// Spectral Radiance computed at creation time
    RiSpectrum 	sunSpectralRad;
				//// Solid angle of the sun when seen from earth
    RiReal 	sunSolidAngle;
				//// Perez data
    RiReal zenith_Y, zenith_x, zenith_y;
				//// Perez coefficients (change to 5 when arrays become zero based)
    RiReal perez_Y[6], perez_x[6], perez_y[6];
				// GROUP: Constants for Atmospheric Perspective
				////
    RiSpectrum beta_m, beta_p, beta_m_ang_prefix,  beta_p_ang_prefix;
				////
    RiReal	V;
				////
    bool atmInited;
};


#endif /* RISUNSKY_H */
