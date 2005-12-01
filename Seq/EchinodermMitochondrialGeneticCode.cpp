//
// File: EchinodermMitochondrialGeneticCode.h
// Created by: Eric Bazin
// Created on: 14 11:31:27 CET 2005

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

#include "EchinodermMitochondrialGeneticCode.h"
#include "EchinodermMitochondrialCodonAlphabet.h"
#include "ProteicAlphabet.h"
#include <iostream>

EchinodermMitochondrialGeneticCode::EchinodermMitochondrialGeneticCode(const NucleicAlphabet * alpha) : GeneticCode() 
{
	codonAlphabet   = new EchinodermMitochondrialCodonAlphabet(alpha);
	proteicAlphabet = new ProteicAlphabet();
}


EchinodermMitochondrialGeneticCode::~EchinodermMitochondrialGeneticCode() {
	delete codonAlphabet;
	delete proteicAlphabet;	
}

int    EchinodermMitochondrialGeneticCode::translate(int state) const throw (BadIntException) {
	vector<int> positions = codonAlphabet -> getPositions(state);
	switch(positions[0]) {
		//First position:
		case 0 : //A
		switch(positions[1]) {
			//Second position:
			case 0 : //AA
			switch(positions[2]) {
				//Third position:
				case 2 :          		return proteicAlphabet -> charToInt("K"); //Lysine
				case 0 : case 1 : case 3 :      return proteicAlphabet -> charToInt("N"); //Asparagine
			}
			case 1 : //AC
			                               return proteicAlphabet -> charToInt("T"); //Threonine
			case 2 : //AG
			switch(positions[2]) {
				//Third position:
				case 0 : case 1 : case 2 : case 3 :      return proteicAlphabet -> charToInt("S"); //Serine
			}
			case 3 : //AT
			switch(positions[2]) {
				//Third position:
				case 2:                    return proteicAlphabet -> charToInt("M"); //Methionine
				case 0 : case 1 : case 3 : return proteicAlphabet -> charToInt("I"); //Isoleucine
			}
		}
		case 1 : //C
		switch(positions[1]) {
			//Second position:
			case 0 : //CA
			switch(positions[2]) {
				//Third position:
				case 0 : case 2 :          return proteicAlphabet -> charToInt("Q"); //Glutamine
				case 1 : case 3 :          return proteicAlphabet -> charToInt("H"); //Histidine
			}
			case 1 : //CC
			                               return proteicAlphabet -> charToInt("P"); //Proline
			case 2 : //CG
			                               return proteicAlphabet -> charToInt("R"); //Arginine
			case 3 : //CT
			                               return proteicAlphabet -> charToInt("L"); //Leucine
		}
		case 2 : //G
		switch(positions[1]) {
			//Second position:
			case 0 : //GA
			switch(positions[2]) {
				//Third position:
				case 0 : case 2 :          return proteicAlphabet -> charToInt("E"); //Glutamic acid
				case 1 : case 3 :          return proteicAlphabet -> charToInt("D"); //Aspartic acid
			}
			case 1 : //GC
			                               return proteicAlphabet -> charToInt("A"); //Alanine
			case 2 : //GG
			                               return proteicAlphabet -> charToInt("G"); //Glycine
			case 3 : //GT
			                               return proteicAlphabet -> charToInt("V"); //Valine
		}
		case 3 : //T(U)
		switch(positions[1]) {
			//Second position:
			case 0 : //TA
			switch(positions[2]) {
				//Third position:
				case 0 :                   throw StopCodonException("", "TAA"); //Stop codon
				case 2 :                   throw StopCodonException("", "TAG"); //Stop codon
				case 1 : case 3 :          return proteicAlphabet -> charToInt("Y"); //Tyrosine
			}
			case 1 : //TC
			                               return proteicAlphabet -> charToInt("S"); //Serine
			case 2 : //TG
			switch(positions[2]) {
				//Third position:
				case 0 : case 2 :          return proteicAlphabet -> charToInt("W"); //Tryptophane
				case 1 : case 3 :          return proteicAlphabet -> charToInt("C"); //Cysteine
			}
			case 3 : //TT
			switch(positions[2]) {
				//Third position:
				case 0 : case 2 :          return proteicAlphabet -> charToInt("L"); //Leucine
				case 1 : case 3 :          return proteicAlphabet -> charToInt("F"); //Phenylalanine
			}
		}
	}
	throw BadIntException(state, "EchinodermMitochondrialGeneticCode::translate", codonAlphabet);
}

string EchinodermMitochondrialGeneticCode::translate(const string & state) const throw (BadCharException) {
	return proteicAlphabet -> intToChar(translate(codonAlphabet -> charToInt(state)));
}