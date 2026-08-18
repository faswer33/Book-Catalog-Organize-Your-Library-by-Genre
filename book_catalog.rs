# book_catalog.rs
/**
 * 📚 Book Catalog – Organize Your Library by Genre (Rust Edition)
 * Features: add, edit, delete, search, genre stats, persistence
 * Dependencies: serde, serde_json, uuid, chrono, colored
 */

use chrono::Local;
use colored::*;
use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};
use std::fs;
use std::io::{self, Write, BufRead};
use std::path::PathBuf;
use uuid::Uuid;

// ─── Data Model ─────────────────────────────────────────────────────────────

#[derive(Debug, Serialize, Deserialize, Clone)]
struct Book {
    id: String,
    title: String,
    author: String,
    genre: String,
    year: i32,
    isbn: String,
}

#[derive(Debug, Serialize, Deserialize)]
struct CatalogData {
    books: Vec<Book>,
}

// ─── Colors ──────────────────────────────────────────────────────────────────

fn c(text: &str, color: &str) -> String {
    match color {
        "green" => text.green().to_string(),
        "red" => text.red().to_string(),
        "yellow" => text.yellow().to_string(),
        "cyan" => text.cyan().to_string(),
        "bright" => text.bright().to_string(),
        "dim" => text.dimmed().to_string(),
        _ => text.to_string(),
    }
}

// ─── Catalog Manager ──────────────────────────────────────────────────────

struct BookCatalog {
    books: Vec<Book>,
    file_path: PathBuf,
}

impl BookCatalog {
    fn new() -> Self {
        let home = std::env::var("HOME").or_else(|_| std::env::var("USERPROFILE")).unwrap_or_else(|_| ".".to_string());
        let dir = PathBuf::from(home).join(".book_catalog");
        fs::create_dir_all(&dir).unwrap();
        let file_path = dir.join("books.json");
        let mut c = BookCatalog { books: Vec::new(), file_path };
        c.load();
        c
    }

    fn load(&mut self) {
        if let Ok(raw) = fs::read_to_string(&self.file_path) {
            if let Ok(data) = serde_json::from_str::<CatalogData>(&raw) {
                self.books = data.books;
                return;
            }
        }
        self.books = Vec::new();
    }

    fn save(&self) {
        let data = CatalogData { books: self.books.clone() };
        let raw = serde_json::to_string_pretty(&data).unwrap();
        let _ = fs::write(&self.file_path, raw);
    }

    fn add_book(&mut self, title: String, author: String, genre: String, year: i32, isbn: String) -> Book {
        let book = Book {
            id: Uuid::new_v4().to_string(),
            title,
            author,
            genre,
            year,
            isbn,
        };
        self.books.push(book.clone());
        self.save();
        book
    }

    fn delete_book(&mut self, id: &str) -> bool {
        let idx = self.books.iter().position(|b| b.id == id);
        if let Some(i) = idx {
            self.books.remove(i);
            self.save();
            true
        } else {
            false
        }
    }

    fn update_book(&mut self, id: &str, updates: HashMap<String, String>) -> bool {
        for book in &mut self.books {
            if book.id == id {
                if let Some(v) = updates.get("title") { book.title = v.clone(); }
                if let Some(v) = updates.get("author") { book.author = v.clone(); }
                if let Some(v) = updates.get("genre") { book.genre = v.clone(); }
                if let Some(v) = updates.get("year") {
                    if let Ok(y) = v.parse::<i32>() { book.year = y; }
                }
                if let Some(v) = updates.get("isbn") { book.isbn = v.clone(); }
                self.save();
                return true;
            }
        }
        false
    }

    fn get_book(&self, id: &str) -> Option<&Book> {
        self.books.iter().find(|b| b.id == id)
    }

    fn search(&self, query: Option<&str>, genre: Option<&str>, author: Option<&str>,
               year_min: Option<i32>, year_max: Option<i32>) -> Vec<Book> {
        let mut results = self.books.clone();
        if let Some(q) = query {
            let q = q.to_lowercase();
            results.retain(|b| {
                b.title.to_lowercase().contains(&q) ||
                b.author.to_lowercase().contains(&q) ||
                b.genre.to_lowercase().contains(&q)
            });
        }
        if let Some(g) = genre {
            let g = g.to_lowercase();
            results.retain(|b| b.genre.to_lowercase() == g);
        }
        if let Some(a) = author {
            let a = a.to_lowercase();
            results.retain(|b| b.author.to_lowercase().contains(&a));
        }
        if let Some(ymin) = year_min {
            results.retain(|b| b.year >= ymin);
        }
        if let Some(ymax) = year_max {
            results.retain(|b| b.year <= ymax);
        }
        results
    }

