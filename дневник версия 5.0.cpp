#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <random>
#include <Windows.h>
#include <filesystem>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

std::string performRequest(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Не удалось выполнить запрос!"
                << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return readBuffer;
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

vector<string> readFramesFromFile(const string& filename, const string& delimiter) {
    vector<string> frames;
    ifstream file(filename);
    if (!file) {
        cerr << "Не удалось открыть файл " << filename << endl;
        return frames;
    }

    string line, frame;
    while (getline(file, line)) {
        if (line == delimiter) {
            if (!frame.empty()) {
                frames.push_back(frame);
                frame.clear();
            }
        }
        else {
            frame += line + "\n";
        }
    }
    if (!frame.empty()) {
        frames.push_back(frame);
    }

    return frames;
}

void animate(const vector<string>& frames) {
    const int numFrames = frames.size();
    for (int i = 0; i < numFrames; ++i) {
        clearScreen();
        cout << frames[i] << endl;
        this_thread::sleep_for(chrono::milliseconds(300));
    }
}

int randomInt(int min, int max) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}

void play_animation(vector<string>& frames1, vector<string>& frames2) {
    int random_number = randomInt(0, 100);
    if (random_number % 2 == 0)
    {
        animate(frames1);
    }
    else {
        animate(frames2);
    }
}

void getFileContents(const std::string& filename, std::string& fileContents) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл: " << filename << std::endl;
        std::ofstream create_file(filename);
        create_file << "";
        create_file.close();
        std::cout << "Новый файл был успешно создан!";
        return;
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    fileContents = oss.str();
    file.close();
}

void saveToFile(const std::string& filename, const std::string& newContent) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл: " << filename << std::endl;
        return;
    }
    file << newContent;
    file.close();
}


std::string getCurrentDay() {
    std::time_t now = std::time(0);
    std::tm* formatted = std::localtime(&now);

    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << formatted->tm_mday << "-"
        << std::setw(2) << std::setfill('0') << formatted->tm_mon + 1 << "-"
        << 1900 + formatted->tm_year;
    return oss.str();
}

static std::vector<std::filesystem::path> listOfFilesInDirectory(const std::string& dirpath) {
    vector<std::filesystem::path> filepathList;
    for (const auto& filepath : std::filesystem::directory_iterator(dirpath)) {
        if (filepath.is_regular_file())
        {
            filepathList.push_back(filepath.path());
        }
    }
    return filepathList;
}

void showAllFilenamesInDirectory(const string& directory) {
    for (const auto& file : listOfFilesInDirectory(directory)) {
        std::cout << file.filename() << std::endl;
    }
}

void diary() {
    const std::string base_directory = "diaries";
    std::filesystem::create_directory(base_directory);
    std::string input;
    std::string content;
    while (true) {
        showAllFilenamesInDirectory(base_directory);
        std::cout << "Введите имя файла:";
        std::getline(std::cin, input);
        if (input == "exit") {
            break;
        }
        else if (input == "today") {
            std::string currentDate = getCurrentDay();
            std::string todayDiaryPath = base_directory + "/diary" + currentDate + ".txt";
            std::string input_diary;
            std::string content_diary;
            getFileContents(todayDiaryPath, content_diary);
            while (true) {
                std::cout << content_diary;
                std::cout << "Введите что-нибудь:";
                std::getline(std::cin, input);
                if (input == "end") {
                    saveToFile(todayDiaryPath, content_diary);
                    break;
                }
                content_diary += input;
                clearScreen();
            }
        }
        else {
            if (std::filesystem::exists(base_directory + "/" + input)) {
                getFileContents(base_directory + "/" + input, content);
                std::cout << content;
            }
            else {
                std::cout << "Такой файл не существует!";
            }
        }
    }
}

const std::string BASE_URL = "https://rickandmortyapi.com/api";

std::string getCharacterName(int id) {
    json jsonData = json::parse(performRequest(BASE_URL + "/character"));
    //std::cout << jsonData["info"].get<json>() << std::endl;
    return jsonData["results"]
        .get<std::vector<json>>()[id]["name"]
        .get<std::string>();
}
std::string getEpisodeAirDate(int id) {
    json jsonData = json::parse(performRequest(BASE_URL + "/episode"));
    //std::cout << jsonData["info"].get<json>() << std::endl;
    vector<json> result = jsonData["results"].get<std::vector<json>>();
    std::string air_date = result[id]["air_date"].get<std::string>();
    return air_date;
}

int main() {
    setlocale(0, "ru");
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
    //  json jsonData = json::parse(performRequest("https://alfa-leetcode-api.onrender.com/problems?limit=1"));
    cout << getCharacterName(2);
    cout << getEpisodeAirDate(1);
    return 0;
    string filename1 = "for.txt";
    string delimiter1 = ",";
    vector<string> frames1 = readFramesFromFile(filename1, delimiter1);

    string filename2 = "tytyty.txt";
    string delimiter2 = "?";
    vector<string> frames2 = readFramesFromFile(filename2, delimiter2);

    string filename3 = "go.txt";
    string delimiter3 = "r";
    vector<string> frames3 = readFramesFromFile(filename3, delimiter3);


    if (!frames1.empty() && !frames2.empty()) {
        std::string input = "";
        while (true) {
            std::cout << "Введите что угодно\n";
            std::getline(std::cin, input);
            if (input == "выход") {
                break;
            }
            if (input == "animation") {
                play_animation(frames1, frames2);
            }
            if (input == "diary") {
                diary();
            }
        }
    }
    else {
        cout << "Нет кадров для анимации." << endl;
    }

    return 0;
}
