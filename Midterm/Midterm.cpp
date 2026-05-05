// Midterm.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <chrono>
#include <queue>

const std::size_t MaxQueuSize = 1000;
using WordCountMapType = std::unordered_map<std::string, std::size_t>;  // use this to keep track of count of each word.

struct WordCount // use this to track words to sort, this is an example of storing the count, but there are other options
{
	std::string word;
	std::size_t count;
};

std::vector<WordCount> GetTopWords(WordCountMapType& wordMap, int topN)
{
	std::vector<WordCount> vec;
	// map out the words and the counts
	// sort to only get the topN counts

	return vec;
}

// This writes the word counts to the outputfile
void WriteOutput(const char* const filename, WordCountMapType& wordMap)
{
	std::ofstream out(filename);
	std::vector<WordCount> topWords = GetTopWords(wordMap, 20);

	for (auto& data : topWords)
	{
		out << data.word << " : " << data.count << std::endl;
	}
}

WordCountMapType WordsInFile(const char* const fileName) // for each word
{ // in file, return
	std::ifstream file(fileName); // # of
	WordCountMapType wordCounts; // occurrences
	for (std::string word; file >> word;)// this is not efficient. You can read the whole file into one string and the process that string , instead of reading one word at a time from file.
	{
		++wordCounts[word];
	}
	return wordCounts;
}
int main(int argc, char* argv[])
{
	std::cout << "Started\n";

    std::cout << "Midterm Word Count:\n";
}