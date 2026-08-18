# book_catalog.js
/**
 * 📚 Book Catalog – Organize Your Library by Genre (Node.js Edition)
 * Features: add, edit, delete, search, genre stats, persistence
 */

const fs = require('fs');
const path = require('path');
const os = require('os');
const readline = require('readline');
const { v4: uuidv4 } = require('uuid');

// ─── Colors ──────────────────────────────────────────────────────────────────

const colors = {
    reset: '\x1b[0m',
    bright: '\x1b[1m',
    dim: '\x1b[2m',
    red: '\x1b[31m',
    green: '\x1b[32m',
    yellow: '\x1b[33m',
    blue: '\x1b[34m',
    magenta: '\x1b[35m',
    cyan: '\x1b[36m',
};

const c = (str, color) => `${color}${str}${colors.reset}`;

// ─── Config ──────────────────────────────────────────────────────────────────

const CONFIG = {
    dataDir: path.join(os.homedir(), '.book_catalog'),
    dataFile: 'books.json'
};

// ─── Book Class ─────────────────────────────────────────────────────────────

class Book {
    constructor(title, author, genre, year, isbn = '', id = null) {
        this.id = id || uuidv4();
        this.title = title;
        this.author = author;
        this.genre = genre;
        this.year = year;
        this.isbn = isbn;
    }
    toJSON() {
        return {
            id: this.id,
            title: this.title,
            author: this.author,
            genre: this.genre,
            year: this.year,
            isbn: this.isbn
        };
    }
    static fromJSON(data) {
        return new Book(data.title, data.author, data.genre, data.year, data.isbn, data.id);
    }
}

// ─── Catalog Manager ──────────────────────────────────────────────────────

class BookCatalog {
    constructor() {
        this.books = [];
        this._load();
    }

    _getDataPath() {
        if (!fs.existsSync(CONFIG.dataDir)) fs.mkdirSync(CONFIG.dataDir, { recursive: true });
        return path.join(CONFIG.dataDir, CONFIG.dataFile);
    }

    _load() {
        const file = this._getDataPath();
        if (fs.existsSync(file)) {
            try {
                const raw = fs.readFileSync(file, 'utf8');
                const data = JSON.parse(raw);
                this.books = data.books.map(b => Book.fromJSON(b));
            } catch (_) {
                this.books = [];
            }
        } else {
            this.books = [];
        }
    }

    _save() {
        const data = { books: this.books.map(b => b.toJSON()) };
        fs.writeFileSync(this._getDataPath(), JSON.stringify(data, null, 2));
    }

    addBook(title, author, genre, year, isbn = '') {
        const book = new Book(title, author, genre, year, isbn);
        this.books.push(book);
        this._save();
        return book;
    }

    deleteBook(id) {
        const idx = this.books.findIndex(b => b.id === id);
        if (idx === -1) return false;
        this.books.splice(idx, 1);
        this._save();
        return true;
    }

    updateBook(id, updates) {
        const book = this.books.find(b => b.id === id);
        if (!book) return false;
        Object.assign(book, updates);
        this._save();
        return true;
    }

    getBook(id) {
        return this.books.find(b => b.id === id) || null;
    }

    search({ query = '', genre = '', author = '', yearMin = null, yearMax = null } = {}) {
        let results = this.books;
        if (query) {
            const q = query.toLowerCase();
            results = results.filter(b =>
                b.title.toLowerCase().includes(q) ||
                b.author.toLowerCase().includes(q) ||
                b.genre.toLowerCase().includes(q)
            );
        }
        if (genre) results = results.filter(b => b.genre.toLowerCase() === genre.toLowerCase());
        if (author) results = results.filter(b => b.author.toLowerCase().includes(author.toLowerCase()));
        if (yearMin !== null) results = results.filter(b => b.year >= yearMin);
        if (yearMax !== null) results = results.filter(b => b.year <= yearMax);
        return results;
    }

    getGenres() {
        const map = {};
        for (const b of this.books) {
            map[b.genre] = (map[b.genre] || 0) + 1;
        }
        return map;
    }

    getStats() {
        const total = this.books.length;
        const authors = new Set(this.books.map(b => b.author));
        const years = this.books.map(b => b.year);
        return {
            total,
            authors: authors.size,
            genres: Object.keys(this.getGenres()).length,
            minYear: years.length ? Math.min(...years) : null,
            maxYear: years.length ? Math.max(...years) : null
        };
    }
}

// ─── Main App ──────────────────────────────────────────────────────────────

class BookApp {
    constructor() {
        this.rl = readline.createInterface({ input: process.stdin, output: process.stdout });
        this.catalog = new BookCatalog();
    }

    _ask(prompt) {
        return new Promise(resolve => this.rl.question(prompt, resolve));
    }

    async _askInt(prompt, def = 0) {
        while (true) {
            const ans = await this._ask(prompt);
            if (ans.trim() === '') return def;
            const num = parseInt(ans.trim());
            if (!isNaN(num)) return num;
            console.log(c('❌ Please enter a number.', colors.red));
        }
    }

    async _showMenu() {
        console.log('\n' + c('═'.repeat(50), colors.cyan));
        console.log(c('📚 BOOK CATALOG', colors.bright + colors.cyan));
        console.log(c('═'.repeat(50), colors.cyan));
        console.log(`  Total books: ${this.catalog.books.length}`);
        console.log(`  Genres: ${Object.keys(this.catalog.getGenres()).length}`);
        console.log(c('═'.repeat(50), colors.cyan));
        console.log('  1. 📖 List all books');
        console.log('  2. ➕ Add a book');
        console.log('  3. 🔍 Search books');
        console.log('  4. 📊 Statistics');
        console.log('  5. 🗂️  Browse by genre');
        console.log('  6. ✏️  Edit a book');
        console.log('  7. 🗑️  Delete a book');
        console.log('  0. 🚪 Exit');
        console.log(c('═'.repeat(50), colors.cyan));
    }

