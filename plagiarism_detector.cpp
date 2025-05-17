#include <iostream>
#include <fstream>
#include <unordered_set>
#include <string>
#include <cmath>
#include <iomanip>
#include <cctype>
#include <vector>
#include <locale>
#include <codecvt>

using namespace std;

class PlagiarismDetector {
private:
    const int DEFAULT_NGRAM_SIZE = 3;  // Reduced for better sensitivity with Arabic
    const unsigned long long BASE = 256;
    const unsigned long long MOD = 1e9 + 7;
    
    int ngram_size;
    unordered_set<unsigned long long> ngrams1;
    unordered_set<unsigned long long> ngrams2;

public:
    explicit PlagiarismDetector(int size = 0) : ngram_size(size > 0 ? size : DEFAULT_NGRAM_SIZE) {}

    double compareFiles(const string& file1, const string& file2) {
        try {
            ngrams1.clear(); ngrams2.clear();
            string text1 = readFileAsUTF8(file1);
            string text2 = readFileAsUTF8(file2);
            
            if (text1.empty() || text2.empty()) {
                throw runtime_error("One or both files are empty");
            }
            
            text1 = normalizeArabicText(text1);
            text2 = normalizeArabicText(text2);
            
            generateNgrams(text1, ngrams1);
            generateNgrams(text2, ngrams2);
            
            return calculateSimilarity() * 100.0;
        } catch (const exception& e) {
            cerr << "Error: " << e.what() << endl;
            return -1.0;
        }
    }

private:
    string readFileAsUTF8(const string& filename) {
        ifstream file(filename, ios::binary);
        if (!file.is_open()) {
            throw runtime_error("Failed to open file: " + filename);
        }
        
        // Read entire file content
        string content((istreambuf_iterator<char>(file)), 
                     istreambuf_iterator<char>());
        file.close();
        return content;
    }

    string normalizeArabicText(const string& utf8_text) {
        // Convert UTF-8 to wide string
        wstring_convert<codecvt_utf8<wchar_t>> conv;
        wstring wide_text = conv.from_bytes(utf8_text);
        
        // Normalization steps
        wstring normalized;
        for (wchar_t c : wide_text) {
            // Remove diacritics
            if (c >= 0x064B && c <= 0x0652) continue;
            
            // Normalize letters
            switch (c) {
                case L'آ': case L'أ': case L'إ': case L'ٱ': c = L'ا'; break;
                case L'ى': case L'ي': c = L'ي'; break;
                case L'ة': c = L'ه'; break;
                case L'ـ': continue; // Remove tatweel
            }
            
            // Remove punctuation and numbers
            if (iswpunct(c) || iswdigit(c)) continue;
            
            normalized += c;
        }
        
        // Convert back to UTF-8
        return conv.to_bytes(normalized);
    }

    void generateNgrams(const string& text, unordered_set<unsigned long long>& ngrams) {
        if (text.length() < static_cast<size_t>(ngram_size)) {
            return;
        }

        for (size_t i = 0; i <= text.length() - ngram_size; ++i) {
            unsigned long long hash = 0;
            for (int j = 0; j < ngram_size; ++j) {
                hash = (hash * BASE + static_cast<unsigned char>(text[i + j])) % MOD;
            }
            ngrams.insert(hash);
        }
    }

    double calculateSimilarity() const {
        if (ngrams1.empty() && ngrams2.empty()) return 1.0;
        if (ngrams1.empty() || ngrams2.empty()) return 0.0;

        size_t common = 0;
        for (const auto& ngram : ngrams1) {
            if (ngrams2.count(ngram)) {
                common++;
            }
        }

        size_t total_unique = ngrams1.size() + ngrams2.size() - common;
        return static_cast<double>(common) / total_unique; 
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <file1> <file2>" << endl;
        cerr << "Default ngram_size is 3" << endl;
        return 1;
    }

    try {
        try {
        // Try multiple locale options
        try {
            locale::global(locale("en_US.UTF-8"));
        } catch (const exception&) {
            try {
                locale::global(locale("C.UTF-8"));
            } catch (const exception&) {
                locale::global(locale(""));
            }
        }
    cout.imbue(locale());
    cerr.imbue(locale());
} catch (const exception& e) {
    cerr << "Warning: Couldn't set preferred locale (" << e.what() << "), using default" << endl;
}

        PlagiarismDetector detector;
        double similarity = detector.compareFiles(argv[1], argv[2]);

        if (similarity < 0) {
            return 1;
        }

        cout << "\nPlagiarism Detection Results" << endl;
        cout << "===========================" << endl;
        cout << "File 1: " << argv[1] << endl;
        cout << "File 2: " << argv[2] << endl;
        cout << fixed << setprecision(2);
        cout << "Similarity: " << similarity << "%\n" << endl;

        if (similarity < 10) {
            cout << "Interpretation: No significant similarity detected" << endl;
        } else if (similarity < 30) {
            cout << "Interpretation: Minor similarity - possibly coincidental" << endl;
        } else if (similarity < 50) {
            cout << "Interpretation: Moderate similarity - potential paraphrasing" << endl;
        } else if (similarity < 70) {
            cout << "Interpretation: High similarity - likely plagiarism" << endl;
        } else {
            cout << "Interpretation: Very high similarity - probable direct copying" << endl;
        }
    } catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << endl;
        return 1;
    }

    return 0;
}