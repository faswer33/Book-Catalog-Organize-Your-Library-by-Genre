# book_catalog.cpp
/**
 * 📚 Book Catalog – Organize Your Library by Genre (C++ Edition)
 * Features: add, edit, delete, search, genre stats, persistence
 * Uses only STL, no external libraries.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <random>
#include <filesystem>
#include <cctype>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#endif

// ─── Colors ──────────────────────────────────────────────────────────────────

#ifdef _WIN32
HANDLE hConsole;
void setColor(int color) { SetConsoleTextAttribute(hConsole, color); }
#define RESET_COLOR setColor(7)
#define COLOR_RED setColor(12)
#define COLOR_GREEN setColor(10)
#define COLOR_YELLOW setColor(14)
#define COLOR_BLUE setColor(9)
#define COLOR_MAGENTA setColor(13)
#define COLOR_CYAN setColor(11)
#define COLOR_BRIGHT setColor(15)
#define COLOR_DIM setColor(8)
#else
#define RESET_COLOR std::cout << "\x1b[0m"
#define COLOR_RED std::cout << "\x1b[31m"
#define COLOR_GREEN std::cout << "\x1b[32m"
#define COLOR_YELLOW std::cout << "\x1b[33m"
#define COLOR_BLUE std::cout << "\x1b[34m"
#define COLOR_MAGENTA std::cout << "\x1b[35m"
#define COLOR_CYAN std::cout << "\x1b[36m"
#define COLOR_BRIGHT std::cout << "\x1b[1m"
#define COLOR_DIM std::cout << "\x1b[2m"
#endif

#define C(str, color) color << str << RESET_COLOR

// ─── Helpers ─────────────────────────────────────────────────────────────────

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::string generate_uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

std::string get_home_dir() {
#ifdef _WIN32
    const char* h = std::getenv("USERPROFILE");
#else
    const char* h = std::getenv("HOME");
#endif
    return h ? std::string(h) : ".";
}

// ─── Data Model ─────────────────────────────────────────────────────────────

struct Book {
    std::string id;
    std::string title;
    std::string author;
    std::string genre;
    int year;
    std::string isbn;
};

struct CatalogData {
    std::vector<Book> books;
};

// ─── JSON (simplified) ─────────────────────────────────────────────────────

std::string escape_json(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

std::string serialize_books(const std::vector<Book>& books) {
    std::ostringstream json;
    json << "{\n  \"books\": [\n";
    for (size_t i = 0; i < books.size(); ++i) {
        const auto& b = books[i];
        json << "    {\n";
        json << "      \"id\": \"" << escape_json(b.id) << "\",\n";
        json << "      \"title\": \"" << escape_json(b.title) << "\",\n";
        json << "      \"author\": \"" << escape_json(b.author) << "\",\n";
        json << "      \"genre\": \"" << escape_json(b.genre) << "\",\n";
        json << "      \"year\": " << b.year << ",\n";
        json << "      \"isbn\": \"" << escape_json(b.isbn) << "\"\n";
        json << "    }";
        if (i + 1 < books.size()) json << ",";
        json << "\n";
    }
    json << "  ]\n}";
    return json.str();
}

bool deserialize_books(const std::string& json_str, std::vector<Book>& books) {
    books.clear();
    // Simple manual parser: find book objects
    size_t pos = json_str.find("\"books\":");
    if (pos == std::string::npos) return false;
    size_t start_arr = json_str.find("[", pos);
    if (start_arr == std::string::npos) return false;
    size_t end_arr = json_str.rfind("]");
    if (end_arr == std::string::npos || end_arr <= start_arr) return false;
    std::string arr = json_str.substr(start_arr + 1, end_arr - start_arr - 1);
    // Split by "{" and "}"
    size_t brace_start = arr.find("{");
    while (brace_start != std::string::npos) {
        size_t brace_end = arr.find("}", brace_start);
        if (brace_end == std::string::npos) break;
        std::string obj = arr.substr(brace_start, brace_end - brace_start + 1);
        // Parse fields
        Book b;
        auto extract = [&](const std::string& key) -> std::string {
            size_t p = obj.find("\"" + key + "\":");
            if (p == std::string::npos) return "";
            p = obj.find(":", p) + 1;
            while (p < obj.length() && (obj[p] == ' ' || obj[p] == '\n' || obj[p] == '\r')) p++;
            if (obj[p] == '"') {
                p++;
                size_t e = obj.find("\"", p);
                if (e == std::string::npos) return "";
                return obj.substr(p, e - p);
            } else {
                size_t e = obj.find_first_of(",}\n\r", p);
                if (e == std::string::npos) return "";
                return obj.substr(p, e - p);
            }
        };
        b.id = extract("id");
        b.title = extract("title");
        b.author = extract("author");
        b.genre = extract("genre");
        std::string year_str = extract("year");
        if (!year_str.empty()) b.year = std::stoi(year_str);
        else b.year = 0;
        b.isbn = extract("isbn");
        if (!b.id.empty() && !b.title.empty()) {
            books.push_back(b);
        }
        brace_start = arr.find("{", brace_end);
    }
    return true;
}

// ─── Catalog Manager ──────────────────────────────────────────────────────

class BookCatalog {
public:
    BookCatalog() {
        home = get_home_dir();
        data_dir = home + "/.book_catalog";
        std::filesystem::create_directories(data_dir);
        data_file = data_dir + "/books.json";
        load();
    }

    void load() {
        std::ifstream file(data_file);
        if (!file.is_open()) {
            books.clear();
            return;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        if (!deserialize_books(buffer.str(), books)) {
            books.clear();
        }
    }

    void save() {
        std::string json = serialize_books(books);
        std::string temp = data_file + ".tmp";
        std::ofstream out(temp);
        if (out.is_open()) {
            out << json;
            out.close();
            std::filesystem::rename(temp, data_file);
        }
    }

    void addBook(const std::string& title, const std::string& author, const std::string& genre, int year, const std::string& isbn) {
        Book b;
        b.id = generate_uuid();
        b.title = title;
        b.author = author;
        b.genre = genre;
        b.year = year;
        b.isbn = isbn;
        books.push_back(b);
        save();
    }

    bool deleteBook(const std::string& id) {
        for (auto it = books.begin(); it != books.end(); ++it) {
            if (it->id == id) {
                books.erase(it);
                save();
                return true;
            }
        }
        return false;
    }

    bool updateBook(const std::string& id, const std::map<std::string, std::string>& updates) {
        for (auto& b : books) {
            if (b.id == id) {
                auto it = updates.find("title");
                if (it != updates.end()) b.title = it->second;
                it = updates.find("author");
                if (it != updates.end()) b.author = it->second;
                it = updates.find("genre");
                if (it != updates.end()) b.genre = it->second;
                it = updates.find("year");
                if (it != updates.end()) b.year = std::stoi(it->second);
                it = updates.find("isbn");
                if (it != updates.end()) b.isbn = it->second;
                save();
                return true;
            }
        }
        return false;
    }

    Book* getBook(const std::string& id) {
        for (auto& b : books) {
            if (b.id == id) return &b;
        }
        return nullptr;
    }

    std::vector<Book> search(const std::string& query, const std::string& genre,
                             const std::string& author, int yearMin, int yearMax) {
        std::vector<Book> results = books;
        if (!query.empty()) {
            std::string q = toLower(query);
            results.erase(std::remove_if(results.begin(), results.end(),
                [&](const Book& b) {
                    return !(toLower(b.title).find(q) != std::string::npos ||
                             toLower(b.author).find(q) != std::string::npos ||
                             toLower(b.genre).find(q) != std::string::npos);
                }), results.end());
        }
        if (!genre.empty()) {
            std::string g = toLower(genre);
            results.erase(std::remove_if(results.begin(), results.end(),
                [&](const Book& b) { return toLower(b.genre) != g; }), results.end());
        }
        if (!author.empty()) {
            std::string a = toLower(author);
            results.erase(std::remove_if(results.begin(), results.end(),
                [&](const Book& b) { return toLower(b.author).find(a) == std::string::npos; }), results.end());
        }
        if (yearMin > 0) {
            results.erase(std::remove_if(results.begin(), results.end(),
                [&](const Book& b) { return b.year < yearMin; }), results.end());
        }
        if (yearMax < 9999) {
            results.erase(std::remove_if(results.begin(), results.end(),
                [&](const Book& b) { return b.year > yearMax; }), results.end());
        }
        return results;
    }

    std::map<std::string, int> getGenres() const {
        std::map<std::string, int> m;
        for (const auto& b : books) {
            m[b.genre]++;
        }
        return m;
    }

    void getStats(int& total, int& authors, int& genres, int& minYear, int& maxYear) const {
        total = books.size();
        std::set<std::string> authorSet;
        for (const auto& b : books) authorSet.insert(b.author);
        authors = authorSet.size();
        genres = getGenres().size();
        if (total > 0) {
            minYear = books[0].year;
            maxYear = books[0].year;
            for (const auto& b : books) {
                if (b.year < minYear) minYear = b.year;
                if (b.year > maxYear) maxYear = b.year;
            }
        } else {
            minYear = 0;
            maxYear = 0;
        }
    }

    std::vector<Book> books;

private:
    std::string home, data_dir, data_file;
};

// ─── Main App ──────────────────────────────────────────────────────────────

class BookApp {
public:
    BookApp() : catalog() {}

    void run() {
        std::cout << "\033[2J\033[1;1H";
        std::cout << C("\n📚 Book Catalog – Organize Your Library", COLOR_BRIGHT) << C("", COLOR_CYAN) << std::endl;
        std::cout << C("Manage your books by genre!", COLOR_DIM) << std::endl;

        while (true) {
            showMenu();
            std::string choice = ask("Your choice: ");
            if (choice == "1") listBooks();
            else if (choice == "2") addBook();
            else if (choice == "3") searchBooks();
            else if (choice == "4") showStats();
            else if (choice == "5") browseByGenre();
            else if (choice == "6") editBook();
            else if (choice == "7") deleteBook();
            else if (choice == "0") {
                std::cout << C("👋 Goodbye!", COLOR_CYAN) << std::endl;
                break;
            } else {
                std::cout << C("❌ Invalid choice.", COLOR_RED) << std::endl;
            }
            if (choice != "0") {
                std::cout << "\nPress Enter to continue...";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cin.get();
            }
        }
    }

private:
    BookCatalog catalog;

    std::string ask(const std::string& prompt) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        return trim(line);
    }

    int askInt(const std::string& prompt, int def) {
        while (true) {
            std::string ans = ask(prompt);
            if (ans.empty()) return def;
            try { return std::stoi(ans); }
            catch (...) { std::cout << C("❌ Please enter a number.", COLOR_RED) << std::endl; }
        }
    }

    void showMenu() {
        std::cout << "\n" << C(std::string(50, '═'), COLOR_CYAN) << std::endl;
        std::cout << C("📚 BOOK CATALOG", COLOR_BRIGHT) << C("", COLOR_CYAN) << std::endl;
        std::cout << C(std::string(50, '═'), COLOR_CYAN) << std::endl;
        std::cout << "  Total books: " << catalog.books.size() << std::endl;
        std::cout << "  Genres: " << catalog.getGenres().size() << std::endl;
        std::cout << C(std::string(50, '═'), COLOR_CYAN) << std::endl;
        std::cout << "  1. 📖 List all books" << std::endl;
        std::cout << "  2. ➕ Add a book" << std::endl;
        std::cout << "  3. 🔍 Search books" << std::endl;
        std::cout << "  4. 📊 Statistics" << std::endl;
        std::cout << "  5. 🗂️  Browse by genre" << std::endl;
        std::cout << "  6. ✏️  Edit a book" << std::endl;
        std::cout << "  7. 🗑️  Delete a book" << std::endl;
        std::cout << "  0. 🚪 Exit" << std::endl;
        std::cout << C(std::string(50, '═'), COLOR_CYAN) << std::endl;
    }

    void listBooks() {
        if (catalog.books.empty()) {
            std::cout << C("No books found.", COLOR_YELLOW) << std::endl;
            return;
        }
        std::cout << "\n📖 BOOKS" << std::endl;
        std::cout << C(std::string(60, '─'), COLOR_DIM) << std::endl;
        for (const auto& b : catalog.books) {
            std::cout << "  " << b.title << " by " << b.author << " (" << b.year << ") – " << b.genre << " [ISBN: " << (b.isbn.empty() ? "N/A" : b.isbn) << "]" << std::endl;
        }
    }

    void addBook() {
        std::string title = ask("Title: ");
        std::string author = ask("Author: ");
        std::string genre = ask("Genre: ");
        int year = askInt("Year: ", 2000);
        std::string isbn = ask("ISBN (optional): ");
        catalog.addBook(title, author, genre, year, isbn);
        std::cout << C("✅ Book added.", COLOR_GREEN) << std::endl;
    }

    void searchBooks() {
        std::string query = ask("Search term (title/author/genre): ");
        std::string genre = ask("Filter by genre (optional): ");
        std::string author = ask("Filter by author (optional): ");
        int yearMin = askInt("Minimum year (optional): ", 0);
        int yearMax = askInt("Maximum year (optional): ", 9999);
        auto results = catalog.search(query, genre, author, yearMin, yearMax);
        if (results.empty()) {
            std::cout << C("No books match your search.", COLOR_YELLOW) << std::endl;
        } else {
            std::cout << "\n📖 SEARCH RESULTS" << std::endl;
            std::cout << C(std::string(60, '─'), COLOR_DIM) << std::endl;
            for (const auto& b : results) {
                std::cout << "  " << b.title << " by " << b.author << " (" << b.year << ") – " << b.genre << std::endl;
            }
        }
    }

    void showStats() {
        int total, authors, genres, minYear, maxYear;
        catalog.getStats(total, authors, genres, minYear, maxYear);
        auto genreMap = catalog.getGenres();
        std::cout << "\n📊 STATISTICS" << std::endl;
        std::cout << C(std::string(30, '─'), COLOR_DIM) << std::endl;
        std::cout << "  Total Books: " << total << std::endl;
        std::cout << "  Unique Authors: " << authors << std::endl;
        std::cout << "  Genres: " << genres << std::endl;
        if (total > 0) {
            std::cout << "  Oldest Book: " << minYear << std::endl;
            std::cout << "  Newest Book: " << maxYear << std::endl;
        } else {
            std::cout << "  Oldest Book: —" << std::endl;
            std::cout << "  Newest Book: —" << std::endl;
        }
        if (!genreMap.empty()) {
            std::cout << "\n📚 Books by Genre:" << std::endl;
            for (const auto& [g, count] : genreMap) {
                std::cout << "  " << g << ": " << count << std::endl;
            }
        }
    }

    void browseByGenre() {
        auto genres = catalog.getGenres();
        if (genres.empty()) {
            std::cout << C("No books yet.", COLOR_YELLOW) << std::endl;
            return;
        }
        std::vector<std::string> genreNames;
        for (const auto& [g, _] : genres) genreNames.push_back(g);
        std::sort(genreNames.begin(), genreNames.end());
        std::cout << "Select a genre:" << std::endl;
        for (size_t i = 0; i < genreNames.size(); ++i) {
            std::cout << "  " << (i+1) << ". " << genreNames[i] << " (" << genres[genreNames[i]] << ")" << std::endl;
        }
        std::string choice = ask("Number: ");
        try {
            int idx = std::stoi(choice) - 1;
            if (idx >= 0 && idx < (int)genreNames.size()) {
                std::string selected = genreNames[idx];
                auto results = catalog.search("", selected, "", 0, 9999);
                listBooks(results);
            } else {
                std::cout << C("Invalid selection.", COLOR_RED) << std::endl;
            }
        } catch (...) {
            std::cout << C("Invalid input.", COLOR_RED) << std::endl;
        }
    }

    void listBooks(const std::vector<Book>& books) {
        if (books.empty()) {
            std::cout << C("No books found.", COLOR_YELLOW) << std::endl;
            return;
        }
        std::cout << "\n📖 BOOKS" << std::endl;
        std::cout << C(std::string(60, '─'), COLOR_DIM) << std::endl;
        for (const auto& b : books) {
            std::cout << "  " << b.title << " by " << b.author << " (" << b.year << ") – " << b.genre << " [ISBN: " << (b.isbn.empty() ? "N/A" : b.isbn) << "]" << std::endl;
        }
    }

    void editBook() {
        std::string id = ask("Enter book ID to edit: ");
        Book* book = catalog.getBook(id);
        if (!book) {
            std::cout << C("Book not found.", COLOR_RED) << std::endl;
            return;
        }
        std::cout << "Editing: " << book->title << " by " << book->author << std::endl;
        std::string title = ask("Title (" + book->title + "): ");
        std::string author = ask("Author (" + book->author + "): ");
        std::string genre = ask("Genre (" + book->genre + "): ");
        std::string year_str = ask("Year (" + std::to_string(book->year) + "): ");
        std::string isbn = ask("ISBN (" + book->isbn + "): ");
        std::map<std::string, std::string> updates;
        if (!title.empty()) updates["title"] = title;
        if (!author.empty()) updates["author"] = author;
        if (!genre.empty()) updates["genre"] = genre;
        if (!year_str.empty()) updates["year"] = year_str;
        if (!isbn.empty()) updates["isbn"] = isbn;
        if (catalog.updateBook(id, updates)) {
            std::cout << C("✅ Book updated.", COLOR_GREEN) << std::endl;
        } else {
            std::cout << C("Failed to update.", COLOR_RED) << std::endl;
        }
    }

    void deleteBook() {
        std::string id = ask("Enter book ID to delete: ");
        std::string confirm = ask("Delete book " + id + "? (yes/no): ");
        if (toLower(trim(confirm)) != "yes") return;
        if (catalog.deleteBook(id)) {
            std::cout << C("🗑️  Book deleted.", COLOR_YELLOW) << std::endl;
        } else {
            std::cout << C("Book not found.", COLOR_RED) << std::endl;
        }
    }
};

int main() {
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
    try {
        BookApp app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << C("❌ Unexpected error: ", COLOR_RED) << e.what() << std::endl;
        return 1;
    }
    return 0;
}
