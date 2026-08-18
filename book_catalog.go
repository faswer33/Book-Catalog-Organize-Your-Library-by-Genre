# book_catalog.go
/**
 * 📚 Book Catalog – Organize Your Library by Genre (Go Edition)
 * Features: add, edit, delete, search, genre stats, persistence
 */

package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"
	"github.com/google/uuid"
)

// ─── Data Model ─────────────────────────────────────────────────────────────

type Book struct {
	ID     string `json:"id"`
	Title  string `json:"title"`
	Author string `json:"author"`
	Genre  string `json:"genre"`
	Year   int    `json:"year"`
	ISBN   string `json:"isbn"`
}

type CatalogData struct {
	Books []Book `json:"books"`
}

// ─── Colors ──────────────────────────────────────────────────────────────────

const (
	reset  = "\x1b[0m"
	bright = "\x1b[1m"
	dim    = "\x1b[2m"
	red    = "\x1b[31m"
	green  = "\x1b[32m"
	yellow = "\x1b[33m"
	blue   = "\x1b[34m"
	magenta = "\x1b[35m"
	cyan   = "\x1b[36m"
)

func c(str, color string) string {
	return color + str + reset
}

// ─── Catalog Manager ──────────────────────────────────────────────────────

type BookCatalog struct {
	books    []Book
	filePath string
}

func NewBookCatalog() *BookCatalog {
	home, _ := os.UserHomeDir()
	dir := filepath.Join(home, ".book_catalog")
	os.MkdirAll(dir, 0755)
	filePath := filepath.Join(dir, "books.json")
	c := &BookCatalog{filePath: filePath}
	c.load()
	return c
}

func (c *BookCatalog) load() {
	if _, err := os.Stat(c.filePath); os.IsNotExist(err) {
		c.books = []Book{}
		return
	}
	raw, err := os.ReadFile(c.filePath)
	if err != nil {
		c.books = []Book{}
		return
	}
	var data CatalogData
	if err := json.Unmarshal(raw, &data); err != nil {
		c.books = []Book{}
		return
	}
	c.books = data.Books
}

func (c *BookCatalog) save() {
	data := CatalogData{Books: c.books}
	raw, _ := json.MarshalIndent(data, "", "  ")
	os.WriteFile(c.filePath, raw, 0644)
}

func (c *BookCatalog) AddBook(title, author, genre string, year int, isbn string) Book {
	book := Book{
		ID:     uuid.New().String(),
		Title:  title,
		Author: author,
		Genre:  genre,
		Year:   year,
		ISBN:   isbn,
	}
	c.books = append(c.books, book)
	c.save()
	return book
}

func (c *BookCatalog) DeleteBook(id string) bool {
	for i, b := range c.books {
		if b.ID == id {
			c.books = append(c.books[:i], c.books[i+1:]...)
			c.save()
			return true
		}
	}
	return false
}

func (c *BookCatalog) UpdateBook(id string, updates map[string]interface{}) bool {
	for i, b := range c.books {
		if b.ID == id {
			if title, ok := updates["title"].(string); ok {
				c.books[i].Title = title
			}
			if author, ok := updates["author"].(string); ok {
				c.books[i].Author = author
			}
			if genre, ok := updates["genre"].(string); ok {
				c.books[i].Genre = genre
			}
			if year, ok := updates["year"].(int); ok {
				c.books[i].Year = year
			}
			if isbn, ok := updates["isbn"].(string); ok {
				c.books[i].ISBN = isbn
			}
			c.save()
			return true
		}
	}
	return false
}

func (c *BookCatalog) GetBook(id string) *Book {
	for _, b := range c.books {
		if b.ID == id {
			return &b
		}
	}
	return nil
}

func (c *BookCatalog) Search(query, genre, author string, yearMin, yearMax *int) []Book {
	results := c.books
	if query != "" {
		q := strings.ToLower(query)
		results = filterBooks(results, func(b Book) bool {
			return strings.Contains(strings.ToLower(b.Title), q) ||
				strings.Contains(strings.ToLower(b.Author), q) ||
				strings.Contains(strings.ToLower(b.Genre), q)
		})
	}
	if genre != "" {
		g := strings.ToLower(genre)
		results = filterBooks(results, func(b Book) bool {
			return strings.ToLower(b.Genre) == g
		})
	}
	if author != "" {
		a := strings.ToLower(author)
		results = filterBooks(results, func(b Book) bool {
			return strings.Contains(strings.ToLower(b.Author), a)
		})
	}
	if yearMin != nil {
		results = filterBooks(results, func(b Book) bool { return b.Year >= *yearMin })
	}
	if yearMax != nil {
		results = filterBooks(results, func(b Book) bool { return b.Year <= *yearMax })
	}
	return results
}