    fn get_genres(&self) -> HashMap<String, usize> {
        let mut map = HashMap::new();
        for b in &self.books {
            *map.entry(b.genre.clone()).or_insert(0) += 1;
        }
        map
    }

    fn get_stats(&self) -> (usize, usize, usize, Option<i32>, Option<i32>) {
        let total = self.books.len();
        let authors: HashSet<_> = self.books.iter().map(|b| &b.author).collect();
        let genres = self.get_genres().len();
        let years: Vec<i32> = self.books.iter().map(|b| b.year).collect();
        let min_year = years.iter().min().copied();
        let max_year = years.iter().max().copied();
        (total, authors.len(), genres, min_year, max_year)
    }
}

// ─── Main App ──────────────────────────────────────────────────────────────

struct BookApp {
    catalog: BookCatalog,
}

impl BookApp {
    fn new() -> Self {
        BookApp { catalog: BookCatalog::new() }
    }

    fn ask(&self, prompt: &str) -> String {
        print!("{}", prompt);
        io::stdout().flush().unwrap();
        let mut line = String::new();
        io::stdin().read_line(&mut line).unwrap();
        line.trim().to_string()
    }

    fn ask_int(&self, prompt: &str, def: i32) -> i32 {
        loop {
            let ans = self.ask(prompt);
            if ans.is_empty() { return def; }
            if let Ok(val) = ans.parse::<i32>() {
                return val;
            }
            println!("{}", c("❌ Please enter a number.", "red"));
        }
    }

    fn show_menu(&self) {
        println!("\n{}", "═".repeat(50).cyan());
        println!("{}", "📚 BOOK CATALOG".bright().cyan());
        println!("{}", "═".repeat(50).cyan());
        println!("  Total books: {}", self.catalog.books.len());
        println!("  Genres: {}", self.catalog.get_genres().len());
        println!("{}", "═".repeat(50).cyan());
        println!("  1. 📖 List all books");
        println!("  2. ➕ Add a book");
        println!("  3. 🔍 Search books");
        println!("  4. 📊 Statistics");
        println!("  5. 🗂️  Browse by genre");
        println!("  6. ✏️  Edit a book");
        println!("  7. 🗑️  Delete a book");
        println!("  0. 🚪 Exit");
        println!("{}", "═".repeat(50).cyan());
    }

    fn list_books(&self, books: Option<&[Book]>) {
        let target = match books {
            Some(b) => b,
            None => &self.catalog.books,
        };
        if target.is_empty() {
            println!("{}", c("No books found.", "yellow"));
            return;
        }
        println!("\n📖 BOOKS");
        println!("{}", "─".repeat(60).dimmed());
        for b in target {
            println!("  {} by {} ({}) – {} [ISBN: {}]", b.title, b.author, b.year, b.genre, b.isbn);
        }
    }

    fn add_book(&mut self) {
        let title = self.ask("Title: ");
        let author = self.ask("Author: ");
        let genre = self.ask("Genre: ");
        let year = self.ask_int("Year: ", 2000);
        let isbn = self.ask("ISBN (optional): ");
        let book = self.catalog.add_book(title, author, genre, year, isbn);
        println!("{}", c(&format!("✅ Book '{}' added with ID {}", book.title, book.id), "green"));
    }

    fn search_books(&self) {
        let query = self.ask("Search term (title/author/genre): ");
        let genre = self.ask("Filter by genre (optional): ");
        let author = self.ask("Filter by author (optional): ");
        let year_min = self.ask_int("Minimum year (optional): ", 0);
        let year_max = self.ask_int("Maximum year (optional): ", 9999);
        let q = if query.is_empty() { None } else { Some(query.as_str()) };
        let g = if genre.is_empty() { None } else { Some(genre.as_str()) };
        let a = if author.is_empty() { None } else { Some(author.as_str()) };
        let ymin = if year_min > 0 { Some(year_min) } else { None };
        let ymax = if year_max < 9999 { Some(year_max) } else { None };
        let results = self.catalog.search(q, g, a, ymin, ymax);
        if results.is_empty() {
            println!("{}", c("No books match your search.", "yellow"));
        } else {
            self.list_books(Some(&results));
        }
    }

