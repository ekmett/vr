// -*- C++ -*-
/* Copyright 1999 
 * Wed Apr 14 10:07:50 1999  Brian Smits  (bes@phoenix.cs.utah.edu)
 * 
 * RiSunSky.C
 * 
 *	
 * 
 * $Id: RiSunSky.C,v 1.2 1999/07/15 00:07:34 bes Exp $ 
 * 
 */

/**********************************************************
//  TODO							   
// Fix Indexing (make it zero based, not 1 based...)
// ********************************************************/

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


#include <iostream.h>


/**********************************************************
// Atmospheric Perspective Constants
// 
// ********************************************************/


// density decreses as exp(-alpha*h) alpha in m^-1.
// _1 refers to Haze, _2 referes to molecules.

static RiReal const Alpha_1 = 0.83331810215394e-3;
static RiReal const Alpha_2 = 0.11360016092149e-3;

#define Nt 20 // Number of bins for theta
#define Np 20 // Number of bins for phi

#define radians(x) ((x)/180.0*RiPi)

static RiSpectrum AT0_1[Nt+1][Np+1];
static RiSpectrum AT0_2[Nt+1][Np+1];

inline RiSpectrum exp(const RiSpectrum &in)
{
    RiSpectrum out;
    for(int i = 0; i < RiSpectrum::GetNumComponents(); i++)
      out[i] = exp(in[i]);
    return out;
}

/* All angles in radians, theta angles measured from normal */
inline RiReal RiAngleBetween(RiReal thetav, RiReal phiv, RiReal theta, RiReal phi)
{
  RiReal cospsi = sin(thetav) * sin(theta) * cos(phi-phiv) + cos(thetav) * cos(theta);
  if (cospsi > 1) return 0;
  if (cospsi < -1) return M_PI;
  return  acos(cospsi);
}




RiSunSky::RiSunSky(RiReal lat,
		   RiReal longi,
		   int sm,       // standardMeridian
		   int jd,       // julianDay
		   RiReal tOfDay,   // timeOfDay
		   RiReal turb,
		   bool initAtmEffects) // turbidity
{
    latitude = lat;
    longitude = longi;
    julianDay = jd;
    timeOfDay = tOfDay;
    standardMeridian = sm * 15.0;  // sm is actually timezone number (east to west, zero based...)
    turbidity = turb;

    V = 4.0; // Junge's exponent.

    InitSunThetaPhi();
    toSun = RiVector3(cos(phiS)*sin(thetaS), sin(phiS)*sin(thetaS), cos(thetaS));

    sunSpectralRad =  ComputeAttenuatedSunlight(thetaS, turbidity);
    sunSolidAngle =  0.25*RiPi*1.39*1.39/(150*150);  // = 6.7443e-05

    RiReal theta2 = thetaS*thetaS;
    RiReal theta3 = theta2*thetaS;
    RiReal T = turb;
    RiReal T2 = turb*turb;

    RiReal chi = (4.0/9.0 - T / 120.0) * (RiPi - 2 * thetaS);
    zenith_Y = (4.0453 * T - 4.9710) * tan(chi) - .2155 * T + 2.4192;
    zenith_Y *= 1000;  // conversion from kcd/m^2 to cd/m^2
    
    zenith_x =
	(+0.00165*theta3 - 0.00374*theta2 + 0.00208*thetaS + 0)          * T2 +
	(-0.02902*theta3 + 0.06377*theta2 - 0.03202*thetaS + 0.00394) * T +
	(+0.11693*theta3 - 0.21196*theta2 + 0.06052*thetaS + 0.25885);

    zenith_y =
	(+0.00275*theta3 - 0.00610*theta2 + 0.00316*thetaS  + 0)          * T2 +
	(-0.04214*theta3 + 0.08970*theta2 - 0.04153*thetaS  + 0.00515) * T +
	(+0.15346*theta3 - 0.26756*theta2 + 0.06669*thetaS  + 0.26688);
      
    perez_Y[1] =    0.17872 *T  - 1.46303;
    perez_Y[2] =   -0.35540 *T  + 0.42749;
    perez_Y[3] =   -0.02266 *T  + 5.32505;
    perez_Y[4] =    0.12064 *T  - 2.57705;
    perez_Y[5] =   -0.06696 *T  + 0.37027;
      
    perez_x[1] =   -0.01925 *T  - 0.25922;
    perez_x[2] =   -0.06651 *T  + 0.00081;
    perez_x[3] =   -0.00041 *T  + 0.21247;
    perez_x[4] =   -0.06409 *T  - 0.89887;
    perez_x[5] =   -0.00325 *T  + 0.04517;
      
    perez_y[1] =   -0.01669 *T  - 0.26078;
    perez_y[2] =   -0.09495 *T  + 0.00921;
    perez_y[3] =   -0.00792 *T  + 0.21023;
    perez_y[4] =   -0.04405 *T  - 1.65369;
    perez_y[5] =   -0.01092 *T  + 0.05291;

    if(initAtmEffects) {
				// Atmospheric Perspective initialization
	CreateConstants();	
	InitA0();
    }
    atmInited = initAtmEffects;
    
}