func filterBooks(books []Book, fn func(Book) bool) []Book {
	var res []Book
	for _, b := range books {
		if fn(b) {
			res = append(res, b)
		}
	}
	return res
}

func (c *BookCatalog) GetGenres() map[string]int {
	m := make(map[string]int)
	for _, b := range c.books {
		m[b.Genre]++
	}
	return m
}

func (c *BookCatalog) GetStats() (total, authors, genres int, minYear, maxYear *int) {
	total = len(c.books)
	authorSet := make(map[string]bool)
	for _, b := range c.books {
		authorSet[b.Author] = true
	}
	authors = len(authorSet)
	genres = len(c.GetGenres())
	if total > 0 {
		minY, maxY := c.books[0].Year, c.books[0].Year
		for _, b := range c.books {
			if b.Year < minY {
				minY = b.Year
			}
			if b.Year > maxY {
				maxY = b.Year
			}
		}
		minYear = &minY
		maxYear = &maxY
	}
	return
}

// ─── Main App ──────────────────────────────────────────────────────────────

type BookApp struct {
	reader   *bufio.Reader
	catalog  *BookCatalog
}

func NewBookApp() *BookApp {
	return &BookApp{
		reader:  bufio.NewReader(os.Stdin),
		catalog: NewBookCatalog(),
	}
}

func (app *BookApp) ask(prompt string) string {
	fmt.Print(prompt)
	line, _ := app.reader.ReadString('\n')
	return strings.TrimSpace(line)
}

func (app *BookApp) askInt(prompt string, def int) int {
	for {
		ans := app.ask(prompt)
		if ans == "" {
			return def
		}
		if val, err := strconv.Atoi(ans); err == nil {
			return val
		}
		fmt.Println(c("❌ Please enter a number.", red))
	}
}

func (app *BookApp) showMenu() {
	fmt.Println("\n" + c(strings.Repeat("═", 50), cyan))
	fmt.Println(c("📚 BOOK CATALOG", bright+cyan))
	fmt.Println(c(strings.Repeat("═", 50), cyan))
	fmt.Printf("  Total books: %d\n", len(app.catalog.books))
	fmt.Printf("  Genres: %d\n", len(app.catalog.GetGenres()))
	fmt.Println(c(strings.Repeat("═", 50), cyan))
	fmt.Println("  1. 📖 List all books")
	fmt.Println("  2. ➕ Add a book")
	fmt.Println("  3. 🔍 Search books")
	fmt.Println("  4. 📊 Statistics")
	fmt.Println("  5. 🗂️  Browse by genre")
	fmt.Println("  6. ✏️  Edit a book")
	fmt.Println("  7. 🗑️  Delete a book")
	fmt.Println("  0. 🚪 Exit")
	fmt.Println(c(strings.Repeat("═", 50), cyan))
}

func (app *BookApp) listBooks(books []Book) {
	if books == nil {
		books = app.catalog.books
	}
	if len(books) == 0 {
		fmt.Println(c("No books found.", yellow))
		return
	}
	fmt.Println("\n📖 BOOKS")
	fmt.Println(c(strings.Repeat("─", 60), dim))
	for _, b := range books {
		fmt.Printf("  %s by %s (%d) – %s [ISBN: %s]\n", b.Title, b.Author, b.Year, b.Genre, b.ISBN)
	}
}

func (app *BookApp) addBook() {
	title := app.ask("Title: ")
	author := app.ask("Author: ")
	genre := app.ask("Genre: ")
	year := app.askInt("Year: ", 2000)
	isbn := app.ask("ISBN (optional): ")
	book := app.catalog.AddBook(title, author, genre, year, isbn)
	fmt.Printf(c("✅ Book '%s' added with ID %s\n", green), book.Title, book.ID)
}

func (app *BookApp) searchBooks() {
	query := app.ask("Search term (title/author/genre): ")
	genre := app.ask("Filter by genre (optional): ")
	author := app.ask("Filter by author (optional): ")
	yearMin := app.askInt("Minimum year (optional): ", 0)
	yearMax := app.askInt("Maximum year (optional): ", 9999)
	var yMin, yMax *int
	if yearMin > 0 {
		yMin = &yearMin
	}
	if yearMax < 9999 {
		yMax = &yearMax
	}
	results := app.catalog.Search(query, genre, author, yMin, yMax)
	if len(results) > 0 {
		app.listBooks(results)
	} else {
		fmt.Println(c("No books match your search.", yellow))
	}
}

