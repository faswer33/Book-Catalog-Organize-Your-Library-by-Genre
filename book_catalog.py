# book_catalog.py
#!/usr/bin/env python3
"""
📚 Book Catalog – Organize Your Library by Genre (Python Edition)
Features: add, edit, delete, search by title/author/genre/year, genre stats, persistence
"""

import json
import os
import sys
import uuid
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Optional

try:
    from rich.console import Console
    from rich.table import Table
    from rich.panel import Panel
    from rich.prompt import Prompt, IntPrompt, Confirm
    from rich import box
    RICH_AVAILABLE = True
except ImportError:
    RICH_AVAILABLE = False
    print("⚠️  Install 'rich' for enhanced UI: pip install rich")


# ─── Colors ──────────────────────────────────────────────────────────────────

def c(text: str, color: str) -> str:
    colors = {
        "reset": "\033[0m", "bright": "\033[1m", "dim": "\033[2m",
        "red": "\033[31m", "green": "\033[32m", "yellow": "\033[33m",
        "blue": "\033[34m", "magenta": "\033[35m", "cyan": "\033[36m"
    }
    return f"{colors.get(color, '')}{text}{colors['reset']}"


# ─── Data Model ─────────────────────────────────────────────────────────────

class Book:
    def __init__(self, title: str, author: str, genre: str, year: int, isbn: str = "", book_id: str = None):
        self.id = book_id or str(uuid.uuid4())
        self.title = title
        self.author = author
        self.genre = genre
        self.year = year
        self.isbn = isbn

    def to_dict(self) -> Dict:
        return {
            "id": self.id,
            "title": self.title,
            "author": self.author,
            "genre": self.genre,
            "year": self.year,
            "isbn": self.isbn
        }

    @classmethod
    def from_dict(cls, data: Dict) -> 'Book':
        return cls(
            title=data["title"],
            author=data["author"],
            genre=data["genre"],
            year=data["year"],
            isbn=data.get("isbn", ""),
            book_id=data.get("id")
        )


# ─── Catalog Manager ───────────────────────────────────────────────────────

class BookCatalog:
    DATA_DIR = Path.home() / ".book_catalog"
    DATA_FILE = DATA_DIR / "books.json"

    def __init__(self):
        self.console = Console() if RICH_AVAILABLE else None
        self.books: List[Book] = []
        self._load()

    def _load(self):
        if self.DATA_FILE.exists():
            try:
                with open(self.DATA_FILE, 'r') as f:
                    data = json.load(f)
                    self.books = [Book.from_dict(b) for b in data.get("books", [])]
            except Exception:
                self.books = []
        else:
            self.books = []

    def _save(self):
        self.DATA_DIR.mkdir(parents=True, exist_ok=True)
        data = {"books": [b.to_dict() for b in self.books]}
        with open(self.DATA_FILE, 'w') as f:
            json.dump(data, f, indent=2)

    def add_book(self, title: str, author: str, genre: str, year: int, isbn: str = "") -> Book:
        book = Book(title, author, genre, year, isbn)
        self.books.append(book)
        self._save()
        return book

    def delete_book(self, book_id: str) -> bool:
        for i, b in enumerate(self.books):
            if b.id == book_id:
                del self.books[i]
                self._save()
                return True
        return False

    def update_book(self, book_id: str, **kwargs) -> bool:
        for b in self.books:
            if b.id == book_id:
                for key, val in kwargs.items():
                    if hasattr(b, key):
                        setattr(b, key, val)
                self._save()
                return True
        return False

    def get_book(self, book_id: str) -> Optional[Book]:
        for b in self.books:
            if b.id == book_id:
                return b
        return None

    def search(self, query: str = "", genre: str = "", author: str = "", year_min: int = None, year_max: int = None) -> List[Book]:
        results = self.books
        if query:
            q = query.lower()
            results = [b for b in results if q in b.title.lower() or q in b.author.lower() or q in b.genre.lower()]
        if genre:
            results = [b for b in results if b.genre.lower() == genre.lower()]
        if author:
            results = [b for b in results if author.lower() in b.author.lower()]
        if year_min is not None:
            results = [b for b in results if b.year >= year_min]
        if year_max is not None:
            results = [b for b in results if b.year <= year_max]
        return results

    def get_genres(self) -> Dict[str, int]:
        genre_counts = {}
        for b in self.books:
            genre_counts[b.genre] = genre_counts.get(b.genre, 0) + 1
        return genre_counts

    def get_stats(self) -> Dict:
        total = len(self.books)
        authors = set(b.author for b in self.books)
        years = [b.year for b in self.books]
        return {
            "total": total,
            "authors": len(authors),
            "genres": len(self.get_genres()),
            "min_year": min(years) if years else None,
            "max_year": max(years) if years else None
        }


# ─── Main App ──────────────────────────────────────────────────────────────