/**********************************************************
// South = x,  East = y, up = z
// All times in decimal form (6.25 = 6:15 AM)
// All angles in Radians
// ********************************************************/

void RiSunSky::SunThetaPhi(RiReal &stheta, RiReal &sphi) const
{
    sphi = phiS;
    stheta = thetaS;
}

/**********************************************************
// South = x,  East = y, up = z
// All times in decimal form (6.25 = 6:15 AM)
// All angles in Radians
// From IES Lighting Handbook pg 361
// ********************************************************/

void RiSunSky::InitSunThetaPhi()
{
    RiReal solarTime = timeOfDay +
	(0.170*sin(4*RiPi*(julianDay - 80)/373) - 0.129*sin(2*RiPi*(julianDay - 8)/355)) +
	(standardMeridian - longitude)/15.0;

    RiReal solarDeclination = (0.4093*sin(2*RiPi*(julianDay - 81)/368));

    RiReal solarAltitude= asin(sin(radians(latitude)) * sin(solarDeclination) -
			       cos(radians(latitude)) * cos(solarDeclination) * cos(RiPi*solarTime/12));

    RiReal opp, adj;
    opp = -cos(solarDeclination) * sin(RiPi*solarTime/12);
    adj = -(cos(radians(latitude)) * sin(solarDeclination) +
	    sin(radians(latitude)) * cos(solarDeclination) *  cos(RiPi*solarTime/12));
    RiReal solarAzimuth=atan2(opp,adj);

    phiS = -solarAzimuth;
    thetaS = RiPi / 2.0 - solarAltitude;

}

RiVector3 RiSunSky::GetSunPosition() const
{
    return toSun;
}

/**********************************************************
// Solar Radiance
// 
// ********************************************************/

RiSpectrum RiSunSky::GetSunSpectralRadiance() const
{
    return sunSpectralRad;
}

RiReal RiSunSky::GetSunSolidAngle() const
{
    return sunSolidAngle;
}

/**********************************************************
// Sky Radiance
// 
// ********************************************************/

RiSpectrum RiSunSky::GetSkySpectralRadiance(const RiVector3 &varg) const
{
    RiReal theta, phi;
    RiUnitVector3 v = varg;
    if (v.Z() < 0) return RiSpectrum(0.000000);
    if (v.Z() < 0.001) 
      v = RiUnitVector3(v.X(),v.Y(),.001 );
    
    theta = acos(v.Z());
    if (fabs(theta)< 1e-5) phi = 0;
    else  phi = atan2(v.Y(),v.X());
    return GetSkySpectralRadiance(theta,phi);
}