    fn show_stats(&self) {
        let (total, authors, genres, min_year, max_year) = self.catalog.get_stats();
        let genre_map = self.catalog.get_genres();
        println!("\n📊 STATISTICS");
        println!("{}", "─".repeat(30).dimmed());
        println!("  Total Books: {}", total);
        println!("  Unique Authors: {}", authors);
        println!("  Genres: {}", genres);
        if let (Some(min), Some(max)) = (min_year, max_year) {
            println!("  Oldest Book: {}", min);
            println!("  Newest Book: {}", max);
        } else {
            println!("  Oldest Book: —");
            println!("  Newest Book: —");
        }
        if !genre_map.is_empty() {
            println!("\n📚 Books by Genre:");
            let mut sorted: Vec<_> = genre_map.iter().collect();
            sorted.sort_by_key(|(k, _)| *k);
            for (g, count) in sorted {
                println!("  {}: {}", g, count);
            }
        }
    }

    fn browse_by_genre(&self) {
        let genres = self.catalog.get_genres();
        if genres.is_empty() {
            println!("{}", c("No books yet.", "yellow"));
            return;
        }
        let mut genre_names: Vec<_> = genres.keys().collect();
        genre_names.sort();
        println!("Select a genre:");
        for (i, &g) in genre_names.iter().enumerate() {
            println!("  {}. {} ({})", i+1, g, genres[g]);
        }
        let choice = self.ask("Number: ");
        if let Ok(idx) = choice.parse::<usize>() {
            if idx >= 1 && idx <= genre_names.len() {
                let selected = genre_names[idx-1];
                let books = self.catalog.search(None, Some(selected), None, None, None);
                self.list_books(Some(&books));
                return;
            }
        }
        println!("{}", c("Invalid selection.", "red"));
    }

    fn edit_book(&mut self) {
        let id = self.ask("Enter book ID to edit: ");
        if let Some(book) = self.catalog.get_book(&id).cloned() {
            println!("Editing: {} by {}", book.title, book.author);
            let title = self.ask(&format!("Title ({}): ", book.title));
            let author = self.ask(&format!("Author ({}): ", book.author));
            let genre = self.ask(&format!("Genre ({}): ", book.genre));
            let year = self.ask_int(&format!("Year ({}): ", book.year), book.year);
            let isbn = self.ask(&format!("ISBN ({}): ", book.isbn));
            let mut updates = HashMap::new();
            if !title.is_empty() { updates.insert("title".to_string(), title); }
            if !author.is_empty() { updates.insert("author".to_string(), author); }
            if !genre.is_empty() { updates.insert("genre".to_string(), genre); }
            updates.insert("year".to_string(), year.to_string());
            if !isbn.is_empty() { updates.insert("isbn".to_string(), isbn); }
            if self.catalog.update_book(&id, updates) {
                println!("{}", c("✅ Book updated.", "green"));
            } else {
                println!("{}", c("Failed to update.", "red"));
            }
        } else {
            println!("{}", c("Book not found.", "red"));
        }
    }

    fn delete_book(&mut self) {
        let id = self.ask("Enter book ID to delete: ");
        let confirm = self.ask(&format!("Delete book {}? (yes/no): ", id));
        if confirm.to_lowercase() != "yes" { return; }
        if self.catalog.delete_book(&id) {
            println!("{}", c("🗑️  Book deleted.", "yellow"));
        } else {
            println!("{}", c("Book not found.", "red"));
        }
    }

    fn run(&mut self) {
        println!("{}", "\n📚 Book Catalog – Organize Your Library".bright().cyan());
        println!("{}", "Manage your books by genre!".dimmed());

        loop {
            self.show_menu();
            let choice = self.ask("Your choice: ");
            match choice.as_str() {
                "1" => self.list_books(None),
                "2" => self.add_book(),
                "3" => self.search_books(),
                "4" => self.show_stats(),
                "5" => self.browse_by_genre(),
                "6" => self.edit_book(),
                "7" => self.delete_book(),
                "0" => {
                    println!("{}", c("👋 Goodbye!", "cyan"));
                    return;
                }
                _ => println!("{}", c("❌ Invalid choice.", "red")),
            }
            if choice != "0" {
                print!("\nPress Enter to continue...");
                io::stdout().flush().unwrap();
                let mut _dummy = String::new();
                io::stdin().read_line(&mut _dummy).unwrap();
            }
        }
    }
}

// ─── Main ────────────────────────────────────────────────────────────────────

fn main() {
    let mut app = BookApp::new();
    app.run();
}
