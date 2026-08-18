# BookCatalog.cs
/**
 * 📚 Book Catalog – Organize Your Library by Genre (C# Edition)
 * Features: add, edit, delete, search, genre stats, persistence
 * Requires: .NET 6.0+
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

class BookCatalog
{
    // ─── Colors ────────────────────────────────────────────────────────────

    private static readonly string Reset = "\u001B[0m";
    private static readonly string Bright = "\u001B[1m";
    private static readonly string Dim = "\u001B[2m";
    private static readonly string Red = "\u001B[31m";
    private static readonly string Green = "\u001B[32m";
    private static readonly string Yellow = "\u001B[33m";
    private static readonly string Blue = "\u001B[34m";
    private static readonly string Magenta = "\u001B[35m";
    private static readonly string Cyan = "\u001B[36m";

    private static string C(string text, string color) => color + text + Reset;

    // ─── Data Model ──────────────────────────────────────────────────────

    public class Book
    {
        [JsonPropertyName("id")]
        public string Id { get; set; } = "";
        [JsonPropertyName("title")]
        public string Title { get; set; } = "";
        [JsonPropertyName("author")]
        public string Author { get; set; } = "";
        [JsonPropertyName("genre")]
        public string Genre { get; set; } = "";
        [JsonPropertyName("year")]
        public int Year { get; set; }
        [JsonPropertyName("isbn")]
        public string Isbn { get; set; } = "";
    }

    // ─── Config ────────────────────────────────────────────────────────────

    private static readonly string DataDir = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
        ".book_catalog"
    );
    private static readonly string DataFile = Path.Combine(DataDir, "books.json");

    // ─── Catalog Manager ──────────────────────────────────────────────────

    private class Catalog
    {
        private readonly List<Book> books = new();

        public Catalog()
        {
            Directory.CreateDirectory(DataDir);
            Load();
        }

        private void Load()
        {
            if (!File.Exists(DataFile))
            {
                books.Clear();
                return;
            }
            try
            {
                string json = File.ReadAllText(DataFile);
                var data = JsonSerializer.Deserialize<Dictionary<string, List<Book>>>(json);
                if (data != null && data.ContainsKey("books"))
                {
                    books.Clear();
                    books.AddRange(data["books"]);
                }
            }
            catch
            {
                books.Clear();
            }
        }

        private void Save()
        {
            var data = new Dictionary<string, List<Book>> { ["books"] = books };
            string json = JsonSerializer.Serialize(data, new JsonSerializerOptions { WriteIndented = true });
            File.WriteAllText(DataFile, json);
        }

        public Book AddBook(string title, string author, string genre, int year, string isbn)
        {
            var book = new Book
            {
                Id = Guid.NewGuid().ToString(),
                Title = title,
                Author = author,
                Genre = genre,
                Year = year,
                Isbn = isbn
            };
            books.Add(book);
            Save();
            return book;
        }

        public bool DeleteBook(string id)
        {
            int removed = books.RemoveAll(b => b.Id == id);
            if (removed > 0) { Save(); return true; }
            return false;
        }

        public bool UpdateBook(string id, Book updates)
        {
            var book = books.FirstOrDefault(b => b.Id == id);
            if (book == null) return false;
            if (!string.IsNullOrEmpty(updates.Title)) book.Title = updates.Title;
            if (!string.IsNullOrEmpty(updates.Author)) book.Author = updates.Author;
            if (!string.IsNullOrEmpty(updates.Genre)) book.Genre = updates.Genre;
            if (updates.Year > 0) book.Year = updates.Year;
            if (!string.IsNullOrEmpty(updates.Isbn)) book.Isbn = updates.Isbn;
            Save();
            return true;
        }

        public Book GetBook(string id) => books.FirstOrDefault(b => b.Id == id);

        public List<Book> Search(string query, string genre, string author, int? yearMin, int? yearMax)
        {
            var results = books.AsEnumerable();
            if (!string.IsNullOrEmpty(query))
            {
                var q = query.ToLower();
                results = results.Where(b =>
                    b.Title.ToLower().Contains(q) ||
                    b.Author.ToLower().Contains(q) ||
                    b.Genre.ToLower().Contains(q)
                );
            }
            if (!string.IsNullOrEmpty(genre))
                results = results.Where(b => b.Genre.Equals(genre, StringComparison.OrdinalIgnoreCase));
            if (!string.IsNullOrEmpty(author))
                results = results.Where(b => b.Author.ToLower().Contains(author.ToLower()));
            if (yearMin.HasValue) results = results.Where(b => b.Year >= yearMin.Value);
            if (yearMax.HasValue) results = results.Where(b => b.Year <= yearMax.Value);
            return results.ToList();
        }

        public Dictionary<string, int> GetGenres()
        {
            return books.GroupBy(b => b.Genre)
                        .ToDictionary(g => g.Key, g => g.Count());
        }

        public (int total, int authors, int genres, int? minYear, int? maxYear) GetStats()
        {
            int total = books.Count;
            int authors = books.Select(b => b.Author).Distinct().Count();
            int genres = GetGenres().Count;
            int? minYear = books.Any() ? books.Min(b => b.Year) : (int?)null;
            int? maxYear = books.Any() ? books.Max(b => b.Year) : (int?)null;
            return (total, authors, genres, minYear, maxYear);
        }
    }

    // ─── Main App ──────────────────────────────────────────────────────────

    private readonly Catalog catalog;

    public BookCatalog()
    {
        catalog = new Catalog();
    }

    private string Ask(string prompt)
    {
        Console.Write(prompt);
        return Console.ReadLine()?.Trim() ?? "";
    }

    private int AskInt(string prompt, int def)
    {
        while (true)
        {
            string ans = Ask(prompt);
            if (string.IsNullOrEmpty(ans)) return def;
            if (int.TryParse(ans, out int val)) return val;
            Console.WriteLine(C("❌ Please enter a number.", Red));
        }
    }

    private void ShowMenu()
    {
        Console.WriteLine("\n" + C(new string('═', 50), Cyan));
        Console.WriteLine(C("📚 BOOK CATALOG", Bright + Cyan));
        Console.WriteLine(C(new string('═', 50), Cyan));
        Console.WriteLine($"  Total books: {catalog.books.Count}");
        Console.WriteLine($"  Genres: {catalog.GetGenres().Count}");
        Console.WriteLine(C(new string('═', 50), Cyan));
        Console.WriteLine("  1. 📖 List all books");
        Console.WriteLine("  2. ➕ Add a book");
        Console.WriteLine("  3. 🔍 Search books");
        Console.WriteLine("  4. 📊 Statistics");
        Console.WriteLine("  5. 🗂️  Browse by genre");
        Console.WriteLine("  6. ✏️  Edit a book");
        Console.WriteLine("  7. 🗑️  Delete a book");
        Console.WriteLine("  0. 🚪 Exit");
        Console.WriteLine(C(new string('═', 50), Cyan));
    }

    private void ListBooks(List<Book> books = null)
    {
        if (books == null) books = catalog.books;
        if (!books.Any())
        {
            Console.WriteLine(C("No books found.", Yellow));
            return;
        }
        Console.WriteLine("\n📖 BOOKS");
        Console.WriteLine(C(new string('─', 60), Dim));
        foreach (var b in books)
        {
            Console.WriteLine($"  {b.Title} by {b.Author} ({b.Year}) – {b.Genre} [ISBN: {b.Isbn}]");
        }
    }

    private void AddBook()
    {
        string title = Ask("Title: ");
        string author = Ask("Author: ");
        string genre = Ask("Genre: ");
        int year = AskInt("Year: ", 2000);
        string isbn = Ask("ISBN (optional): ");
        var book = catalog.AddBook(title, author, genre, year, isbn);
        Console.WriteLine(C($"✅ Book '{book.Title}' added with ID {book.Id}", Green));
    }

    private void SearchBooks()
    {
        string query = Ask("Search term (title/author/genre): ");
        string genre = Ask("Filter by genre (optional): ");
        string author = Ask("Filter by author (optional): ");
        int yearMin = AskInt("Minimum year (optional): ", 0);
        int yearMax = AskInt("Maximum year (optional): ", 9999);
        var results = catalog.Search(
            query, genre, author,
            yearMin > 0 ? yearMin : (int?)null,
            yearMax < 9999 ? yearMax : (int?)null
        );
        if (results.Any())
            ListBooks(results);
        else
            Console.WriteLine(C("No books match your search.", Yellow));
    }

    private void ShowStats()
    {
        var (total, authors, genres, minYear, maxYear) = catalog.GetStats();
        var genreMap = catalog.GetGenres();
        Console.WriteLine("\n📊 STATISTICS");
        Console.WriteLine(C(new string('─', 30), Dim));
        Console.WriteLine($"  Total Books: {total}");
        Console.WriteLine($"  Unique Authors: {authors}");
        Console.WriteLine($"  Genres: {genres}");
        Console.WriteLine($"  Oldest Book: {minYear?.ToString() ?? "—"}");
        Console.WriteLine($"  Newest Book: {maxYear?.ToString() ?? "—"}");
        if (genreMap.Any())
        {
            Console.WriteLine("\n📚 Books by Genre:");
            foreach (var g in genreMap.OrderBy(x => x.Key))
                Console.WriteLine($"  {g.Key}: {g.Value}");
        }
    }

    private void BrowseByGenre()
    {
        var genres = catalog.GetGenres();
        if (!genres.Any())
        {
            Console.WriteLine(C("No books yet.", Yellow));
            return;
        }
        var genreNames = genres.Keys.OrderBy(x => x).ToList();
        Console.WriteLine("Select a genre:");
        for (int i = 0; i < genreNames.Count; i++)
            Console.WriteLine($"  {i+1}. {genreNames[i]} ({genres[genreNames[i]]})");
        string choice = Ask("Number: ");
        if (int.TryParse(choice, out int idx) && idx >= 1 && idx <= genreNames.Count)
        {
            string selected = genreNames[idx-1];
            var books = catalog.Search("", selected, "", null, null);
            ListBooks(books);
        }
        else
            Console.WriteLine(C("Invalid selection.", Red));
    }

    private void EditBook()
    {
        string id = Ask("Enter book ID to edit: ");
        var book = catalog.GetBook(id);
        if (book == null)
        {
            Console.WriteLine(C("Book not found.", Red));
            return;
        }
        Console.WriteLine($"Editing: {book.Title} by {book.Author}");
        string title = Ask($"Title ({book.Title}): ");
        string author = Ask($"Author ({book.Author}): ");
        string genre = Ask($"Genre ({book.Genre}): ");
        string yearStr = Ask($"Year ({book.Year}): ");
        string isbn = Ask($"ISBN ({book.Isbn}): ");
        var updates = new Book
        {
            Title = string.IsNullOrEmpty(title) ? book.Title : title,
            Author = string.IsNullOrEmpty(author) ? book.Author : author,
            Genre = string.IsNullOrEmpty(genre) ? book.Genre : genre,
            Year = string.IsNullOrEmpty(yearStr) ? book.Year : int.Parse(yearStr),
            Isbn = string.IsNullOrEmpty(isbn) ? book.Isbn : isbn
        };
        if (catalog.UpdateBook(id, updates))
            Console.WriteLine(C("✅ Book updated.", Green));
        else
            Console.WriteLine(C("Failed to update.", Red));
    }

    private void DeleteBook()
    {
        string id = Ask("Enter book ID to delete: ");
        string confirm = Ask($"Delete book {id}? (yes/no): ");
        if (!confirm.Equals("yes", StringComparison.OrdinalIgnoreCase)) return;
        if (catalog.DeleteBook(id))
            Console.WriteLine(C("🗑️  Book deleted.", Yellow));
        else
            Console.WriteLine(C("Book not found.", Red));
    }

    public void Run()
    {
        Console.Clear();
        Console.WriteLine(C("\n📚 Book Catalog – Organize Your Library", Bright + Cyan));
        Console.WriteLine(C("Manage your books by genre!", Dim));

        while (true)
        {
            ShowMenu();
            string choice = Ask("Your choice: ");
            switch (choice)
            {
                case "1": ListBooks(); break;
                case "2": AddBook(); break;
                case "3": SearchBooks(); break;
                case "4": ShowStats(); break;
                case "5": BrowseByGenre(); break;
                case "6": EditBook(); break;
                case "7": DeleteBook(); break;
                case "0":
                    Console.WriteLine(C("👋 Goodbye!", Cyan));
                    return;
                default:
                    Console.WriteLine(C("❌ Invalid choice.", Red));
                    break;
            }
            if (choice != "0")
            {
                Console.Write("\nPress Enter to continue...");
                Console.ReadLine();
            }
        }
    }

    public static void Main()
    {
        try
        {
            new BookCatalog().Run();
        }
        catch (Exception ex)
        {
            Console.WriteLine(C($"❌ Unexpected error: {ex.Message}", Red));
            Environment.Exit(1);
        }
    }
}
