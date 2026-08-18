# BookCatalog.java
/**
 * 📚 Book Catalog – Organize Your Library by Genre (Java Edition)
 * Features: add, edit, delete, search, genre stats, persistence
 * Requires: Java 17+
 */

import java.io.*;
import java.nio.file.*;
import java.time.*;
import java.util.*;
import java.util.stream.*;

public class BookCatalog {
    // ─── Colors ────────────────────────────────────────────────────────────

    private static final String RESET = "\u001B[0m";
    private static final String BRIGHT = "\u001B[1m";
    private static final String DIM = "\u001B[2m";
    private static final String RED = "\u001B[31m";
    private static final String GREEN = "\u001B[32m";
    private static final String YELLOW = "\u001B[33m";
    private static final String BLUE = "\u001B[34m";
    private static final String MAGENTA = "\u001B[35m";
    private static final String CYAN = "\u001B[36m";

    private static String c(String text, String color) { return color + text + RESET; }

    // ─── Data Model ──────────────────────────────────────────────────────

    private static class Book {
        String id;
        String title;
        String author;
        String genre;
        int year;
        String isbn;

        Book(String id, String title, String author, String genre, int year, String isbn) {
            this.id = id;
            this.title = title;
            this.author = author;
            this.genre = genre;
            this.year = year;
            this.isbn = isbn;
        }

        Map<String, Object> toMap() {
            Map<String, Object> m = new LinkedHashMap<>();
            m.put("id", id);
            m.put("title", title);
            m.put("author", author);
            m.put("genre", genre);
            m.put("year", year);
            m.put("isbn", isbn);
            return m;
        }

        static Book fromMap(Map<String, Object> m) {
            return new Book(
                (String) m.get("id"),
                (String) m.get("title"),
                (String) m.get("author"),
                (String) m.get("genre"),
                (int) m.get("year"),
                (String) m.get("isbn")
            );
        }
    }

    // ─── Config ────────────────────────────────────────────────────────────

    private static final String DATA_DIR = System.getProperty("user.home") + "/.book_catalog";
    private static final String DATA_FILE = DATA_DIR + "/books.json";

    // ─── Catalog Manager ──────────────────────────────────────────────────

    private static class Catalog {
        private List<Book> books = new ArrayList<>();

        Catalog() throws IOException {
            Files.createDirectories(Paths.get(DATA_DIR));
            load();
        }

        private void load() {
            Path path = Paths.get(DATA_FILE);
            if (!Files.exists(path)) {
                books = new ArrayList<>();
                return;
            }
            try {
                String json = Files.readString(path);
                // Simple manual parse (for demo)
                books = parseBooks(json);
            } catch (Exception e) {
                books = new ArrayList<>();
            }
        }

        private List<Book> parseBooks(String json) {
            List<Book> result = new ArrayList<>();
            // Very simple: find book objects
            int idx = json.indexOf("{\"id\":");
            while (idx != -1) {
                int end = json.indexOf("}", idx) + 1;
                String obj = json.substring(idx, end);
                Map<String, Object> map = parseObject(obj);
                if (map != null && map.containsKey("id")) {
                    result.add(Book.fromMap(map));
                }
                idx = json.indexOf("{\"id\":", end);
            }
            return result;
        }

        private Map<String, Object> parseObject(String obj) {
            Map<String, Object> map = new HashMap<>();
            String[] pairs = obj.split(",");
            for (String p : pairs) {
                String[] kv = p.split(":", 2);
                if (kv.length == 2) {
                    String key = kv[0].trim().replaceAll("\"", "");
                    String val = kv[1].trim().replaceAll("\"", "");
                    if (key.equals("year")) {
                        map.put(key, Integer.parseInt(val));
                    } else {
                        map.put(key, val);
                    }
                }
            }
            return map;
        }

