//
// File: GranthamAAPolarityIndex.h
// Created by: Julien Dutheil
// Created on: Tue Apr 21 2005
//

/*
Copyright or � or Copr. CNRS, (November 17, 2004)

This software is a computer program whose purpose is to provide classes
for sequences analysis.

This software is governed by the CeCILL  license under French law and
abiding by the rules of distribution of free software.  You can  use, 
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info". 

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability. 

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or 
data to be ensured and,  more generally, to use and operate it in the 
same conditions as regards security. 

The fact that you are presently reading this means that you have had
knowledge of the CeCILL license and that you accept its terms.
*/

#ifndef _GRANTHAMAAPOLARITYINDEX_H_
#define _GRANTHAMAAPOLARITYINDEX_H_

#include "AlphabetIndex1.h"
#include "ProteicAlphabet.h"

/**
 * @brief Polarity index used in Grantham (1974).
 *
 * Database: AAindex1
 * Entry: GRAR740102
 * H GRAR740102
 * D Polarity (Grantham, 1974)
 * R LIT:2004143b PMID:4843792
 * A Grantham, R.
 * T Amino acid difference formula to help explain protein evolution
 * J Science 185, 862-864 (1974)
 * I    A/L     R/K     N/M     D/F     C/P     Q/S     E/T     G/W     H/Y     I/V
 *      8.1    10.5    11.6    13.0     5.5    10.5    12.3     9.0    10.4     5.2
 *      4.9    11.3     5.7     5.2     8.0     9.2     8.6     5.4     6.2     5.9
 * //
 */
class GranthamAAPolarityIndex: public AlphabetIndex1<double>
{

	private:
		vector<double> _polarity;
		const ProteicAlphabet * _alpha;

	public:
		GranthamAAPolarityIndex()
		{
			_alpha = new ProteicAlphabet();
			_polarity.resize(20);
			_polarity[ 0] =  8.1; //A
			_polarity[ 1] = 10.5; //R
			_polarity[ 2] = 11.6; //N
			_polarity[ 3] = 13.0; //D
			_polarity[ 4] =  5.5; //C
			_polarity[ 5] = 10.5; //Q
			_polarity[ 6] = 12.3; //E
			_polarity[ 7] =  9.0; //G
			_polarity[ 8] = 10.4; //H
			_polarity[ 9] =  5.2; //I
			_polarity[10] =  4.9; //L
			_polarity[11] = 11.3; //K
			_polarity[12] =  5.7; //M
			_polarity[13] =  5.2; //F
			_polarity[14] =  8.0; //P
			_polarity[15] =  9.2; //S
			_polarity[16] =  8.6; //T
			_polarity[17] =  5.4; //W
			_polarity[18] =  6.2; //Y
			_polarity[19] =  5.9; //V
		}

		virtual ~GranthamAAPolarityIndex() { delete _alpha;	}

	public:
		double getIndex(int state) const throw (BadIntException)
		{
			if(state < 0 || state > 19) throw BadIntException(state, "GranthamAAPolarityIndex::getIndex(). Invalid state.", _alpha);
			return _polarity[state];
		}
		
		double getIndex(const string & state) const throw (BadCharException)
		{
			return _polarity[_alpha -> charToInt(state)];
		}

		const Alphabet * getAlphabet() const { return _alpha; }

		Clonable * clone() const { return new GranthamAAPolarityIndex(); }
};

#endif //_GRANTHAMAAPOLARITYINDEX_H_
