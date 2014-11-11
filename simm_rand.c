/*
*
*                           File "rand.c" 
*                     Random Variate Generation 
*
*                                         (c) 1987  M. H. MacDougall
* 
*  Note:  modifications in 'stream' function July/2005 Marcos Portnoi
*
*         randJain:  alternative function for generating uniform random numbers
*           included 20.Dec.2005 Marcos Portnoi
*
*         pareto random variate generator (from Kenneth J. Christensen's page) included
*           20.Dec.2005 Marcos Portnoi
*
* Copyright (C) 2002, 2005, 2006, 2007 Marcos Portnoi, Sergio F. Brito
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

#define CPU 0           /* CPU type:  8086 or 68000 or 0 (random C
													library function)  */

#define then

#define A 16807L           /* multiplier (7**5) for 'ranf' */
#define M 2147483647L      /* modulus (2**31-1) for 'ranf' */


static long In[16]= {0L,   /* seeds for streams 1 thru 15  */
  1973272912L,  747177549L,   20464843L,  640830765L, 1098742207L,
    78126602L,   84743774L,  831312807L,  124667236L, 1172177002L,
  1124933064L, 1223960546L, 1878892440L, 1449793615L,  553303732L};

static int strm=1;         /* index of current stream */

/* FILE	*istf, *ostf; */
static	char	state1[256],state2[256];

#if CPU==8086
/*-------------  UNIFORM [0, 1] RANDOM NUMBER GENERATOR  -------------*/
/*                                                                    */
/* This implementation is for Intel 8086/8 and 80286/386 CPUs using   */
/* C compilers with 16-bit short integers and 32-bit long integers.   */
/*                                                                    */
/*--------------------------------------------------------------------*/
double ranf()
  {
    short *p,*q,k; long Hi,Lo;
    /* generate product using double precision simulation  (comments  */
    /* refer to In's lower 16 bits as "L", its upper 16 bits as "H")  */
/*here*/
	p=(short *) & In[strm]; Hi= *(p+1)*A;                 /* 16807*H->Hi */
    *(p+1)=0; Lo=In[strm]*A;                           /* 16807*L->Lo */
    p=(short *)&Lo; Hi+= *(p+1);    /* add high-order bits of Lo to Hi */
    q=(short *)&Hi;                       /* low-order bits of Hi->LO */
    *(p+1)= *q&0X7FFF;                               /* clear sign bit */
    k= *(q+1)<<1; if (*q&0X8000) then k++;         /* Hi bits 31-45->K */
    /* form Z + K [- M] (where Z=Lo): presubtract M to avoid overflow */
    Lo-=M; Lo+=k; if (Lo<0) then Lo+=M;
    In[strm]=Lo;
    return((double)Lo*4.656612875E-10);             /* Lo x 1/(2**31-1) */
  }
#endif

#if CPU==68000
/*-------------  UNIFORM [0, 1] RANDOM NUMBER GENERATOR  -------------*/
/*                                                                    */
/* This implementation is for Motorola 680x0 CPUs using C compilers   */
/* with 16-bit short integers and 32-bit long integers.               */
/*                                                                    */
/*--------------------------------------------------------------------*/
double ranf()
  {
    short *p,*q,k; long Hi,Lo;
    /* generate product using double precision simulation  (comments  */
    /* refer to In's lower 16 bits as "L", its upper 16 bits as "H")  */
    p=(short *)&In[strm]; Hi= *(p)*A;                   /* 16807*H->Hi */
    *(p)=0; Lo=In[strm]*A;                             /* 16807*L->Lo */
    p=(short *)&Lo; Hi+= *(p);      /* add high-order bits of Lo to Hi */
    q=(short *)&Hi;                       /* low-order bits of Hi->LO */
    *(p)= *(q+1)&0X7FFF;                             /* clear sign bit */
    k= *(q)<<1; if (*(q+1)&0X8000) then k++;       /* Hi bits 31-45->K */
    /* form Z + K [- M] (where Z=Lo): presubtract M to avoid overflow */
    Lo-=M; Lo+=k; if (Lo<0) then Lo+=M;
    In[strm]=Lo;
    return((double)Lo*4.656612875E-10);             /* Lo x 1/(2**31-1) */
  }
#endif