        private void save() {
            StringBuilder sb = new StringBuilder();
            sb.append("{\n  \"books\": [\n");
            for (int i = 0; i < books.size(); i++) {
                Book b = books.get(i);
                sb.append("    {\n");
                sb.append("      \"id\": \"").append(escapeJson(b.id)).append("\",\n");
                sb.append("      \"title\": \"").append(escapeJson(b.title)).append("\",\n");
                sb.append("      \"author\": \"").append(escapeJson(b.author)).append("\",\n");
                sb.append("      \"genre\": \"").append(escapeJson(b.genre)).append("\",\n");
                sb.append("      \"year\": ").append(b.year).append(",\n");
                sb.append("      \"isbn\": \"").append(escapeJson(b.isbn)).append("\"\n");
                sb.append("    }");
                if (i < books.size() - 1) sb.append(",");
                sb.append("\n");
            }
            sb.append("  ]\n}");
            try {
                Files.writeString(Paths.get(DATA_FILE), sb.toString());
            } catch (IOException e) { e.printStackTrace(); }
        }

        private String escapeJson(String s) {
            return s.replace("\\", "\\\\").replace("\"", "\\\"");
        }

        String generateId() {
            return UUID.randomUUID().toString();
        }

        Book addBook(String title, String author, String genre, int year, String isbn) {
            Book b = new Book(generateId(), title, author, genre, year, isbn);
            books.add(b);
            save();
            return b;
        }

        boolean deleteBook(String id) {
            Iterator<Book> it = books.iterator();
            while (it.hasNext()) {
                if (it.next().id.equals(id)) {
                    it.remove();
                    save();
                    return true;
                }
            }
            return false;
        }

        boolean updateBook(String id, Map<String, Object> updates) {
            for (Book b : books) {
                if (b.id.equals(id)) {
                    if (updates.containsKey("title")) b.title = (String) updates.get("title");
                    if (updates.containsKey("author")) b.author = (String) updates.get("author");
                    if (updates.containsKey("genre")) b.genre = (String) updates.get("genre");
                    if (updates.containsKey("year")) b.year = (int) updates.get("year");
                    if (updates.containsKey("isbn")) b.isbn = (String) updates.get("isbn");
                    save();
                    return true;
                }
            }
            return false;
        }

        Book getBook(String id) {
            return books.stream().filter(b -> b.id.equals(id)).findFirst().orElse(null);
        }

        List<Book> search(String query, String genre, String author, Integer yearMin, Integer yearMax) {
            return books.stream()
                .filter(b -> {
                    boolean match = true;
                    if (query != null && !query.isEmpty()) {
                        String q = query.toLowerCase();
                        match = match && (b.title.toLowerCase().contains(q) ||
                                          b.author.toLowerCase().contains(q) ||
                                          b.genre.toLowerCase().contains(q));
                    }
                    if (genre != null && !genre.isEmpty()) {
                        match = match && b.genre.equalsIgnoreCase(genre);
                    }
                    if (author != null && !author.isEmpty()) {
                        match = match && b.author.toLowerCase().contains(author.toLowerCase());
                    }
                    if (yearMin != null) match = match && b.year >= yearMin;
                    if (yearMax != null) match = match && b.year <= yearMax;
                    return match;
                })
                .collect(Collectors.toList());
        }

        Map<String, Integer> getGenres() {
            Map<String, Integer> map = new LinkedHashMap<>();
            for (Book b : books) {
                map.put(b.genre, map.getOrDefault(b.genre, 0) + 1);
            }
            return map;
        }

        Map<String, Object> getStats() {
            Map<String, Object> stats = new LinkedHashMap<>();
            stats.put("total", books.size());
            stats.put("authors", books.stream().map(b -> b.author).distinct().count());
            stats.put("genres", getGenres().size());
            OptionalInt minYear = books.stream().mapToInt(b -> b.year).min();
            OptionalInt maxYear = books.stream().mapToInt(b -> b.year).max();
            stats.put("minYear", minYear.isPresent() ? minYear.getAsInt() : null);
            stats.put("maxYear", maxYear.isPresent() ? maxYear.getAsInt() : null);
            return stats;
        }
    }

