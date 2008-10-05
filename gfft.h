/***************************************************************************
 *   Copyright (C) 2008 by Volodymyr Myrnyy                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 ***************************************************************************/

/** \file
    \brief Definition of main template classes, object factory and singleton
*/


#ifndef __gfft_h
#define __gfft_h

// General Doxygen documentation
#ifdef GFFTDOC
#include "gfftdoc.h"
#endif

#include "loki/Typelist.h"
#include "loki/Factory.h"
#include "loki/Singleton.h"

#include "finit.h"
#include "gfftalg.h"
#include "gfftstdalg.h"
#include "gfftpolicy.h"

namespace DFT {

/// Direction of transform
struct FORWARD {
   enum { ID = 0 };
   template<unsigned N, typename T>
   struct Type : public Forward<N,T> {};

   template<class List, class Separator>
   struct AddSeparator {
      typedef typename Loki::TL::Append<List,Separator>::Result Result;
   };
};

struct BACKWARD {
   enum { ID = 1 };
   template<unsigned N, typename T>
   struct Type : public Backward<N,T> {};

   template<class List, class Separator>
   struct AddSeparator {
      typedef Loki::Typelist<Separator,List> Result;
   };
};

/// Type of decimation used in transform: in-time or in-frequency
struct INTIME {
   enum { ID = 0 };
   template<unsigned N, typename T,
            class Swap,
            class Direction>
   class List {
      typedef InTime<N,T,Direction::Sign> InT;
   public:
      typedef TYPELIST_3(Swap,InT,Direction) Result;
   };
};

struct INFREQ {
   enum { ID = 1 };
   template<unsigned N, typename T,
            class Swap,
            class Direction>
   class List {
      typedef InFreq<N,T,Direction::Sign> InF;
   public:
      typedef TYPELIST_3(InF,Swap,Direction) Result;
   };
};

/// Type of transform
struct COMPLEX {
   enum { ID = 0 };
   template<class Direction, class List, class Separator>
   struct Algorithm {
      typedef List Result;
   };
};

struct REAL {
   enum { ID = 1 };
   template<class Direction, class List, class Separator>
   struct Algorithm {
      typedef typename Direction::
         template AddSeparator<List,Separator>::Result Result;
   };
};


/// Generic Fast Fourier transform in-place
/**
\param P defines transform length, which is 2^P
\param T type of data element
\param Type type of transform: COMPLEX, REAL
\param Decimation in-time or in-frequency: INTIME, INFREQ
\param Direction transform direction: FORWARD, BACKWARD
\param FactoryPolicy policy used to create an object factory. Don't define it explicitely, if unsure
*/
template<unsigned P, class T,
class Type,                          ///< COMPLEX, REAL
class Decimation=INFREQ,              // INTIME, INFREQ
class Direction=FORWARD,         // FORWARD, BACKWARD
class FactoryPolicy=Empty,
unsigned IDN = P>
class GFFT:public FactoryPolicy {
   enum { N = 1<<P };
   typedef GFFTswap<N,T> Swap;

   typedef typename Direction::template Type<N,T> Dir;

   typedef Separate<N,T,Dir::Sign> Sep;

   typedef typename Decimation::template List<N,T,Swap,Dir>::Result TList;

   typedef typename Type::template Algorithm<Direction,TList,Sep>::Result Alg;

   PoliciesHandler<Alg> run;
public:
   enum { ID = IDN };
   static FactoryPolicy* Create() {
      return new GFFT<P,T,Type,Decimation,Direction,FactoryPolicy>();
   }
   void fft(T* data) {
      run.apply(data);
   }
};

/// Generator of a Typelist of GFFTs classes for data lengths from 2^Begin to 2^End
template<unsigned Begin, unsigned End, typename T,
class Type,                          // COMPLEX, REAL
class Decimation=INFREQ,              // INTIME, INFREQ
class Direction=FORWARD,         // FORWARD, BACKWARD
class FactoryPolicy=AbstractFFT<T> >
struct GFFTList {
   typedef Loki::Typelist<GFFT<Begin,T,Type,Decimation,Direction,FactoryPolicy>,
           typename GFFTList<Begin+1,End,T,Type,Decimation,Direction,
                    FactoryPolicy>::Result> Result;
};

template<unsigned End, typename T,
class Type,                          // COMPLEX, REAL
class Decimation,              // INTIME, INFREQ
class Direction,                // FORWARD, BACKWARD
class FactoryPolicy>
struct GFFTList<End,End,T,Type,Decimation,Direction,FactoryPolicy> {
   typedef Loki::NullType Result;
};


template<
class Type,                          // COMPLEX, REAL
class Decimation,              // INTIME, INFREQ
class Direction=FORWARD>
class GFFT_Options { };

/// Singleton for GFFT object factory
template<unsigned Min, unsigned Max, typename T,
class Type,                          // COMPLEX, REAL
class Decimation,              // INTIME, INFREQ
class Direction=FORWARD>
class GFFT_Singleton
: public Loki::SingletonHolder<Loki::Factory<AbstractFFT<T>,unsigned int>,
                               GFFT_Options<Type,Decimation,Direction> > {
   typedef Loki::Factory<AbstractFFT<T>,unsigned int> GFFT_Factory;
   typedef GFFT_Options<Type,Decimation,Direction> Options;
   typedef Loki::SingletonHolder<GFFT_Factory,Options> Base;
   typedef FactoryInit<typename GFFTList<Min,Max,T,Type,Decimation,Direction>::Result> FInit;

   //Protection
   GFFT_Singleton();
public:
   static GFFT_Factory& Instance() {
      FInit::apply(Base::Instance());
      return Base::Instance();
   }
};


}  // namespace DFT

#endif /*__gfft_h*/