#if CPU==0
/*-------------  UNIFORM [0, 1] RANDOM NUMBER GENERATOR  -------------*/
/*                                                                    */
/* This implementation uses the C library function "random" available */
/* on UNIX machines.												  */
/*                                                                    */
/*--------------------------------------------------------------------*/
/* Esta funcao tem que ser trabalhada com cuidado pois ela nao deve voltar
   valor igual a 0 pois tem geradores que trabalham com log e isto
	 gerara um valor incorreto. Para o UNIX isto precisa ser verificado. */
double ranf() {
	/* return(random()/2.147483647E9); Para o  UNIX */
	double num_alea;
	num_alea = rand ();
	if ( num_alea < 0.) {
		printf ("\nError - ranf - random number less than zero");
		exit(0);
	}
	while (num_alea == 0.) {															/* Para evitar que ranf gere 0*/
		num_alea = rand ();																/* Para o Visual C */
	}
	return ( (float) num_alea / RAND_MAX );
  }
#endif

/*--------------------  SELECT GENERATOR STREAM  ---------------------*/

/* Esta funcao foi colocada como comentario pois a funcao initstate nao
   foi achada no Visual C - quando for para o UNIX e bom verificar
   o que estiver entre [] era comentario */
/*
stream(n)
  int n;
    {
#if CPU==0
	  char	*initstate();
	  int	i;
#endif
	                                //[set stream for 1<=n<=15, return stream for n=0]
      if ((n<0)||(n>15)) then {
		error(0,"stream Argument Error");
      }
      if (n) then {
		strm=n;
#if CPU==0
	  	                                      //[ srandom(strm); ]
		for (i=0;i<=255;i++) state1[i]='\0';
		initstate(strm,state1,256);
#endif
	  }
      return(strm);
    }

*/

/*  Coloca-se aqui a funcao stream () que esta especificada no proprio livro do
    MacDougall para rodar a chamada da semente para maquinas 8086 */

#if CPU==68000 || CPU==8086

int stream (int n)
{
	/* set stream for 1<=n<=15, returns stream for n= 0 */

	if ((n<0) || (n>15)) then error (0,"Stream Argument Error");
	if (n) strm=n;
	return (strm);
}
#endif

//This stream function definition uses de C library 'srand' function to seed the random number generator
//  (included mainly for backward compatibility with SMPL)
//  Use 'x=stream(0)' to return the actual stream number
//  Use 'stream(n)' to set the stream (or seed) to n (n can be any integer)
//included July/2005 by Marcos Portnoi
#if CPU==0

int stream (int n) {
	if (n) {
		strm=n;
		srand(strm);
		//let's have this functionality below deactivated for now
		//randJain(strm);  //seeds Jain's random number generator too with the same seed
	}
	return (strm);
}
#endif

/*--------------------------  SET/GET SEED  --------------------------*/
long seed(long Ik, int n) { /* set seed of stream n for Ik>0, return current seed for Ik=0  */
  if ((n<1)||(n>15)) then error(0,"seed Argument Error");
  if (Ik>0L) then  In[n]=Ik;
  return(In[n]);
}

/*------------  UNIFORM [a, b] RANDOM VARIATE GENERATOR  -------------*/
double uniform(double a, double b) { 
	/* 'uniform' returns a pseudo-random variate from a uniform     */
    /* distribution with lower bound a and upper bound b.           */
    if (a>b) then error(0,"uniform Argument Error: a > b");
    return(a+(b-a)*ranf());
}

/*--------------------  RANDOM INTEGER GENERATOR  --------------------*/
int irandom(int i,int n)
{ /* 'random' returns an integer equiprobably selected from the   */
  /* set of integers i, i+1, i+2, . . , n.                        */
  double num_alea;
	if (i>n) then error(0,"random Argument Error: i > n");
 	num_alea = ranf();
	if ( num_alea == 1. )
	{
		return (n);
	}
	n-=i;
	n= (int)((n+1.0)*num_alea);
  return(i+n);
}

/*--------------  EXPONENTIAL RANDOM VARIATE GENERATOR  --------------*/
double expntl(double x)
{ /* 'expntl' returns a psuedo-random variate from a negative     */
  /* exponential distribution with mean x.                        */
  return(-x*log(ranf()));
}

/*----------------  ERLANG RANDOM VARIATE GENERATOR  -----------------*/
double erlang(double x, double s)
{ /* 'erlang' returns a psuedo-random variate from an erlang      */
  /* distribution with mean x and standard deviation s.           */
  int i,k; double z;
  if (s>x) then error(0,"erlang Argument Error: s > x");
  z=x/s; k=(int) (z*z);
  z=1.0; for (i=0; i<k; i++) z*=ranf();
  return(-(x/k)*log(z));
}

