📚 Book Catalog – Organize Your Library by Genre
"Your personal library manager – catalog, search, and explore books by genre, author, and more!"

📋 Table of Contents
✨ Features

📁 Repository Structure

🚀 Quick Start

💻 Language Implementations

📊 Data Format

🤝 Contributing

📄 License

✨ Features
Feature	Description
📚 Add Books	Add books with title, author, genre, year, and ISBN
🔍 Search & Filter	Search by title, author, genre, or year range
🗂️ Genre Groups	View all books organized by genre with counts
📊 Statistics	See total books, books per genre, author counts
💾 Persistence	All data saved locally in JSON format
✏️ Edit & Delete	Modify book details or remove books from the catalog
🎨 Colorful CLI	Beautiful terminal interface with ANSI colors
⚡ Cross‑Platform	Works on Windows, macOS, and Linux
📁 Repository Structure
text
book-catalog/
├── README.md
├── python/
│   └── book_catalog.py
├── javascript/
│   └── book_catalog.js
├── typescript/
│   └── book_catalog.ts
├── go/
│   └── book_catalog.go
├── rust/
│   └── book_catalog.rs
├── cpp/
│   └── book_catalog.cpp
├── java/
│   └── BookCatalog.java
└── csharp/
    └── BookCatalog.cs
🚀 Quick Start
Prerequisites
Each language requires its respective runtime/compiler (see individual sections)

Clone & Run
bash
git clone https://github.com/yourusername/book-catalog.git
cd book-catalog
# Navigate to your language folder and run
💻 Language Implementations
1. 🐍 Python
bash
cd python
pip install rich
python book_catalog.py
Requires: Python 3.8+

2. 🟨 JavaScript (Node.js)
bash
cd javascript
node book_catalog.js
Requires: Node.js 16+

3. 🟦 TypeScript
bash
cd typescript
npm install -g ts-node
ts-node book_catalog.ts
Requires: Node.js 16+, TypeScript

4. 🟩 Go
bash
cd go
go run book_catalog.go
Requires: Go 1.18+

5. 🦀 Rust
bash
cd rust
cargo run
Requires: Rust 1.70+ (dependencies: serde, serde_json, chrono, colored)

6. ⚙️ C++
bash
cd cpp
g++ -std=c++17 book_catalog.cpp -o book_catalog
./book_catalog
Requires: C++17 compiler

7. ☕ Java
bash
cd java
javac BookCatalog.java
java BookCatalog
Requires: JDK 17+

8. 🔷 C#
bash
cd csharp
dotnet run
Requires: .NET 6.0+

📊 Data Format
All implementations use a unified JSON schema:

json
{
  "books": [
    {
      "id": "uuid",
      "title": "The Hobbit",
      "author": "J.R.R. Tolkien",
      "genre": "Fantasy",
      "year": 1937,
      "isbn": "978-0-618-00221-2"
    }
  ]
}
Data is stored in the user's home directory under .book_catalog/books.json.

🤝 Contributing
Contributions are welcome! Please:

Fork the repository

Create a feature branch

Commit your changes

Open a Pull Request

📄 License
MIT © 2026 Book Catalog Team