class BookApp:
    def __init__(self):
        self.console = Console() if RICH_AVAILABLE else None
        self.catalog = BookCatalog()

    def show_menu(self):
        if self.console:
            panel = Panel(
                f"[bold cyan]📚 Book Catalog[/bold cyan]\n"
                f"  Total books: {len(self.catalog.books)}\n"
                f"  Genres: {len(self.catalog.get_genres())}",
                title="📋 Main Menu",
                border_style="blue"
            )
            self.console.print(panel)
            self.console.print(" [1] 📖 List all books")
            self.console.print(" [2] ➕ Add a book")
            self.console.print(" [3] 🔍 Search books")
            self.console.print(" [4] 📊 Statistics")
            self.console.print(" [5] 🗂️  Browse by genre")
            self.console.print(" [6] ✏️  Edit a book")
            self.console.print(" [7] 🗑️  Delete a book")
            self.console.print(" [0] 🚪 Exit")
        else:
            print("\n" + "="*50)
            print(c("📚 BOOK CATALOG", "bright"))
            print("="*50)
            print(f"  Total books: {len(self.catalog.books)}")
            print(f"  Genres: {len(self.catalog.get_genres())}")
            print("="*50)
            print("  1. 📖 List all books")
            print("  2. ➕ Add a book")
            print("  3. 🔍 Search books")
            print("  4. 📊 Statistics")
            print("  5. 🗂️  Browse by genre")
            print("  6. ✏️  Edit a book")
            print("  7. 🗑️  Delete a book")
            print("  0. 🚪 Exit")
            print("="*50)

    def list_books(self, books: List[Book] = None):
        if books is None:
            books = self.catalog.books
        if not books:
            print(c("No books found.", "yellow"))
            return
        if self.console:
            table = Table(title="📖 Books", box=box.ROUNDED)
            table.add_column("Title", style="green")
            table.add_column("Author", style="cyan")
            table.add_column("Genre", style="yellow")
            table.add_column("Year", justify="right")
            table.add_column("ISBN", style="dim")
            for b in books:
                table.add_row(b.title, b.author, b.genre, str(b.year), b.isbn)
            self.console.print(table)
        else:
            print("\n📖 BOOKS")
            print(c("─"*60, "dim"))
            for b in books:
                print(f"  {b.title} by {b.author} ({b.year}) – {b.genre} [ISBN: {b.isbn or 'N/A'}]")

    def add_book(self):
        if self.console:
            title = Prompt.ask("Title")
            author = Prompt.ask("Author")
            genre = Prompt.ask("Genre")
            year = IntPrompt.ask("Year", default=2000)
            isbn = Prompt.ask("ISBN (optional)", default="")
        else:
            title = input("Title: ").strip()
            author = input("Author: ").strip()
            genre = input("Genre: ").strip()
            try:
                year = int(input("Year: ").strip() or 2000)
            except ValueError:
                year = 2000
            isbn = input("ISBN (optional): ").strip()
        book = self.catalog.add_book(title, author, genre, year, isbn)
        print(c(f"✅ Book '{book.title}' added with ID {book.id}", "green"))

    def search_books(self):
        if self.console:
            query = Prompt.ask("Search term (title/author/genre)", default="")
            genre = Prompt.ask("Filter by genre (optional)", default="")
            author = Prompt.ask("Filter by author (optional)", default="")
            year_min = IntPrompt.ask("Minimum year (optional)", default=0)
            year_max = IntPrompt.ask("Maximum year (optional)", default=9999)
        else:
            query = input("Search term (title/author/genre): ").strip()
            genre = input("Filter by genre (optional): ").strip()
            author = input("Filter by author (optional): ").strip()
            try:
                year_min = int(input("Minimum year (optional): ").strip() or 0)
            except ValueError:
                year_min = 0
            try:
                year_max = int(input("Maximum year (optional): ").strip() or 9999)
            except ValueError:
                year_max = 9999
        results = self.catalog.search(query, genre, author, year_min if year_min > 0 else None, year_max if year_max < 9999 else None)
        if results:
            self.list_books(results)
        else:
            print(c("No books match your search.", "yellow"))

    def show_stats(self):
        stats = self.catalog.get_stats()
        genres = self.catalog.get_genres()
        if self.console:
            stat_table = Table(title="📊 Statistics", box=box.ROUNDED)
            stat_table.add_column("Metric", style="cyan")
            stat_table.add_column("Value", style="green")
            stat_table.add_row("Total Books", str(stats["total"]))
            stat_table.add_row("Unique Authors", str(stats["authors"]))
            stat_table.add_row("Genres", str(stats["genres"]))
            stat_table.add_row("Oldest Book", str(stats["min_year"]) if stats["min_year"] else "—")
            stat_table.add_row("Newest Book", str(stats["max_year"]) if stats["max_year"] else "—")
            self.console.print(stat_table)
            if genres:
                genre_table = Table(title="📚 Books by Genre", box=box.MINIMAL)
                genre_table.add_column("Genre", style="yellow")
                genre_table.add_column("Count", justify="right")
                for g, count in sorted(genres.items()):
                    genre_table.add_row(g, str(count))
                self.console.print(genre_table)
        else:
            print("\n📊 STATISTICS")
            print(c("─"*30, "dim"))
            print(f"  Total Books: {stats['total']}")
            print(f"  Unique Authors: {stats['authors']}")
            print(f"  Genres: {stats['genres']}")
            print(f"  Oldest Book: {stats['min_year'] if stats['min_year'] else '—'}")
            print(f"  Newest Book: {stats['max_year'] if stats['max_year'] else '—'}")
            if genres:
                print("\n📚 Books by Genre:")
                for g, count in sorted(genres.items()):
                    print(f"  {g}: {count}")

    def browse_by_genre(self):
        genres = self.catalog.get_genres()
        if not genres:
            print(c("No books yet.", "yellow"))
            return
        if self.console:
            genre_names = list(genres.keys())
            self.console.print("[bold]Select a genre:[/bold]")
            for i, g in enumerate(genre_names, 1):
                self.console.print(f"  [{i}] {g} ({genres[g]})")
            choice = Prompt.ask("Number", choices=[str(i) for i in range(1, len(genre_names)+1)])
        else:
            print("Select a genre:")
            genre_list = list(genres.keys())
            for i, g in enumerate(genre_list, 1):
                print(f"  {i}. {g} ({genres[g]})")
            choice = input("Number: ").strip()
        try:
            idx = int(choice) - 1
            if 0 <= idx < len(genre_list):
                selected = genre_list[idx]
                books = self.catalog.search(genre=selected)
                self.list_books(books)
            else:
                print(c("Invalid selection.", "red"))
        except ValueError:
            print(c("Invalid input.", "red"))

    def edit_book(self):
        if self.console:
            book_id = Prompt.ask("Enter book ID to edit")
        else:
            book_id = input("Enter book ID to edit: ").strip()
        book = self.catalog.get_book(book_id)
        if not book:
            print(c("Book not found.", "red"))
            return
        if self.console:
            self.console.print(f"[bold]Editing: {book.title} by {book.author}[/bold]")
            title = Prompt.ask("Title", default=book.title)
            author = Prompt.ask("Author", default=book.author)
            genre = Prompt.ask("Genre", default=book.genre)
            year = IntPrompt.ask("Year", default=book.year)
            isbn = Prompt.ask("ISBN", default=book.isbn)
        else:
            print(f"Editing: {book.title} by {book.author}")
            title = input(f"Title ({book.title}): ").strip() or book.title
            author = input(f"Author ({book.author}): ").strip() or book.author
            genre = input(f"Genre ({book.genre}): ").strip() or book.genre
            try:
                year = int(input(f"Year ({book.year}): ").strip() or book.year)
            except ValueError:
                year = book.year
            isbn = input(f"ISBN ({book.isbn}): ").strip() or book.isbn
        self.catalog.update_book(book_id, title=title, author=author, genre=genre, year=year, isbn=isbn)
        print(c("✅ Book updated.", "green"))

    def delete_book(self):
        if self.console:
            book_id = Prompt.ask("Enter book ID to delete")
            if not Confirm.ask(f"Delete book {book_id}?"):
                return
        else:
            book_id = input("Enter book ID to delete: ").strip()
            confirm = input(f"Delete book {book_id}? (yes/no): ").strip().lower()
            if confirm != "yes":
                return
        if self.catalog.delete_book(book_id):
            print(c("🗑️  Book deleted.", "yellow"))
        else:
            print(c("Book not found.", "red"))

    def run(self):
        if self.console:
            self.console.print(Panel.fit("[bold cyan]📚 Book Catalog – Organize Your Library[/bold cyan]", border_style="cyan"))
        else:
            print(c("\n📚 Book Catalog – Organize Your Library", "bright"))
            print(c("Manage your books by genre!", "dim"))

        while True:
            self.show_menu()
            if self.console:
                choice = Prompt.ask("Your choice", choices=["0","1","2","3","4","5","6","7"])
            else:
                choice = input("Your choice: ").strip()

            if choice == "1":
                self.list_books()
            elif choice == "2":
                self.add_book()
            elif choice == "3":
                self.search_books()
            elif choice == "4":
                self.show_stats()
            elif choice == "5":
                self.browse_by_genre()
            elif choice == "6":
                self.edit_book()
            elif choice == "7":
                self.delete_book()
            elif choice == "0":
                print(c("👋 Goodbye!", "cyan"))
                break
            else:
                print(c("❌ Invalid choice.", "red"))

            if choice != "0":
                if self.console:
                    self.console.print("\n[dim]Press Enter to continue...[/dim]")
                    input()
                else:
                    input("\nPress Enter to continue...")


def main():
    try:
        app = BookApp()
        app.run()
    except KeyboardInterrupt:
        print("\n👋 Goodbye!")
        sys.exit(0)
    except Exception as e:
        print(c(f"❌ Unexpected error: {e}", "red"))
        sys.exit(1)

if __name__ == "__main__":
    main()