/*-----------  HYPEREXPONENTIAL RANDOM VARIATE GENERATION  -----------*/
double hyperx(double x, double s)
{ /* 'hyperx' returns a psuedo-random variate from Morse's two-   */
  /* stage hyperexponential distribution with mean x and standard */
  /* deviation s, s>x.  */
  double cv,z,p;
  if (s<=x) then error(0,"hyperx Argument Error: s not > x");
  cv=s/x; z=cv*cv; p=0.5*(1.0-sqrt((z-1.0)/(z+1.0)));
  z=(ranf()>p)? (x/(1.0-p)):(x/p);
  return(-0.5*z*log(ranf()));
}

/*-----------------  NORMAL RANDOM VARIATE GENERATOR  ----------------*/
double normal(double x, double s)
{ /* 'normal' returns a psuedo-random variate from a normal dis-  */
  /* tribution with mean x and standard deviation s.              */
  double v1,v2,w,z1; static double z2=0.0;
  if (z2!=0.0)
    then {z1=z2; z2=0.0;}  /* use value from previous call */
  else
  {
		do
			{v1=2.0*ranf()-1.0; v2=2.0*ranf()-1.0; w=v1*v1+v2*v2;}
		while (w>=1.0);
		w=sqrt((-2.0*log(w))/w); z1=v1*w; z2=v2*w;
  }
  return(x+z1*s);
}

/*
*  PARETO RANDOM VARIATE GENERATOR
*
*  Function to generate Pareto distributed RVs using
*    - Input:  a and k
*    - Output: Returns with Pareto RV
*    - From Kenneth J. Christensen's page 20.Dec.2005
*   http://www.csee.usf.edu/~christen/tools/toolpage.html
*   Email:  christen@csee.usf.edu 
*
*/
double pareto(double a, double k) {
  double z;     // Uniform random number from 0 to 1
  double rv;    // RV to be returned

  // Pull a uniform RV (0 < z < 1)
  do {
    z = randJain(0);
  } while ((z == 0) || (z == 1));

  // Generate Pareto rv using the inversion method
  rv = k / pow(z, (1.0 / a));

  return(rv);
}

/*
*  ALTERNATIVE UNIFORM RANDOM NUMBER GENERATOR
*	
* Multiplicative LCG for generating uniform(0.0, 1.0) random numbers
*   - x_n = 7^5*x_(n-1)mod(2^31 - 1)
*   - With x seeded to 1 the 10000th x value should be 1043618065
*   - From R. Jain, "The Art of Computer Systems Performance Analysis,"
*     John Wiley & Sons, 1991. (Page 443, Figure 26.2)
*
* It is necessary to seed the initial value for the generator!  Call
* randJain(seed) with a non-zero seed argument value to seed the generator, then
* call randJain(0) to obtain a random uniform number.  If this is not done, an initial
* value of 1 to the seed will be assumed.
*
*/
double randJain(int seed) {
  const long  a =      16807;  // Multiplier
  const long  m = 2147483647;  // Modulus
  const long  q =     127773;  // m div a
  const long  r =       2836;  // m mod a
  static long x = 1;           // Random int value (default initial seed = 1)
  long        x_div_q;         // x divided by q
  long        x_mod_q;         // x modulo q
  long        x_new;           // New x value

  // Set the seed if argument is non-zero and then return zero
  if (seed > 0) {
    x = seed;
    return(0.0);
  }

  // RNG using integer arithmetic
  x_div_q = x / q;
  x_mod_q = x % q;
  x_new = (a * x_mod_q) - (r * x_div_q);
  if (x_new > 0)
    x = x_new;
  else
    x = x_new + m;

  // Return a random value between 0.0 and 1.0
  return((double) x / m);
}

/*
#if CPU==0

save_random_state()
 {
	char	*ps,*setstate();
	int		i;

	ps=setstate(state2);

	for (i=0;i<=255;i++) {
		fprintf(ostf,"%d ",(int)ps[i]);
		if (!(i%10)) fprintf(ostf,"\n");
	}
	fprintf(ostf,"\n");
 }

 restore_random_state()
 {
	char	*setstate();
	int 	i,aint;

	for (i=0;i<=255;i++) {
		fscanf(istf,"%d",&aint);
		state1[i] = (char)aint;
	}

	setstate(state1);
 }
#endif
*/