    // ─── Main App ──────────────────────────────────────────────────────────

    private final Scanner scanner;
    private final Catalog catalog;

    public BookCatalog() throws IOException {
        scanner = new Scanner(System.in);
        catalog = new Catalog();
    }

    private String ask(String prompt) {
        System.out.print(prompt);
        return scanner.nextLine().trim();
    }

    private int askInt(String prompt, int def) {
        while (true) {
            try {
                String ans = ask(prompt);
                if (ans.isEmpty()) return def;
                return Integer.parseInt(ans);
            } catch (NumberFormatException e) {
                System.out.println(c("❌ Please enter a number.", RED));
            }
        }
    }

    private void showMenu() {
        System.out.println("\n" + c("═".repeat(50), CYAN));
        System.out.println(c("📚 BOOK CATALOG", BRIGHT + CYAN));
        System.out.println(c("═".repeat(50), CYAN));
        System.out.println("  Total books: " + catalog.books.size());
        System.out.println("  Genres: " + catalog.getGenres().size());
        System.out.println(c("═".repeat(50), CYAN));
        System.out.println("  1. 📖 List all books");
        System.out.println("  2. ➕ Add a book");
        System.out.println("  3. 🔍 Search books");
        System.out.println("  4. 📊 Statistics");
        System.out.println("  5. 🗂️  Browse by genre");
        System.out.println("  6. ✏️  Edit a book");
        System.out.println("  7. 🗑️  Delete a book");
        System.out.println("  0. 🚪 Exit");
        System.out.println(c("═".repeat(50), CYAN));
    }

    private void listBooks(List<Book> books) {
        if (books == null) books = catalog.books;
        if (books.isEmpty()) {
            System.out.println(c("No books found.", YELLOW));
            return;
        }
        System.out.println("\n📖 BOOKS");
        System.out.println(c("─".repeat(60), DIM));
        for (Book b : books) {
            System.out.printf("  %s by %s (%d) – %s [ISBN: %s]%n", b.title, b.author, b.year, b.genre, b.isbn);
        }
    }

    private void addBook() {
        String title = ask("Title: ");
        String author = ask("Author: ");
        String genre = ask("Genre: ");
        int year = askInt("Year: ", 2000);
        String isbn = ask("ISBN (optional): ");
        Book book = catalog.addBook(title, author, genre, year, isbn);
        System.out.println(c("✅ Book '" + book.title + "' added with ID " + book.id, GREEN));
    }

    private void searchBooks() {
        String query = ask("Search term (title/author/genre): ");
        String genre = ask("Filter by genre (optional): ");
        String author = ask("Filter by author (optional): ");
        int yearMin = askInt("Minimum year (optional): ", 0);
        int yearMax = askInt("Maximum year (optional): ", 9999);
        List<Book> results = catalog.search(
            query.isEmpty() ? null : query,
            genre.isEmpty() ? null : genre,
            author.isEmpty() ? null : author,
            yearMin > 0 ? yearMin : null,
            yearMax < 9999 ? yearMax : null
        );
        if (results.isEmpty()) {
            System.out.println(c("No books match your search.", YELLOW));
        } else {
            listBooks(results);
        }
    }

    private void showStats() {
        var stats = catalog.getStats();
        var genres = catalog.getGenres();
        System.out.println("\n📊 STATISTICS");
        System.out.println(c("─".repeat(30), DIM));
        System.out.println("  Total Books: " + stats.get("total"));
        System.out.println("  Unique Authors: " + stats.get("authors"));
        System.out.println("  Genres: " + stats.get("genres"));
        System.out.println("  Oldest Book: " + (stats.get("minYear") != null ? stats.get("minYear") : "—"));
        System.out.println("  Newest Book: " + (stats.get("maxYear") != null ? stats.get("maxYear") : "—"));
        if (!genres.isEmpty()) {
            System.out.println("\n📚 Books by Genre:");
            for (var e : genres.entrySet()) {
                System.out.println("  " + e.getKey() + ": " + e.getValue());
            }
        }
    }

