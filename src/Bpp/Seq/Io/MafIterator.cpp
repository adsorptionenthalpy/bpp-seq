//
// File: MafIterator.cpp
// Authors: Julien Dutheil
// Created: Tue Sep 07 2010
//

/*
Copyright or © or Copr. Bio++ Development Team, (2010)

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

#include "MafIterator.h"
#include "../SequenceWithQuality.h"
#include "../SequenceWithAnnotationTools.h"
#include "../Alphabet/AlphabetTools.h"
#include "../Container/VectorSiteContainer.h"
#include "../SiteTools.h"

using namespace bpp;

//From the STL:
#include <string>

using namespace std;

MafSequence* MafSequence::subSequence(unsigned int startAt, unsigned int length) const
{
  string subseq = toString().substr(startAt, length);
  unsigned int begin = begin_;
  if (hasCoordinates_) {
    for (unsigned int i = 0; i < startAt; ++i) {
      if (! getAlphabet()->isGap(operator[](i))) begin++;
    }
  }
  MafSequence* newSeq = new MafSequence(getName(), subseq, begin, strand_, srcSize_);
  vector<string> anno = getAnnotationTypes();
  for (size_t i = 0; i < anno.size(); ++i) {
    newSeq->addAnnotation(getAnnotation(anno[i]).getPartAnnotation(startAt, length));
  }
  return newSeq;
}

MafBlock* SequenceFilterMafIterator::nextBlock() throw (Exception)
{
  currentBlock_ = iterator_->nextBlock();
  while (currentBlock_) {
    map<string, unsigned int> counts;
    for (size_t i = currentBlock_->getNumberOfSequences(); i > 0; --i) {
      string species = currentBlock_->getSequence(i-1).getSpecies(); 
      if (!VectorTools::contains(species_, species)) {
        if (logstream_) {
          (*logstream_ << "SEQUENCE FILTER: remove sequence '" << species << "' from current block.").endLine();
        }
        currentBlock_->getAlignment().deleteSequence(i - 1);
      } else {
        counts[species]++;
      }
    }
    bool test = currentBlock_->getNumberOfSequences() == 0;
    //Avoid a memory leak:
    if (test) {
      delete currentBlock_;
      if (logstream_) {
        (*logstream_ << "SEQUENCE FILTER: block is now empty. Try to get the next one.").endLine();
      }
    } else {
      test = strict_ && (counts.size() != species_.size());
      if (test) {
        delete currentBlock_;
        if (logstream_) {
          (*logstream_ << "SEQUENCE FILTER: block does not contain all species and will be ignored. Try to get the next one.").endLine();
        }
      } else {
        if (rmDuplicates_) {
          test = false;
          map<string, unsigned int>::iterator it;
          for (it = counts.begin(); it != counts.end() && !(test = it->second > 1); it++) {}
          if (test) {
            delete currentBlock_;
            if (logstream_) {
              (*logstream_ << "SEQUENCE FILTER: block has two sequences for species '" << it->first << "' and will be ignored. Try to get the next one.").endLine();
            }
          } else {
            return currentBlock_;
          }
        } else {
          return currentBlock_;
        }
      }
    }

    //Look for the next block:
    currentBlock_ = iterator_->nextBlock();
  }
  
  return currentBlock_;
}

MafBlock* BlockMergerMafIterator::nextBlock() throw (Exception)
{
  if (!incomingBlock_) return 0;
  currentBlock_  = incomingBlock_;
  incomingBlock_ = iterator_->nextBlock();
  while (incomingBlock_) {
    unsigned int globalSpace = 0;
    for (unsigned int i = 0; i < species_.size(); ++i) {
      try {
        const MafSequence* seq1 = &currentBlock_->getSequence(species_[i]); 
        const MafSequence* seq2 = &incomingBlock_->getSequence(species_[i]);
        if (!seq1->hasCoordinates() || !seq2->hasCoordinates())
          throw Exception("BlockMergerMafIterator::nextBlock. Species '" + species_[i] + "' is missing coordinates in at least one block.");
        unsigned int space = seq1->start() - seq2->stop() - 1;
        if (space > maxDist_)
          return currentBlock_;
        if (i == 0)
          globalSpace = space;
        else {
          if (space != globalSpace)
            return currentBlock_;
        }
        if (seq1->getChromosome() != seq2->getChromosome()
         || VectorTools::contains(ignoreChrs_, seq1->getChromosome())
         || VectorTools::contains(ignoreChrs_, seq2->getChromosome())
         || seq1->getStrand() != seq2->getStrand())
        {
          //There is a syntheny break in this sequence, so we do not merge the blocks.
          return currentBlock_;
        }
      } catch (SequenceNotFoundException& snfe) {
        //At least one block does not contain the sequence.
        //We don't merge the blocks:
        return currentBlock_;
      }
    }
    //We merge the two blocks:
    if (logstream_) {
      (*logstream_ << "BLOCK MERGER: merging two consecutive blocks.").endLine();
    }
    vector<string> names1 = currentBlock_->getAlignment().getSequencesNames();
    vector<string> names2 = incomingBlock_->getAlignment().getSequencesNames();
    vector<string> allNames = VectorTools::vectorUnion(names1, names2);
    //We need to create a new MafBlock:
    MafBlock* mergedBlock = new MafBlock();
    //We average the score and pass values:
    unsigned int p1 = currentBlock_->getPass();
    unsigned int p2 = incomingBlock_->getPass();
    if (p1 == p2) mergedBlock->setPass(p1);
    double s1 = currentBlock_->getScore();
    double n1 = static_cast<double>(currentBlock_->getNumberOfSites());
    double s2 = incomingBlock_->getScore();
    double n2 = static_cast<double>(incomingBlock_->getNumberOfSites());
    mergedBlock->setScore((s1 * n1 + s2 * n2) / (n1 + n2));

    //Now fill the new block:
    for (size_t i = 0; i < allNames.size(); ++i) {
      auto_ptr<MafSequence> seq;
      try {
        seq.reset(new MafSequence(currentBlock_->getSequence(allNames[i])));

        //Check is there is a second sequence:
        try {
          const MafSequence* tmp = &incomingBlock_->getSequence(allNames[i]);
          string ref1 = seq->getDescription(), ref2 = tmp->getDescription();
          //Add spacer if needed:
          if (globalSpace > 0) {
            if (logstream_) {
              (*logstream_ << "BLOCK MERGER: a spacer of size " << globalSpace <<" is inserted in sequence " << allNames[i] << ".").endLine();
            }
            seq->append(vector<int>(globalSpace, AlphabetTools::DNA_ALPHABET.getUnknownCharacterCode()));
          }
          seq->merge(*tmp);
          seq->setSrcSize(0);
          if (logstream_) {
            (*logstream_ << "BLOCK MERGER: merging " << ref1 << " with " << ref2 << " into " << seq->getDescription()).endLine();
          }
        } catch (SequenceNotFoundException& snfe2) {
          //There was a first sequence, we just extend it:
          string ref1 = seq->getDescription();
          seq->setToSizeR(incomingBlock_->getNumberOfSites() + globalSpace);
          if (logstream_) {
            (*logstream_ << "BLOCK MERGER: extending " << ref1 << " with " << incomingBlock_->getNumberOfSites() << " gaps on the right.").endLine();
          }
        }
      } catch (SequenceNotFoundException& snfe1) {
        //There must be a second sequence then:
        seq.reset(new MafSequence(incomingBlock_->getSequence(allNames[i])));
        string ref2 = seq->getDescription();
        seq->setToSizeL(currentBlock_->getNumberOfSites() + globalSpace);
        if (logstream_) {
          (*logstream_ << "BLOCK MERGER: adding " << ref2 << " and extend it with " << currentBlock_->getNumberOfSites() << " gaps on the left.").endLine();
        }
      }
      mergedBlock->addSequence(*seq);
    }
    //Cleaning stuff:
    delete currentBlock_;
    delete incomingBlock_;
    currentBlock_ = mergedBlock;
    //We check if we can also merge the next block:
    incomingBlock_ = iterator_->nextBlock();
  }
  return currentBlock_;
}

MafBlock* FullGapFilterMafIterator::nextBlock() throw (Exception)
{
  MafBlock* block = iterator_->nextBlock();
  if (!block) return 0;

  //We create a copy of the ingroup alignement for better efficiency:
  VectorSiteContainer vsc(&AlphabetTools::DNA_ALPHABET);
  for (size_t i = 0; i < species_.size(); ++i) {
    vsc.addSequence(block->getSequence(i));
  }
  //Now check the positions that are only made of gaps:
  if (verbose_) {
    ApplicationTools::message->endLine();
    ApplicationTools::displayTask("Cleaning block for gap sites", true);
  }
  unsigned int n = block->getNumberOfSites();
  unsigned int count = n;
  unsigned int i = n;
  while (i > 0) {
    if (verbose_)
      ApplicationTools::displayGauge(n - i, n - 1, '=');
    const Site* site = &vsc.getSite(i - 1);
    if (SiteTools::isGapOnly(*site)) {
      unsigned int end = i;
      while (SiteTools::isGapOnly(*site) && i > 0) {
        site = &vsc.getSite(i - 1);
        --i;
      }
      block->getAlignment().deleteSites(i, end - i);
      count -= (end - i);
    } else {
      --i;
    }
  }
  if (verbose_)
    ApplicationTools::displayTaskDone();
  
  //Correct coordinates:
  if (count > 0) {
    for (i = 0; i < block->getNumberOfSequences(); ++i) {
      const MafSequence* seq = &block->getSequence(i);
      if (!VectorTools::contains(species_, seq->getSpecies())) {
        block->removeCoordinatesFromSequence(i);
      }
    }
  }
  if (logstream_) {
    (*logstream_ << "FULL GAP CLEANER: " << count << " positions have been removed.").endLine();
  }
  return block;
}

MafBlock* AlignmentFilterMafIterator::nextBlock() throw (Exception)
{
  if (blockBuffer_.size() == 0) {
    //Else there is no more block in the buffer, we need parse more:
    do {
      MafBlock* block = iterator_->nextBlock();
      if (!block) return 0; //No more block.
    
      //Parse block.
      int gap = AlphabetTools::DNA_ALPHABET.getGapCharacterCode();
      int unk = AlphabetTools::DNA_ALPHABET.getUnknownCharacterCode();
      size_t nr = species_.size();
      vector< vector<int> > aln(nr);
      for (size_t i = 0; i < nr; ++i) {
        aln[i] = block->getSequenceForSpecies(species_[i]).getContent();
      }
      size_t nc = block->getNumberOfSites();
      //First we create a mask:
      vector<unsigned int> pos;
      vector<bool> col(nr);
      //Reset window:
      window_.clear();
      //Init window:
      size_t i;
      for (i = 0; i < windowSize_; ++i) {
        for (size_t j = 0; j < nr; ++j) {
          col[j] = (aln[j][i] == gap|| aln[j][i] == unk);
        }
        window_.push_back(col);
      }
      //Slide window:
      if (verbose_) {
        ApplicationTools::message->endLine();
        ApplicationTools::displayTask("Sliding window for alignment filter", true);
      }
      while (i < nc) {
        if (verbose_)
          ApplicationTools::displayGauge(i - windowSize_, nc - windowSize_ - 1, '>');
        //Evaluate current window:
        unsigned int sum = 0;
        for (size_t u = 0; u < window_.size(); ++u)
          for (size_t v = 0; v < window_[u].size(); ++v)
            if (window_[u][v]) sum++;
        if (sum > maxGap_) {
          if (pos.size() == 0) {
            pos.push_back(i - windowSize_);
            pos.push_back(i);
          } else {
            if (i - windowSize_ < pos[pos.size() - 1]) {
              pos[pos.size() - 1] = i; //Windows are overlapping and we extend previous region
            } else { //This is a new region
              pos.push_back(i - windowSize_);
              pos.push_back(i);
            }
          }
        }
      
        //Move forward:
        for (unsigned int k = 0; k < step_; ++k) {
          for (size_t j = 0; j < nr; ++j) {
            col[j] = (aln[j][i] == gap || aln[j][i] == unk);
          }
          window_.push_back(col);
          window_.pop_front();
          ++i;
        }
      }

      //Evaluate last window:
      unsigned int sum = 0;
      for (size_t u = 0; u < window_.size(); ++u)
        for (size_t v = 0; v < window_[u].size(); ++v)
          if (window_[u][v]) sum++;
      if (sum > maxGap_) {
        if (pos.size() == 0) {
          pos.push_back(i - windowSize_);
          pos.push_back(i);
        } else {
          if (i - windowSize_ <= pos[pos.size() - 1]) {
            pos[pos.size()] = i; //Windows are overlapping and we extend previous region
          } else { //This is a new region
            pos.push_back(i - windowSize_);
            pos.push_back(i);
          }
        }
      } 
      if (verbose_)
        ApplicationTools::displayTaskDone();
    
      //Now we remove regions with two many gaps, using a sliding window:
      if (pos.size() == 0) {
        blockBuffer_.push_back(block);
        if (logstream_) {
          (*logstream_ << "ALN CLEANER: block is clean and kept as is.").endLine();
        }
      } else if (pos.size() == 2 && pos[0] == 0 && pos[pos.size() - 1] == block->getNumberOfSites()) {
        //Everything is removed:
        if (logstream_) {
          (*logstream_ << "ALN CLEANER: block was entirely removed. Tried to get the next one.").endLine();
        }
      } else {
        if (logstream_) {
          (*logstream_ << "ALN CLEANER: block with size "<< block->getNumberOfSites() << " will be split into " << (pos.size() / 2 + 1) << " blocks.").endLine();
        }
        if (verbose_) {
          ApplicationTools::message->endLine();
          ApplicationTools::displayTask("Spliting block", true);
        }
        for (i = 0; i < pos.size(); i+=2) {
          if (verbose_)
            ApplicationTools::displayGauge(i, pos.size() - 2, '=');
          if (logstream_) {
            (*logstream_ << "ALN CLEANER: removing region (" << pos[i] << ", " << pos[i+1] << ") from block.").endLine();
          }
          if (pos[i] > 0) {
            MafBlock* newBlock = new MafBlock();
            newBlock->setScore(block->getScore());
            newBlock->setPass(block->getPass());
            for (unsigned int j = 0; j < block->getNumberOfSequences(); ++j) {
              MafSequence* subseq;
              if (i == 0) {
                subseq = block->getSequence(j).subSequence(0, pos[i]);
              } else {
                subseq = block->getSequence(j).subSequence(pos[i - 1], pos[i] - pos[i - 1]);
              }
              newBlock->addSequence(*subseq);
              delete subseq;
            }
            blockBuffer_.push_back(newBlock);
          }
        
          if (keepTrashedBlocks_) {
            MafBlock* outBlock = new MafBlock();
            outBlock->setScore(block->getScore());
            outBlock->setPass(block->getPass());
            for (unsigned int j = 0; j < block->getNumberOfSequences(); ++j) {
              MafSequence* outseq = block->getSequence(j).subSequence(pos[i], pos[i + 1] - pos[i]);
              outBlock->addSequence(*outseq);
              delete outseq;
            } 
            trashBuffer_.push_back(outBlock);
          }
        }
        //Add last block:
        if (pos[pos.size() - 1] < block->getNumberOfSites()) {
          MafBlock* newBlock = new MafBlock();
          newBlock->setScore(block->getScore());
          newBlock->setPass(block->getPass());
          for (unsigned int j = 0; j < block->getNumberOfSequences(); ++j) {
            MafSequence* subseq;
            subseq = block->getSequence(j).subSequence(pos[pos.size() - 1], block->getNumberOfSites() - pos[pos.size() - 1]);
            newBlock->addSequence(*subseq);
            delete subseq;
          }
          blockBuffer_.push_back(newBlock);
        }
        if (verbose_)
          ApplicationTools::displayTaskDone();

        delete block;
      }
    } while (trashBuffer_.size() == 0);
  }

  MafBlock* block = blockBuffer_.front();
  blockBuffer_.pop_front();
  return block;
}

MafBlock* MaskFilterMafIterator::nextBlock() throw (Exception)
{
  if (blockBuffer_.size() == 0) {
    do {
      //Else there is no more block in the buffer, we need parse more:
      MafBlock* block = iterator_->nextBlock();
      if (!block) return 0; //No more block.
    
      //Parse block.
      vector< vector<bool> > aln;
      for (size_t i = 0; i < species_.size(); ++i) {
        const MafSequence* seq = &block->getSequenceForSpecies(species_[i]);
        if (seq->hasAnnotation(SequenceMask::MASK)) {
          aln.push_back(dynamic_cast<const SequenceMask&>(seq->getAnnotation(SequenceMask::MASK)).getMask());
        }
      }
      size_t nr = aln.size();
      size_t nc = block->getNumberOfSites();
      //First we create a mask:
      vector<unsigned int> pos;
      vector<bool> col(nr);
      //Reset window:
      window_.clear();
      //Init window:
      size_t i;
      for (i = 0; i < windowSize_; ++i) {
        for (size_t j = 0; j < nr; ++j) {
          col[j] = aln[j][i];
        }
        window_.push_back(col);
      }
      //Slide window:
      if (verbose_) {
        ApplicationTools::message->endLine();
        ApplicationTools::displayTask("Sliding window for mask filter", true);
      }
      while (i < nc) {
        if (verbose_)
          ApplicationTools::displayGauge(i - windowSize_, nc - windowSize_ - 1, '>');
        //Evaluate current window:
        unsigned int sum = 0;
        for (size_t u = 0; u < window_.size(); ++u)
          for (size_t v = 0; v < window_[u].size(); ++v)
            if (window_[u][v]) sum++;
        if (sum > maxMasked_) {
          if (pos.size() == 0) {
            pos.push_back(i - windowSize_);
            pos.push_back(i);
          } else {
            if (i - windowSize_ <= pos[pos.size() - 1]) {
              pos[pos.size() - 1] = i; //Windows are overlapping and we extend previous region
            } else { //This is a new region
              pos.push_back(i - windowSize_);
              pos.push_back(i);
            }
          }
        }
      
        //Move forward:
        for (unsigned int k = 0; k < step_; ++k) {
          for (size_t j = 0; j < nr; ++j) {
            col[j] = aln[j][i];
          }  
          window_.push_back(col);
          window_.pop_front();
          ++i;
        }
      }

      //Evaluate last window:
      unsigned int sum = 0;
      for (size_t u = 0; u < window_.size(); ++u)
        for (size_t v = 0; v < window_[u].size(); ++v)
          if (window_[u][v]) sum++;
      if (sum > maxMasked_) {
        if (pos.size() == 0) {
          pos.push_back(i - windowSize_);
          pos.push_back(i);
        } else {
          if (i - windowSize_ < pos[pos.size() - 1]) {
            pos[pos.size()] = i; //Windows are overlapping and we extend previous region
          } else { //This is a new region
            pos.push_back(i - windowSize_);
            pos.push_back(i);
          }
        }
      }
      if (verbose_)
        ApplicationTools::displayTaskDone();
    
      //Now we remove regions with two many gaps, using a sliding window:
      if (pos.size() == 0) {
        blockBuffer_.push_back(block);
        if (logstream_) {
          (*logstream_ << "MASK CLEANER: block is clean and kept as is.").endLine();
        }
      } else if (pos.size() == 2 && pos[0] == 0 && pos[pos.size() - 1] == block->getNumberOfSites()) {
        //Everything is removed:
        if (logstream_) {
          (*logstream_ << "MASK CLEANER: block was entirely removed. Tried to get the next one.").endLine();
        }
      } else {
        if (logstream_) {
          (*logstream_ << "MASK CLEANER: block with size "<< block->getNumberOfSites() << " will be split into " << (pos.size() / 2 + 1) << " blocks.").endLine();
        }
        if (verbose_) {
          ApplicationTools::message->endLine();
          ApplicationTools::displayTask("Spliting block", true);
        }
        for (i = 0; i < pos.size(); i+=2) {
          if (verbose_)
            ApplicationTools::displayGauge(i, pos.size() - 2, '=');
          if (logstream_) {
            (*logstream_ << "MASK CLEANER: removing region (" << pos[i] << ", " << pos[i+1] << ") from block.").endLine();
          }
          if (pos[i] > 0) {
            MafBlock* newBlock = new MafBlock();
            newBlock->setScore(block->getScore());
            newBlock->setPass(block->getPass());
            for (unsigned int j = 0; j < block->getNumberOfSequences(); ++j) {
              MafSequence* subseq;
              if (i == 0) {
                subseq = block->getSequence(j).subSequence(0, pos[i]);
              } else {
                subseq = block->getSequence(j).subSequence(pos[i - 1], pos[i] - pos[i - 1]);
              }
              newBlock->addSequence(*subseq);
              delete subseq;
            }
            blockBuffer_.push_back(newBlock);
          }
          
          if (keepTrashedBlocks_) {
            MafBlock* outBlock = new MafBlock();
            outBlock->setScore(block->getScore());
            outBlock->setPass(block->getPass());
            for (unsigned int j = 0; j < block->getNumberOfSequences(); ++j) {
              MafSequence* outseq = block->getSequence(j).subSequence(pos[i], pos[i + 1] - pos[i]);
              outBlock->addSequence(*outseq);
              delete outseq;
            } 
            trashBuffer_.push_back(outBlock);
          }
        }
        //Add last block:
        if (pos[pos.size() - 1] < block->getNumberOfSites()) {
          MafBlock* newBlock = new MafBlock();
          newBlock->setScore(block->getScore());
          newBlock->setPass(block->getPass());
          for (unsigned int j = 0; j < block->getNumberOfSequences(); ++j) {
            MafSequence* subseq;
            subseq = block->getSequence(j).subSequence(pos[pos.size() - 1], block->getNumberOfSites() - pos[pos.size() - 1]);
            newBlock->addSequence(*subseq);
            delete subseq;
          }
          blockBuffer_.push_back(newBlock);
        }
        if (verbose_)
          ApplicationTools::displayTaskDone();

        delete block;
      }  
    } while (trashBuffer_.size() == 0);
  }

  MafBlock* block = blockBuffer_.front();
  blockBuffer_.pop_front();
  return block;
}

MafBlock* QualityFilterMafIterator::nextBlock() throw (Exception)
{
  if (blockBuffer_.size() == 0) {
    do {
      //Else there is no more block in the buffer, we need parse more:
      MafBlock* block = iterator_->nextBlock();
      if (!block) return 0; //No more block.
    
      //Parse block.
      vector< vector<int> > aln;
      for (size_t i = 0; i < species_.size(); ++i) {
        const MafSequence* seq = &block->getSequenceForSpecies(species_[i]);
        if (seq->hasAnnotation(SequenceQuality::QUALITY_SCORE)) {
          aln.push_back(dynamic_cast<const SequenceQuality&>(seq->getAnnotation(SequenceQuality::QUALITY_SCORE)).getScores());
        }
      }
      if (aln.size() != species_.size()) {
        blockBuffer_.push_back(block);
        if (logstream_) {
          (*logstream_ << "QUAL CLEANER: block is missing qulaity score for at least one species and will therefore not be filtered.").endLine();
        }
        //NB here we could decide to discard the block instead!
      } else {
        size_t nr = aln.size();
        size_t nc = block->getNumberOfSites();
        //First we create a mask:
        vector<unsigned int> pos;
        vector<int> col(nr);
        //Reset window:
        window_.clear();
        //Init window:
        size_t i;
        for (i = 0; i < windowSize_; ++i) {
          for (size_t j = 0; j < nr; ++j) {
            col[j] = aln[j][i];
          }
          window_.push_back(col);
        }
        //Slide window:
        if (verbose_) {
          ApplicationTools::message->endLine();
          ApplicationTools::displayTask("Sliding window for quality filter", true);
        }
        while (i < nc) {
          if (verbose_)
            ApplicationTools::displayGauge(i - windowSize_, nc - windowSize_ - 1, '>');
          //Evaluate current window:
          double mean = 0;
          double n = static_cast<double>(aln.size() * windowSize_);
          for (size_t u = 0; u < window_.size(); ++u)
            for (size_t v = 0; v < window_[u].size(); ++v) {
              mean += (window_[u][v] > 0 ? static_cast<double>(window_[u][v]) : 0.);
              if (window_[u][v] == -1) n--;
            }
          if (n > 0 && (mean / n) < minQual_) {
            if (pos.size() == 0) {
              pos.push_back(i - windowSize_);
              pos.push_back(i);
            } else {
              if (i - windowSize_ <= pos[pos.size() - 1]) {
                pos[pos.size() - 1] = i; //Windows are overlapping and we extend previous region
              } else { //This is a new region
                pos.push_back(i - windowSize_);
                pos.push_back(i);
              }
            }
          }
      
          //Move forward:
          for (unsigned int k = 0; k < step_; ++k) {
            for (size_t j = 0; j < nr; ++j) {
              col[j] = aln[j][i];
            }  
            window_.push_back(col);
            window_.pop_front();
            ++i;
          }
        }

        //Evaluate last window:
        double mean = 0;
        double n = static_cast<double>(aln.size() * windowSize_);
        for (size_t u = 0; u < window_.size(); ++u)
          for (size_t v = 0; v < window_[u].size(); ++v) {
            mean += (window_[u][v] > 0 ? static_cast<double>(window_[u][v]) : 0.);
            if (window_[u][v] == -1) n--;
          }
        if (n > 0 && (mean / n) < minQual_) {
          if (pos.size() == 0) {
            pos.push_back(i - windowSize_);
            pos.push_back(i);
          } else {
            if (i - windowSize_ < pos[pos.size() - 1]) {
              pos[pos.size()] = i; //Windows are overlapping and we extend previous region
            } else { //This is a new region
              pos.push_back(i - windowSize_);
              pos.push_back(i);
            }
          }
        }
        if (verbose_)
          ApplicationTools::displayTaskDone();
    
        //Now we remove regions with two many gaps, using a sliding window:
        if (pos.size() == 0) {
          blockBuffer_.push_back(block);
          if (logstream_) {
            (*logstream_ << "QUAL CLEANER: block is clean and kept as is.").endLine();
          }
        } else if (pos.size() == 2 && pos[0] == 0 && pos[pos.size() - 1] == block->getNumberOfSites()) {
          //Everything is removed:
          if (logstream_) {
            (*logstream_ << "QUAL CLEANER: block was entirely removed. Tried to get the next one.").endLine();
          }
        } else {
          if (logstream_) {
            (*logstream_ << "QUAL CLEANER: block with size "<< block->getNumberOfSites() << " will be split into " << (pos.size() / 2 + 1) << " blocks.").endLine();
          }
          if (verbose_) {
            ApplicationTools::message->endLine();
            ApplicationTools::displayTask("Spliting block", true);
          }
          for (i = 0; i < pos.size(); i+=2) {
            if (verbose_)
              ApplicationTools::displayGauge(i, pos.size() - 2, '=');
            if (logstream_) {
              (*logstream_ << "QUAL CLEANER: removing region (" << pos[i] << ", " << pos[i+1] << ") from block.").endLine();
            }
            if (pos[i] > 0) {
              MafBlock* newBlock = new MafBlock();
              newBlock->setScore(block->getScore());
              newBlock->setPass(block->getPass());
              for (unsigned int j = 0; j < block->getNumberOfSequences(); ++j) {
                MafSequence* subseq;
                if (i == 0) {
                  subseq = block->getSequence(j).subSequence(0, pos[i]);
                } else {
                  subseq = block->getSequence(j).subSequence(pos[i - 1], pos[i] - pos[i - 1]);
                }
                newBlock->addSequence(*subseq);
                delete subseq;
              }
              blockBuffer_.push_back(newBlock);
            }
           
            if (keepTrashedBlocks_) {
              MafBlock* outBlock = new MafBlock();
              outBlock->setScore(block->getScore());
              outBlock->setPass(block->getPass());
              for (unsigned int j = 0; j < block->getNumberOfSequences(); ++j) {
                MafSequence* outseq = block->getSequence(j).subSequence(pos[i], pos[i + 1] - pos[i]);
                outBlock->addSequence(*outseq);
                delete outseq;
              } 
              trashBuffer_.push_back(outBlock);
            }
          }
          //Add last block:
          if (pos[pos.size() - 1] < block->getNumberOfSites()) {
            MafBlock* newBlock = new MafBlock();
            newBlock->setScore(block->getScore());
            newBlock->setPass(block->getPass());
            for (unsigned int j = 0; j < block->getNumberOfSequences(); ++j) {
              MafSequence* subseq;
              subseq = block->getSequence(j).subSequence(pos[pos.size() - 1], block->getNumberOfSites() - pos[pos.size() - 1]);
              newBlock->addSequence(*subseq);
              delete subseq;
            }
            blockBuffer_.push_back(newBlock);
          }
          if (verbose_)
            ApplicationTools::displayTaskDone();

          delete block;
        }
      }
    } while (trashBuffer_.size() == 0);
  }

  MafBlock* block = blockBuffer_.front();
  blockBuffer_.pop_front();
  return block;
}

void OutputMafIterator::writeHeader(std::ostream& out) const
{
  out << "##maf version=1 program=Bio++" << endl << endl;
  //There are more options in the header that we may want to support...
}

void OutputMafIterator::writeBlock(std::ostream& out, const MafBlock& block) const
{
  out << "a";
  if (block.getScore() > -1.)
    out << " score=" << block.getScore();
  if (block.getPass() > 0)
    out << " pass=" << block.getPass();
  out << endl;
  
  //Now we write sequences. First need to count characters for aligning blocks:
  size_t mxcSrc = 0, mxcStart = 0, mxcSize = 0, mxcSrcSize = 0;
  for (unsigned int i = 0; i < block.getNumberOfSequences(); i++) {
    const MafSequence* seq = &block.getSequence(i);
    unsigned int start = 0; //Maybe we should output sthg else here?
    if (seq->hasCoordinates())
      start = seq->start();
    mxcSrc     = max(mxcSrc    , seq->getName().size());
    mxcStart   = max(mxcStart  , TextTools::toString(start).size());
    mxcSize    = max(mxcSize   , TextTools::toString(seq->getGenomicSize()).size());
    mxcSrcSize = max(mxcSrcSize, TextTools::toString(seq->getSrcSize()).size());
  }
  //Now print each sequence:
  for (unsigned int i = 0; i < block.getNumberOfSequences(); i++) {
    const MafSequence* seq = &block.getSequence(i);
    out << "s ";
    out << TextTools::resizeRight(seq->getName(), mxcSrc, ' ') << " ";
    unsigned int start = 0; //Maybe we should output sthg else here?
    if (seq->hasCoordinates())
      start = seq->start();
    out << TextTools::resizeLeft(TextTools::toString(start), mxcStart, ' ') << " ";
    out << TextTools::resizeLeft(TextTools::toString(seq->getGenomicSize()), mxcSize, ' ') << " ";
    out << seq->getStrand() << " ";
    out << TextTools::resizeLeft(TextTools::toString(seq->getSrcSize()), mxcSrcSize, ' ') << " ";
    //Shall we write the sequence as masked?
    string seqstr = seq->toString();
    if (mask_ && seq->hasAnnotation(SequenceMask::MASK)) {
      const SequenceMask* mask = &dynamic_cast<const SequenceMask&>(seq->getAnnotation(SequenceMask::MASK));
      for (unsigned int j = 0; j < seqstr.size(); ++j) {
        char c = ((*mask)[j] ? tolower(seqstr[j]) : seqstr[j]);
        out << c;
      }
    } else {
      out << seqstr;
    }
    out << endl;
    //Write quality scores if any:
    if (mask_ && seq->hasAnnotation(SequenceQuality::QUALITY_SCORE)) {
      const SequenceQuality* qual = &dynamic_cast<const SequenceQuality&>(seq->getAnnotation(SequenceQuality::QUALITY_SCORE));
      out << "q ";
      out << TextTools::resizeRight(seq->getName(), mxcSrc + mxcStart + mxcSize + mxcSrcSize + 5, ' ') << " ";
      string qualStr;
      for (unsigned int j = 0; j < seq->size(); ++j) {
        double s = (*qual)[j];
        if (s == -1) {
          qualStr += "-";
        } else if (s == -2) {
          qualStr += "?";
        } else if (s >=0 && s < 10) {
          qualStr += TextTools::toString(s);
        } else if (s == 10) {
          qualStr += "F";
        } else {
          throw Exception("MafAlignmentParser::writeBlock. Unsuported score value: " + TextTools::toString(s));
        }
      }
      out << qualStr << endl;
    }
  }
  out << endl;
}
