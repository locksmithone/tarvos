/**********************************************************************/
/*                                                                    */
/*                            File "stat.c"                           */
/*      Normal and T Distribution Quantile Computation Functions      */
/*                                                                    */
/*  This file is intended for inclusion in an analysis module (such   */
/*  as the "bmeans" module of Figure of 4.7) which defines "double"     */
/*  and "then", and includes "math.h.                                 */
/*                                                                    */
/**********************************************************************/
/* Copyright (C) 2002, 2005, 2006, 2007 Marcos Portnoi, Sergio F. Brito
*
* This file is part of TARVOS Computer Networks Simulator.
*
* TARVOS Computer Networks Simulator is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* TARVOS Computer Networks Simulator is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with TARVOS Computer Networks Simulator.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "simm_globals.h"

/*--------  COMPUTE pth QUANTILE OF THE NORMAL DISTRIBUTION  ---------*/
double Z(double p)
{ /* This function computes the pth upper quantile of the stand-  */
  /* ard normal distribution (i.e., the value of z for which the  */
  /* are under the curve from z to +infinity is equal to p).  'Z' */
  /* is a transliteration of the 'STDZ' function in Appendix C of */
  /* "Principles of Discrete Event Simulation", G. S. Fishman,    */
  /* Wiley, 1978.   The  approximation used initially appeared in */
  /* in  "Approximations for Digital Computers", C. Hastings, Jr.,*/
  /* Princeton U. Press, 1955. */
  double q,z1,n,d; // double sqrt(),log(); // Cannot do this with C++11;
  q=(p>0.5)? (1-p):p; z1=sqrt(-2.0*log(q));
  n=(0.010328*z1+0.802853)*z1+2.515517;
  d=((0.001308*z1+0.189269)*z1+1.43278)*z1+1;
  z1-=n/d; if (p>0.5) z1= -z1;
  return(z1);
}

/*-----------  COMPUTE pth QUANTILE OF THE t DISTRIBUTION  -----------*/
double T(double p, double ndf)
{ /* This function computes the upper pth quantile of the t dis-  */
  /* tribution (the value of t for which the area under the curve */
  /* from t to +infinity is equal to p).  It is a transliteration */
  /* of the 'STUDTP' function given in Appendix C of  "Principles */
  /* of Discrete Event Simulation", G. S. Fishman, Wiley, 1978.   */
  int i; double z1,z2,h[4],x=0.0; // double fabs(); // Cannot do this with C++11;
  z1=fabs(Z(p)); z2=z1*z1;
  h[0]=0.25*z1*(z2+1.0); h[1]=0.010416667*z1*((5.0*z2+16.0)*z2+3.0);
  h[2]=0.002604167*z1*(((3.0*z2+19.0)*z2+17.0)*z2-15.0);
  h[3]=0.000010851*z1*((((79.0*z2+776.0)*z2+1482.0)*z2-1920.0)*z2-945.0);
  for (i=3; i>=0; i--) x=(x+h[i])/(double)ndf;
  z1+=x; if (p>0.5) z1=-z1;
  return(z1);
}
