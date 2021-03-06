/*
 * Copyright 2008 Daniel de Kok
 *
 * This file is part of citar.
 *
 * Citar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Citar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Citar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

#include <tr1/memory>
#include <tr1/unordered_map>

#include <citar/corpus/BrownCorpusReader.hh>
#include <citar/corpus/SentenceHandler.hh>
#include <citar/tagger/hmm/Model.hh>

using namespace std;
using namespace std::tr1;
using namespace citar::corpus;
using namespace citar::tagger;

typedef std::tr1::unordered_map<string, map<string, size_t> > Lexicon;

class TrainHandler : public SentenceHandler
{
public:
	TrainHandler() : d_lexicon(new Lexicon),
		d_uniGrams(new unordered_map<string, size_t>),
		d_biGrams(new unordered_map<string, size_t>),
		d_triGrams(new unordered_map<string, size_t>) {}
	void handleSentence(vector<TaggedWord> const &sentence);
	Lexicon const &lexicon();
	unordered_map<string, size_t> const &biGrams();
	unordered_map<string, size_t> const &triGrams();
	unordered_map<string, size_t> const &uniGrams();
public:
	shared_ptr<Lexicon> d_lexicon;
	shared_ptr<unordered_map<string, size_t> > d_uniGrams;
	shared_ptr<unordered_map<string, size_t> > d_biGrams;
	shared_ptr<unordered_map<string, size_t> > d_triGrams;
};

void TrainHandler::handleSentence(vector<TaggedWord> const &sentence)
{
	for (vector<TaggedWord>::const_iterator iter = sentence.begin();
		iter != sentence.end(); ++iter)
	{
		++(*d_uniGrams)[iter->tag];

		size_t beginDistance = distance(sentence.begin(), iter);

		if (beginDistance > 0)
			++(*d_biGrams)[(iter - 1)->tag + " " + iter->tag];

		if (beginDistance > 1)
			++(*d_triGrams)[(iter - 2)->tag + " " + (iter - 1)->tag + " " + iter->tag];

		++(*d_lexicon)[iter->word][iter->tag];
	}
}

inline Lexicon const &TrainHandler::lexicon()
{
	return *d_lexicon;
}

inline unordered_map<string, size_t> const &TrainHandler::biGrams()
{
	return *d_biGrams;
}

inline unordered_map<string, size_t> const &TrainHandler::triGrams()
{
	return *d_triGrams;
}

inline unordered_map<string, size_t> const &TrainHandler::uniGrams()
{
	return *d_uniGrams;
}

void writeLexicon(ostream &out, Lexicon const &lexicon)
{
	for (Lexicon::const_iterator iter = lexicon.begin();
		iter != lexicon.end(); ++iter)
	{
		out << iter->first;

		for (map<string, size_t>::const_iterator tagIter = iter->second.begin();
			tagIter != iter->second.end(); ++tagIter)
		{
			out << " " << tagIter->first << " " << tagIter->second;
		}

		out << endl;
	}
}

void writeNGrams(ostream &out,
		unordered_map<string, size_t> const &uniGrams,
		unordered_map<string, size_t> const &biGrams,
		unordered_map<string, size_t> const &triGrams)
{
	for (unordered_map<string, size_t>::const_iterator iter = uniGrams.begin();
			iter != uniGrams.end(); ++iter)
		out << iter->first << " " << iter->second << endl;

	for (unordered_map<string, size_t>::const_iterator iter = biGrams.begin();
			iter != biGrams.end(); ++iter)
	{
		string biGram = iter->first;
		out << biGram << " " << iter->second << endl;
	}

	for (unordered_map<string, size_t>::const_iterator iter = triGrams.begin();
			iter != triGrams.end(); ++iter)
	{
		string triGram = iter->first;
		out << triGram << " " << iter->second << endl;
	}
}

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		cerr << "Usage: " << argv[0] << " corpus lexicon ngrams" << endl;
		return 1;
	}

	vector<TaggedWord> startTags(2, TaggedWord("<START>", "<START>"));
	vector<TaggedWord> endTags(1, TaggedWord("<END>", "<END>"));
	BrownCorpusReader brownCorpusReader(startTags, endTags, true);

	shared_ptr<TrainHandler> trainHandler(new TrainHandler);
	brownCorpusReader.addSentenceHandler(trainHandler);

	ifstream corpusStream(argv[1]);
	if (!corpusStream.good())
	{
		cerr << "Could not open corpus for reading: " << argv[1] << endl;
		return 1;
	}

	brownCorpusReader.parse(corpusStream);

	ofstream lexiconStream(argv[2]);
	if (!lexiconStream.good())
	{
		cerr << "Could not open lexicon for writing: " << argv[2] << endl;
		return 1;
	}

	ofstream ngramStream(argv[3]);
	if (!ngramStream.good())
	{
		cerr << "Could not open ngram list for writing: " << argv[3] << endl;
		return 1;
	}


	writeLexicon(lexiconStream, trainHandler->lexicon());

	writeNGrams(ngramStream, trainHandler->uniGrams(), trainHandler->biGrams(),
		trainHandler->triGrams());
}