inline RiReal RiSunSky::PerezFunction(const RiReal *lam, RiReal theta, RiReal gamma, RiReal lvz) const
{
   RiReal den = ((1 + lam[1]*exp(lam[2])) *
		 (1 + lam[3]*exp(lam[4]*thetaS) + lam[5]*cos(thetaS)*cos(thetaS)));
                
   RiReal num = ((1 + lam[1]*exp(lam[2]/cos(theta))) *
		 (1 + lam[3]*exp(lam[4]*gamma)  + lam[5]*cos(gamma)*cos(gamma)));

   return lvz* num/den;
}

RiSpectrum RiSunSky::GetSkySpectralRadiance(RiReal theta, RiReal phi) const
{
    RiReal gamma = RiAngleBetween(theta,phi,thetaS,phiS);
				// Compute xyY values
    RiReal x = PerezFunction(perez_x, theta, gamma, zenith_x);
    RiReal y = PerezFunction(perez_y, theta, gamma, zenith_y);
    RiReal Y = PerezFunction(perez_Y, theta, gamma, zenith_Y);

    RiSpectrum spect = ChromaticityToSpectrum(x,y);

				// A simple luminance function would be more efficient.
    return Y * spect / RiColorXYZV(spect).Y();
}



/**********************************************************
// Atmospheric perspective functions
// 
// ********************************************************/


/**********************************************************
// Initialization
// 
// ********************************************************/





void RiSunSky::CalculateA0(RiReal thetav, RiReal phiv, RiSpectrum &A0_1, RiSpectrum &A0_2) const
{
    RiReal psi;
    RiSpectrum skyRad;
    RiSpectrum beta_ang_1, beta_ang_2;
    RiReal theta, phi, phiDelta = M_PI/20;
    RiReal thetaDelta = M_PI/2/20;
    RiReal thetaUpper;
  
    RiSpectrum skyAmb_1 = 0;
    RiSpectrum skyAmb_2 = 0;
  
    thetaUpper = M_PI / 2;

    for (theta = 0; theta < thetaUpper; theta += thetaDelta)
      for (phi = 0; phi < 2 * M_PI; phi += phiDelta)  {
	  skyRad = GetSkySpectralRadiance(theta, phi);
	  psi = RiAngleBetween(thetav, phiv, theta, phi);
	  
	  beta_ang_1 = beta_p_ang_prefix * GetNeta(psi, V);
	  beta_ang_2 = beta_m_ang_prefix * (1+0.9324*cos(psi)*cos(psi));
	  
	  skyAmb_1 += skyRad * beta_ang_1 * sin(theta) * thetaDelta * phiDelta;
	  skyAmb_2 += skyRad * beta_ang_2 * sin(theta) * thetaDelta * phiDelta;
      }
  
    /* Sun's ambience term*/
  
    psi = RiAngleBetween(thetav, phiv, thetaS, phiS);

    beta_ang_1 = beta_p_ang_prefix * GetNeta(psi, V);
    beta_ang_2 = beta_m_ang_prefix * (1+0.9324*cos(psi)*cos(psi));
  
    RiSpectrum sunAmb_1 = sunSpectralRad * beta_ang_1 * sunSolidAngle;
    RiSpectrum sunAmb_2 = sunSpectralRad * beta_ang_2 * sunSolidAngle;
    
				// Sum of sun and sky  (should probably add a ground ambient)
    A0_1 =  (sunAmb_1 + skyAmb_1);
    A0_2 =  (sunAmb_2 + skyAmb_2);
}

void  RiSunSky::InitA0() const
{
    int i, j;
    RiReal theta, phi;
  
    RiReal delTheta = M_PI/Nt;
    RiReal delPhi = 2*M_PI/Np;
  
    cerr << "ggSunSky::Preprocessing: 0%\r";
    for (i=0,theta = 0; theta<=  M_PI; theta+= delTheta,i++) {
	for (j=0,phi=0; phi <= 2*M_PI; phi+= delPhi,j++)
	  CalculateA0(theta, phi,  AT0_1[i][j], AT0_2[i][j]);
	cerr << "ggSunSky::Preprocessing: " << 100*(i*Np+j)/((Nt+1)*Np) <<"%  \r";
    }
    cerr << "ggSunSky::Preprocessing:  100%   " << endl;
}      



