#include <iostream>
#include <sstream>   // For std::ostringstream
#include <iomanip>   // For std::setw
#include <curl/curl.h>  // For cURL functions
#include <string>

// Callback function to handle the response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::string urlEncode(const std::string &value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (unsigned char c : value) {
        // Encode only spaces and special characters
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << int(c);
        }
    }
    return escaped.str();
}

std::string getBestMove(const std::string& fen) {
    std::string url = "http://www.chessdb.cn/cdb.php?action=querybest&board=" + urlEncode(fen);
    std::string response;

    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects if any

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Failed to initialize cURL" << std::endl;
    }

    return response;
}

int main() {
    std::string fen = "r3r1k1/1bq1bppp/p2p1n2/npp1p3/P3P3/2PP1N2/1PB2PPP/R1BQRNK1 w - - 4 14";  // Initial position
    std::string bestMove = getBestMove(fen);

    std::cout << "Best Move: " << bestMove << std::endl;
    return 0;
}