    private void browseByGenre() {
        var genres = catalog.getGenres();
        if (genres.isEmpty()) {
            System.out.println(c("No books yet.", YELLOW));
            return;
        }
        List<String> genreNames = new ArrayList<>(genres.keySet());
        Collections.sort(genreNames);
        System.out.println("Select a genre:");
        for (int i = 0; i < genreNames.size(); i++) {
            System.out.printf("  %d. %s (%d)%n", i+1, genreNames.get(i), genres.get(genreNames.get(i)));
        }
        String choice = ask("Number: ");
        try {
            int idx = Integer.parseInt(choice) - 1;
            if (idx >= 0 && idx < genreNames.size()) {
                String selected = genreNames.get(idx);
                List<Book> books = catalog.search(null, selected, null, null, null);
                listBooks(books);
            } else {
                System.out.println(c("Invalid selection.", RED));
            }
        } catch (NumberFormatException e) {
            System.out.println(c("Invalid input.", RED));
        }
    }

    private void editBook() {
        String id = ask("Enter book ID to edit: ");
        Book book = catalog.getBook(id);
        if (book == null) {
            System.out.println(c("Book not found.", RED));
            return;
        }
        System.out.println("Editing: " + book.title + " by " + book.author);
        String title = ask("Title (" + book.title + "): ");
        String author = ask("Author (" + book.author + "): ");
        String genre = ask("Genre (" + book.genre + "): ");
        String yearStr = ask("Year (" + book.year + "): ");
        String isbn = ask("ISBN (" + book.isbn + "): ");
        Map<String, Object> updates = new HashMap<>();
        if (!title.isEmpty()) updates.put("title", title);
        if (!author.isEmpty()) updates.put("author", author);
        if (!genre.isEmpty()) updates.put("genre", genre);
        if (!yearStr.isEmpty()) updates.put("year", Integer.parseInt(yearStr));
        if (!isbn.isEmpty()) updates.put("isbn", isbn);
        if (catalog.updateBook(id, updates)) {
            System.out.println(c("✅ Book updated.", GREEN));
        } else {
            System.out.println(c("Failed to update.", RED));
        }
    }

    private void deleteBook() {
        String id = ask("Enter book ID to delete: ");
        String confirm = ask("Delete book " + id + "? (yes/no): ");
        if (!confirm.equalsIgnoreCase("yes")) return;
        if (catalog.deleteBook(id)) {
            System.out.println(c("🗑️  Book deleted.", YELLOW));
        } else {
            System.out.println(c("Book not found.", RED));
        }
    }

    public void run() {
        System.out.print("\033[H\033[2J");
        System.out.flush();
        System.out.println(c("\n📚 Book Catalog – Organize Your Library", BRIGHT + CYAN));
        System.out.println(c("Manage your books by genre!", DIM));

        while (true) {
            showMenu();
            String choice = ask("Your choice: ");
            switch (choice) {
                case "1": listBooks(null); break;
                case "2": addBook(); break;
                case "3": searchBooks(); break;
                case "4": showStats(); break;
                case "5": browseByGenre(); break;
                case "6": editBook(); break;
                case "7": deleteBook(); break;
                case "0":
                    System.out.println(c("👋 Goodbye!", CYAN));
                    return;
                default:
                    System.out.println(c("❌ Invalid choice.", RED));
            }
            if (!choice.equals("0")) {
                System.out.print("\nPress Enter to continue...");
                scanner.nextLine();
            }
        }
    }

    public static void main(String[] args) {
        try {
            new BookCatalog().run();
        } catch (Exception e) {
            System.err.println(c("❌ Unexpected error: " + e.getMessage(), RED));
            e.printStackTrace();
            System.exit(1);
        }
    }
}