/**********************************************************
// Evaluation
// 
// ********************************************************/


void RiSunSky::GetAtmosphericEffects(const RiVector3 &viewer, const RiVector3 &source,
				     RiSpectrum &attenuation, RiSpectrum &inscatter ) const
{
    assert(atmInited);
				// Clean up the 1000 problem
    RiReal h0 = viewer.Z()+1000;//1000 added to make sure ray doesnt
				//go below zero. 
    RiUnitVector3 direction = source - viewer;
    RiReal thetav = acos(direction.Z());
    RiReal phiv = atan2(direction.Y(),direction.X());
    RiReal s = (viewer - source).Length();


    // This should be changed so that we don't need to worry about it.
    if(h0+s*cos(thetav) <= 0)
    {
	attenuation = 1;
	inscatter = 0;
	cerr << "\nWarning: Ray going below earth's surface \n";
	return;
    }

    attenuation = AttenuationFactor(h0, thetav, s);
    inscatter   = InscatteredRadiance(h0, thetav, phiv, s);
}


inline RiReal EvalFunc(RiReal B, RiReal x)
{
    if (fabs(B*x)< 0.01) return x;
    return (1-exp(-B*x))/B;
}


RiSpectrum RiSunSky::AttenuationFactor(RiReal h0, RiReal theta, RiReal s) const
{
    RiReal costheta = cos(theta);
    RiReal B_1 = Alpha_1 * costheta;
    RiReal B_2 = Alpha_2 * costheta;
    RiReal constTerm_1 = exp(-Alpha_1*h0) * EvalFunc(B_1, s);
    RiReal constTerm_2 = exp(-Alpha_2*h0) * EvalFunc(B_2, s);
  
    return (exp(-beta_p * constTerm_1) *
	    exp(-beta_m * constTerm_2));
}




static void GetA0fromTable(RiReal theta, RiReal phi, RiSpectrum &A0_1, RiSpectrum &A0_2)
{
  RiReal eps = 1e-4;
  if (phi < 0) phi += 2*M_PI; // convert phi from -pi..pi to 0..2pi
  theta = theta*Nt/M_PI - eps;
  phi = phi*Np/(2*M_PI) - eps;
  if (theta < 0) theta = 0;
  if (phi < 0) phi = 0;
  int i = (int) theta;
  RiReal u = theta - i;
  int j = (int)phi;
  RiReal v = phi - j;

  A0_1 = (1-u)*(1-v)*AT0_1[i][j] + u*(1-v)*AT0_1[i+1][j]
    + (1-u)*v*AT0_1[i][j+1] + u*v*AT0_1[i+1][j+1];
  A0_2 = (1-u)*(1-v)*AT0_2[i][j] + u*(1-v)*AT0_2[i+1][j]
    + (1-u)*v*AT0_2[i][j+1] + u*v*AT0_2[i+1][j+1];
}

inline RiReal RiHelper1(RiReal A, RiReal B, RiReal C, RiReal D,
			RiReal H, RiReal K, RiReal u)
{
    RiReal t = exp(-K*(H-u));
    return (t/K)*((A*u*u*u + B*u*u + C*u +D) -
		  (3*A*u*u + 2*B*u + C)/K +
		  (6*A*u + 2*B)/(K*K) -
		  (6*A)/(K*K*K));
}

inline void RiCalculateABCD(RiReal a, RiReal b, RiReal c, RiReal d, RiReal e,
			    RiReal den, RiReal &A, RiReal &B, RiReal &C, RiReal &D)
{
    A = (-b*d -2 + 2*c + a*e - b*e + a*d)/den;
    B = -(2*a*a*e + a*a*d - 3*a - a*b*e +
	  3*a*c + a*b*d - 2*b*b*d - 3*b - b*b*e + 3*b*c)/den;
    C =( -b*b*b*d - 2*b*b*a*e -   b*b*a*d + a*a*b*e +
	2*a*a*b*d - 6*a*b     + 6*b*a*c   + a*a*a*e)/den;
    D = -(   b*b*b - b*b*b*a*d - b*b*a*a*e + b*b*a*a*d
	  -3*a*b*b + b*e*a*a*a - c*a*a*a + 3*c*b*a*a)/den;
}