func (app *BookApp) showStats() {
	total, authors, genres, minYear, maxYear := app.catalog.GetStats()
	genreMap := app.catalog.GetGenres()
	fmt.Println("\n📊 STATISTICS")
	fmt.Println(c(strings.Repeat("─", 30), dim))
	fmt.Printf("  Total Books: %d\n", total)
	fmt.Printf("  Unique Authors: %d\n", authors)
	fmt.Printf("  Genres: %d\n", genres)
	if minYear != nil && maxYear != nil {
		fmt.Printf("  Oldest Book: %d\n", *minYear)
		fmt.Printf("  Newest Book: %d\n", *maxYear)
	} else {
		fmt.Println("  Oldest Book: —")
		fmt.Println("  Newest Book: —")
	}
	if len(genreMap) > 0 {
		fmt.Println("\n📚 Books by Genre:")
		for g, count := range genreMap {
			fmt.Printf("  %s: %d\n", g, count)
		}
	}
}

func (app *BookApp) browseByGenre() {
	genres := app.catalog.GetGenres()
	if len(genres) == 0 {
		fmt.Println(c("No books yet.", yellow))
		return
	}
	genreNames := make([]string, 0, len(genres))
	for g := range genres {
		genreNames = append(genreNames, g)
	}
	fmt.Println("Select a genre:")
	for i, g := range genreNames {
		fmt.Printf("  %d. %s (%d)\n", i+1, g, genres[g])
	}
	choice := app.ask("Number: ")
	idx, err := strconv.Atoi(choice)
	if err == nil && idx >= 1 && idx <= len(genreNames) {
		selected := genreNames[idx-1]
		books := app.catalog.Search("", selected, "", nil, nil)
		app.listBooks(books)
	} else {
		fmt.Println(c("Invalid selection.", red))
	}
}

func (app *BookApp) editBook() {
	id := app.ask("Enter book ID to edit: ")
	book := app.catalog.GetBook(id)
	if book == nil {
		fmt.Println(c("Book not found.", red))
		return
	}
	fmt.Printf("Editing: %s by %s\n", book.Title, book.Author)
	title := app.ask("Title (" + book.Title + "): ")
	if title == "" {
		title = book.Title
	}
	author := app.ask("Author (" + book.Author + "): ")
	if author == "" {
		author = book.Author
	}
	genre := app.ask("Genre (" + book.Genre + "): ")
	if genre == "" {
		genre = book.Genre
	}
	year := app.askInt("Year ("+strconv.Itoa(book.Year)+"): ", book.Year)
	isbn := app.ask("ISBN (" + book.ISBN + "): ")
	if isbn == "" {
		isbn = book.ISBN
	}
	updates := map[string]interface{}{
		"title": title, "author": author, "genre": genre,
		"year": year, "isbn": isbn,
	}
	if app.catalog.UpdateBook(id, updates) {
		fmt.Println(c("✅ Book updated.", green))
	} else {
		fmt.Println(c("Failed to update.", red))
	}
}

func (app *BookApp) deleteBook() {
	id := app.ask("Enter book ID to delete: ")
	confirm := app.ask("Delete book " + id + "? (yes/no): ")
	if strings.ToLower(confirm) != "yes" {
		return
	}
	if app.catalog.DeleteBook(id) {
		fmt.Println(c("🗑️  Book deleted.", yellow))
	} else {
		fmt.Println(c("Book not found.", red))
	}
}

func (app *BookApp) run() {
	fmt.Print("\033[H\033[2J")
	fmt.Println(c("\n📚 Book Catalog – Organize Your Library", bright+cyan))
	fmt.Println(c("Manage your books by genre!", dim))

	for {
		app.showMenu()
		choice := app.ask("Your choice: ")
		switch choice {
		case "1":
			app.listBooks(nil)
		case "2":
			app.addBook()
		case "3":
			app.searchBooks()
		case "4":
			app.showStats()
		case "5":
			app.browseByGenre()
		case "6":
			app.editBook()
		case "7":
			app.deleteBook()
		case "0":
			fmt.Println(c("👋 Goodbye!", cyan))
			return
		default:
			fmt.Println(c("❌ Invalid choice.", red))
		}
		if choice != "0" {
			fmt.Print("\nPress Enter to continue...")
			app.reader.ReadString('\n')
		}
	}
}

func main() {
	app := NewBookApp()
	app.run()
}
