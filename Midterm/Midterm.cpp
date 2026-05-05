// Midterm.cpp : Word counting on multiple books using basic multithreading

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
#include <cctype>

const std::size_t MaxQueuSize = 1000;
const std::size_t Top_N = 20;

using WordCountMapType = std::unordered_map<std::string, std::size_t>;

struct WordCount
{
    std::string word;
    std::size_t count;
};

struct BookState
{
    std::queue<std::string> runningQueue;

    std::mutex queueMutex;
    std::condition_variable queueNotFull;

    bool finished = false;
    bool countedComplete = false;

    WordCountMapType localWordCounts;
    std::size_t wordsProcessed = 0;
};

std::string CleanWord(const std::string& rawWord)
{
    std::string cleaned;

    for (char ch : rawWord)
    {
        unsigned char c = static_cast<unsigned char>(ch);

        if (std::isalnum(c))
        {
            cleaned += static_cast<char>(std::tolower(c));
        }
    }

    return cleaned;
}

std::vector<WordCount> GetTopWords(const WordCountMapType& wordMap, int topN)
{
    std::vector<WordCount> vec;

    for (const auto& pair : wordMap)
    {
        vec.push_back({ pair.first, pair.second });
    }

    std::sort(vec.begin(), vec.end(),
        [](const WordCount& a, const WordCount& b)
        {
            if (a.count == b.count)
            {
                return a.word < b.word;
            }

            return a.count > b.count;
        });

    if (vec.size() > static_cast<std::size_t>(topN))
    {
        vec.resize(topN);
    }

    return vec;
}

std::string MakeOutputFileName(const std::string& inputFile)
{
    std::string name = inputFile;

    std::size_t slashPos = name.find_last_of("/\\");
    if (slashPos != std::string::npos)
    {
        name = name.substr(slashPos + 1);
    }

    std::size_t dotPos = name.find_last_of('.');
    if (dotPos != std::string::npos)
    {
        name = name.substr(0, dotPos);
    }

    return name + "_output.txt";
}

void LogStatus(std::ofstream& outputFile, std::mutex& outputMutex, const std::string& message)
{
    std::lock_guard<std::mutex> lock(outputMutex);

    std::cout << message << std::endl;

    if (outputFile)
    {
        outputFile << message << std::endl;
    }
}

void PrintTopWordsToConsole(const WordCountMapType& wordMap)
{
    std::vector<WordCount> topWords = GetTopWords(wordMap, static_cast<int>(Top_N));

    std::cout << "\nTop " << Top_N << " words from all books:\n";

    for (const auto& data : topWords)
    {
        std::cout << data.word << " : " << data.count << std::endl;
    }
}

void WriteTopWordsToOutputFile(std::ofstream& outputFile, const WordCountMapType& wordMap)
{
    std::vector<WordCount> topWords = GetTopWords(wordMap, static_cast<int>(Top_N));

    outputFile << "\nTop " << Top_N << " words from all books:\n";

    for (const auto& data : topWords)
    {
        outputFile << data.word << " : " << data.count << std::endl;
    }
}