    listBooks(books) {
        if (!books) books = this.catalog.books;
        if (!books.length) {
            console.log(c('No books found.', colors.yellow));
            return;
        }
        console.log('\n📖 BOOKS');
        console.log(c('─'.repeat(60), colors.dim));
        for (const b of books) {
            console.log(`  ${b.title} by ${b.author} (${b.year}) – ${b.genre} [ISBN: ${b.isbn || 'N/A'}]`);
        }
    }

    async addBook() {
        const title = await this._ask('Title: ');
        const author = await this._ask('Author: ');
        const genre = await this._ask('Genre: ');
        const year = await this._askInt('Year: ', 2000);
        const isbn = await this._ask('ISBN (optional): ');
        const book = this.catalog.addBook(title, author, genre, year, isbn);
        console.log(c(`✅ Book '${book.title}' added with ID ${book.id}`, colors.green));
    }

    async searchBooks() {
        const query = await this._ask('Search term (title/author/genre): ');
        const genre = await this._ask('Filter by genre (optional): ');
        const author = await this._ask('Filter by author (optional): ');
        const yearMin = await this._askInt('Minimum year (optional): ', 0);
        const yearMax = await this._askInt('Maximum year (optional): ', 9999);
        const results = this.catalog.search({
            query: query || undefined,
            genre: genre || undefined,
            author: author || undefined,
            yearMin: yearMin > 0 ? yearMin : undefined,
            yearMax: yearMax < 9999 ? yearMax : undefined
        });
        if (results.length) this.listBooks(results);
        else console.log(c('No books match your search.', colors.yellow));
    }

    showStats() {
        const stats = this.catalog.getStats();
        const genres = this.catalog.getGenres();
        console.log('\n📊 STATISTICS');
        console.log(c('─'.repeat(30), colors.dim));
        console.log(`  Total Books: ${stats.total}`);
        console.log(`  Unique Authors: ${stats.authors}`);
        console.log(`  Genres: ${stats.genres}`);
        console.log(`  Oldest Book: ${stats.minYear || '—'}`);
        console.log(`  Newest Book: ${stats.maxYear || '—'}`);
        if (Object.keys(genres).length) {
            console.log('\n📚 Books by Genre:');
            for (const [g, count] of Object.entries(genres).sort()) {
                console.log(`  ${g}: ${count}`);
            }
        }
    }

    async browseByGenre() {
        const genres = this.catalog.getGenres();
        const genreNames = Object.keys(genres);
        if (!genreNames.length) {
            console.log(c('No books yet.', colors.yellow));
            return;
        }
        console.log('Select a genre:');
        genreNames.forEach((g, i) => console.log(`  ${i+1}. ${g} (${genres[g]})`));
        const choice = await this._ask('Number: ');
        const idx = parseInt(choice) - 1;
        if (idx >= 0 && idx < genreNames.length) {
            const selected = genreNames[idx];
            const books = this.catalog.search({ genre: selected });
            this.listBooks(books);
        } else {
            console.log(c('Invalid selection.', colors.red));
        }
    }

    async editBook() {
        const id = await this._ask('Enter book ID to edit: ');
        const book = this.catalog.getBook(id);
        if (!book) {
            console.log(c('Book not found.', colors.red));
            return;
        }
        console.log(`Editing: ${book.title} by ${book.author}`);
        const title = await this._ask(`Title (${book.title}): `) || book.title;
        const author = await this._ask(`Author (${book.author}): `) || book.author;
        const genre = await this._ask(`Genre (${book.genre}): `) || book.genre;
        const year = parseInt(await this._ask(`Year (${book.year}): `) || book.year);
        const isbn = await this._ask(`ISBN (${book.isbn}): `) || book.isbn;
        this.catalog.updateBook(id, { title, author, genre, year, isbn });
        console.log(c('✅ Book updated.', colors.green));
    }

    async deleteBook() {
        const id = await this._ask('Enter book ID to delete: ');
        const confirm = await this._ask(`Delete book ${id}? (yes/no): `);
        if (confirm.toLowerCase() !== 'yes') return;
        if (this.catalog.deleteBook(id)) {
            console.log(c('🗑️  Book deleted.', colors.yellow));
        } else {
            console.log(c('Book not found.', colors.red));
        }
    }

    async run() {
        console.clear();
        console.log(c('\n📚 Book Catalog – Organize Your Library', colors.bright + colors.cyan));
        console.log(c('Manage your books by genre!', colors.dim));

        while (true) {
            await this._showMenu();
            const choice = await this._ask('Your choice: ');
            switch (choice.trim()) {
                case '1': this.listBooks(); break;
                case '2': await this.addBook(); break;
                case '3': await this.searchBooks(); break;
                case '4': this.showStats(); break;
                case '5': await this.browseByGenre(); break;
                case '6': await this.editBook(); break;
                case '7': await this.deleteBook(); break;
                case '0':
                    console.log(c('👋 Goodbye!', colors.cyan));
                    this.rl.close();
                    return;
                default:
                    console.log(c('❌ Invalid choice.', colors.red));
            }
            if (choice !== '0') {
                console.log('\nPress Enter to continue...');
                await this._ask('');
            }
        }
    }
}

// ─── Main ────────────────────────────────────────────────────────────────────

const main = async () => {
    try {
        const app = new BookApp();
        await app.run();
    } catch (e) {
        console.error(c(`❌ Unexpected error: ${e.message}`, colors.red));
        process.exit(1);
    }
};

main();