RiSpectrum RiSunSky::InscatteredRadiance(RiReal h0, RiReal theta, RiReal phi, RiReal s) const
{
    RiReal costheta = cos(theta);
    RiReal B_1 = Alpha_1*costheta;
    RiReal B_2 = Alpha_2*costheta;
    RiSpectrum A0_1; 
    RiSpectrum A0_2;
    RiSpectrum result;
    int i;
    RiSpectrum I_1, I_2;
  
    GetA0fromTable(theta, phi, A0_1, A0_2);

    // approximation (< 0.3 for 1% accuracy)
    if (fabs(B_1*s) < 0.3) {
	RiReal constTerm1 =  exp(-Alpha_1*h0);
	RiReal constTerm2 =  exp(-Alpha_2*h0);

	RiSpectrum C_1 = beta_p * constTerm1;
	RiSpectrum C_2 = beta_m * constTerm2;
	    
	for(int i = 0; i < RiSpectrum::GetNumComponents(); i++) {
	    I_1[i] =  (1-exp(-(B_1 + C_1[i] + C_2[i]) * s)) / (B_1 + C_1[i] + C_2[i]);
	    I_2[i] =  (1-exp(-(B_2 + C_1[i] + C_2[i]) * s)) / (B_2 + C_1[i] + C_2[i]);
	}

	return A0_1 * constTerm1 * I_1 + A0_2 * constTerm2 * I_2;
    }

    // Analytical approximation
    RiReal A,B,C,D,H1,H2,K;
    RiReal u_f1, u_i1,u_f2, u_i2, int_f, int_i, fs, fdashs, fdash0;
    RiReal a1,b1,a2,b2;
    RiReal den1, den2;

    b1 = u_f1 = exp(-Alpha_1*(h0+s*costheta));
    H1 = a1 = u_i1 = exp(-Alpha_1*h0);
    b2 = u_f2 = exp(-Alpha_2*(h0+s*costheta));
    H2 = a2 = u_i2 = exp(-Alpha_2*h0);
    den1 = (a1-b1)*(a1-b1)*(a1-b1);
    den2 = (a2-b2)*(a2-b2)*(a2-b2);
    
    for (i = 0; i < RiSpectrum::GetNumComponents(); i++) {
	// for integral 1
	K = beta_p[i]/(Alpha_1*costheta);
	fdash0 = -beta_m[i]*H2;
	fs = exp(-beta_m[i]/(Alpha_2*costheta)*(u_i2 - u_f2));
	fdashs = -fs*beta_m[i]*u_f2;

	RiCalculateABCD(a1,b1,fs,fdash0,fdashs,den1,A,B,C,D);
	int_f = RiHelper1(A,B,C,D,H1,K, u_f1);
	int_i = RiHelper1(A,B,C,D,H1,K, u_i1);
	I_1[i] = (int_f - int_i)/(-Alpha_1*costheta);

	// for integral 2
	K = beta_m[i]/(Alpha_2*costheta);
	fdash0 = -beta_p[i]*H1;
	fs = exp(-beta_p[i]/(Alpha_1*costheta)*(u_i1 - u_f1));
	fdashs = -fs*beta_p[i]*u_f1;

	RiCalculateABCD(a2,b2,fs,fdash0, fdashs, den2, A,B,C,D);
	int_f = RiHelper1(A,B,C,D,H2,K, u_f2);
	int_i = RiHelper1(A,B,C,D,H2,K, u_i2);
	I_2[i] = (int_f - int_i)/(-Alpha_2*costheta);

    }
    return A0_1 * I_1 + A0_2 * I_2;
}