void ProcessBook(
    const std::string& fileName,
    BookState& state,
    std::condition_variable& dataAvailable,
    std::ofstream& outputFile,
    std::mutex& outputMutex)
{
    LogStatus(outputFile, outputMutex, "Started processing: " + fileName);

    std::ifstream file(fileName);

    if (!file)
    {
        LogStatus(outputFile, outputMutex, "ERROR: Could not open file: " + fileName);

        {
            std::lock_guard<std::mutex> lock(state.queueMutex);
            state.finished = true;
        }

        dataAvailable.notify_one();
        return;
    }

    for (std::string rawWord; file >> rawWord;)
    {
        std::string word = CleanWord(rawWord);

        if (word.size() <= 1)
        {
            continue;
        }

        ++state.localWordCounts[word];
        ++state.wordsProcessed;

        {
            std::unique_lock<std::mutex> lock(state.queueMutex);

            state.runningQueue.push(word);

            if (state.runningQueue.size() >= MaxQueuSize)
            {
                dataAvailable.notify_one();

                state.queueNotFull.wait(lock,
                    [&state]()
                    {
                        return state.runningQueue.size() < MaxQueuSize;
                    });
            }
        }

        if (state.wordsProcessed % 50000 == 0)
        {
            LogStatus(
                outputFile,
                outputMutex,
                "Progress: " + fileName + " processed " +
                std::to_string(state.wordsProcessed) + " words"
            );
        }
    }

    {
        std::lock_guard<std::mutex> lock(state.queueMutex);
        state.finished = true;
    }

    dataAvailable.notify_one();

    LogStatus(
        outputFile,
        outputMutex,
        "Complete: " + fileName + " processed " +
        std::to_string(state.wordsProcessed) + " words"
    );
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: wordCounting.exe book1.txt book2.txt book3.txt\n";
        return 1;
    }

    std::string outputFileName = MakeOutputFileName(argv[1]);
    std::ofstream outputFile(outputFileName);

    if (!outputFile)
    {
        std::cout << "ERROR: Could not create output file: " << outputFileName << std::endl;
        return 1;
    }

    std::mutex outputMutex;

    auto startTime = std::chrono::high_resolution_clock::now();

    LogStatus(outputFile, outputMutex, "Started");
    LogStatus(outputFile, outputMutex, "Midterm Word Count");
    LogStatus(outputFile, outputMutex, "Output file: " + outputFileName);

    int bookCount = argc - 1;

    std::vector<BookState> bookStates(bookCount);
    std::vector<std::thread> threads;

    WordCountMapType globalWordCounts;

    std::condition_variable dataAvailable;
    std::mutex dataAvailableMutex;

    for (int i = 0; i < bookCount; ++i)
    {
        std::string fileName = argv[i + 1];

        threads.emplace_back(
            ProcessBook,
            fileName,
            std::ref(bookStates[i]),
            std::ref(dataAvailable),
            std::ref(outputFile),
            std::ref(outputMutex)
        );
    }

    std::size_t remainingBooks = static_cast<std::size_t>(bookCount);

    while (remainingBooks > 0)
    {
        bool didWork = false;

        for (int i = 0; i < bookCount; ++i)
        {
            BookState& state = bookStates[i];

            std::vector<std::string> batch;
            bool bookCompletedThisLoop = false;

            {
                std::unique_lock<std::mutex> lock(state.queueMutex);

                bool queueReady = state.runningQueue.size() >= MaxQueuSize;
                bool finishedWithRemainingWords = state.finished && !state.runningQueue.empty();

                if (queueReady || finishedWithRemainingWords)
                {
                    while (!state.runningQueue.empty())
                    {
                        batch.push_back(state.runningQueue.front());
                        state.runningQueue.pop();
                    }

                    didWork = true;
                }

                if (state.finished && state.runningQueue.empty() && !state.countedComplete)
                {
                    state.countedComplete = true;
                    --remainingBooks;
                    bookCompletedThisLoop = true;
                    didWork = true;
                }
            }

            state.queueNotFull.notify_one();

            for (const std::string& word : batch)
            {
                ++globalWordCounts[word];
            }

            if (bookCompletedThisLoop)
            {
                std::string fileName = argv[i + 1];

                LogStatus(
                    outputFile,
                    outputMutex,
                    "Main thread finished collecting queue data from: " + fileName
                );
            }
        }

        if (!didWork)
        {
            std::unique_lock<std::mutex> waitLock(dataAvailableMutex);

            dataAvailable.wait_for(
                waitLock,
                std::chrono::milliseconds(50)
            );
        }
    }

    for (std::thread& t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    LogStatus(outputFile, outputMutex, "Writing final top 20 words");

    {
        std::lock_guard<std::mutex> lock(outputMutex);
        WriteTopWordsToOutputFile(outputFile, globalWordCounts);
    }

    PrintTopWordsToConsole(globalWordCounts);

    auto endTime = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsedTime = endTime - startTime;

    LogStatus(
        outputFile,
        outputMutex,
        "Complete. Total time: " + std::to_string(elapsedTime.count()) + " seconds"
    );

    return 0;
}