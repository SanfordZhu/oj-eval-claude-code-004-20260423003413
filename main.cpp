#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <cstdio>

using namespace std;

const int MAX_USERID_LEN = 30;
const int MAX_PASSWORD_LEN = 30;
const int MAX_USERNAME_LEN = 30;
const int MAX_ISBN_LEN = 20;
const int MAX_BOOKNAME_LEN = 60;
const int MAX_AUTHOR_LEN = 60;
const int MAX_KEYWORD_LEN = 60;
const int MAX_PRICE_LEN = 13;

struct Account {
    char userid[MAX_USERID_LEN + 1];
    char password[MAX_PASSWORD_LEN + 1];
    char username[MAX_USERNAME_LEN + 1];
    int privilege;
    int selected_book_pos;
};

struct Book {
    char isbn[MAX_ISBN_LEN + 1];
    char bookname[MAX_BOOKNAME_LEN + 1];
    char author[MAX_AUTHOR_LEN + 1];
    char keyword[MAX_KEYWORD_LEN + 1];
    double price;
    int stock;
};

struct FinanceRecord {
    double income;
    double expense;
};

const string ACCOUNT_FILE = "accounts.dat";
const string BOOK_FILE = "books.dat";
const string FINANCE_FILE = "finance.dat";
const string LOG_FILE = "log.txt";

class BookstoreSystem {
private:
    map<string, int> account_map;
    map<string, int> book_map;
    stack<string> login_stack;
    vector<Account> accounts;
    vector<Book> books;
    vector<FinanceRecord> finance_log;
    int current_finance_pos;

    bool isValidUserID(const string& s) {
        if (s.empty() || s.length() > MAX_USERID_LEN) return false;
        for (char c : s) {
            if (!isalnum(c) && c != '_') return false;
        }
        return true;
    }

    bool isValidPassword(const string& s) {
        if (s.empty() || s.length() > MAX_PASSWORD_LEN) return false;
        for (char c : s) {
            if (!isalnum(c) && c != '_') return false;
        }
        return true;
    }

    bool isValidUsername(const string& s) {
        if (s.empty() || s.length() > MAX_USERNAME_LEN) return false;
        for (char c : s) {
            if (c < 32 || c > 126) return false;
        }
        return true;
    }

    bool isValidPrivilege(const string& s) {
        if (s.length() != 1) return false;
        return s == "1" || s == "3" || s == "7";
    }

    bool isValidISBN(const string& s) {
        if (s.empty() || s.length() > MAX_ISBN_LEN) return false;
        for (char c : s) {
            if (c < 32 || c > 126) return false;
        }
        return true;
    }

    bool isValidBookName(const string& s) {
        if (s.empty() || s.length() > MAX_BOOKNAME_LEN) return false;
        for (char c : s) {
            if (c < 32 || c > 126 || c == '"') return false;
        }
        return true;
    }

    bool isValidAuthor(const string& s) {
        if (s.empty() || s.length() > MAX_AUTHOR_LEN) return false;
        for (char c : s) {
            if (c < 32 || c > 126 || c == '"') return false;
        }
        return true;
    }

    bool isValidKeyword(const string& s) {
        if (s.empty() || s.length() > MAX_KEYWORD_LEN) return false;
        for (char c : s) {
            if (c < 32 || c > 126 || c == '"') return false;
        }
        return true;
    }

    bool isValidPrice(const string& s) {
        if (s.empty() || s.length() > MAX_PRICE_LEN) return false;
        bool has_dot = false;
        for (char c : s) {
            if (!isdigit(c)) {
                if (c == '.') {
                    if (has_dot) return false;
                    has_dot = true;
                } else {
                    return false;
                }
            }
        }
        return true;
    }

    bool isValidQuantity(const string& s) {
        if (s.empty() || s.length() > 10) return false;
        for (char c : s) {
            if (!isdigit(c)) return false;
        }
        return true;
    }

    bool hasDuplicateKeywords(const string& keyword) {
        vector<string> keywords;
        stringstream ss(keyword);
        string token;
        while (getline(ss, token, '|')) {
            for (const string& k : keywords) {
                if (k == token) return true;
            }
            keywords.push_back(token);
        }
        return false;
    }

    bool hasMultipleKeywords(const string& keyword) {
        return keyword.find('|') != string::npos;
    }

    int getCurrentPrivilege() {
        if (login_stack.empty()) return 0;
        string userid = login_stack.top();
        auto it = account_map.find(userid);
        if (it != account_map.end()) {
            return accounts[it->second].privilege;
        }
        return 0;
    }

    string getCurrentUser() {
        if (login_stack.empty()) return "";
        return login_stack.top();
    }

    int getSelectedBookPos() {
        string userid = getCurrentUser();
        if (userid.empty()) return -1;
        auto it = account_map.find(userid);
        if (it != account_map.end()) {
            return accounts[it->second].selected_book_pos;
        }
        return -1;
    }

    void setSelectedBookPos(int pos) {
        string userid = getCurrentUser();
        if (userid.empty()) return;
        auto it = account_map.find(userid);
        if (it != account_map.end()) {
            accounts[it->second].selected_book_pos = pos;
        }
    }

    void saveAccounts() {
        ofstream out(ACCOUNT_FILE, ios::binary);
        if (!out.is_open()) return;
        for (const auto& acc : accounts) {
            out.write(reinterpret_cast<const char*>(&acc), sizeof(Account));
        }
        out.close();
    }

    void loadAccounts() {
        ifstream in(ACCOUNT_FILE, ios::binary);
        if (!in.is_open()) {
            Account root;
            strcpy(root.userid, "root");
            strcpy(root.password, "sjtu");
            strcpy(root.username, "root");
            root.privilege = 7;
            root.selected_book_pos = -1;
            accounts.push_back(root);
            account_map["root"] = 0;
            saveAccounts();
            return;
        }
        accounts.clear();
        account_map.clear();
        Account acc;
        while (in.read(reinterpret_cast<char*>(&acc), sizeof(Account))) {
            accounts.push_back(acc);
            account_map[acc.userid] = accounts.size() - 1;
        }
        in.close();
    }

    void saveBooks() {
        ofstream out(BOOK_FILE, ios::binary);
        if (!out.is_open()) return;
        for (const auto& book : books) {
            out.write(reinterpret_cast<const char*>(&book), sizeof(Book));
        }
        out.close();
    }

    void loadBooks() {
        ifstream in(BOOK_FILE, ios::binary);
        if (!in.is_open()) return;
        books.clear();
        book_map.clear();
        Book book;
        while (in.read(reinterpret_cast<char*>(&book), sizeof(Book))) {
            books.push_back(book);
            book_map[book.isbn] = books.size() - 1;
        }
        in.close();
    }

    void saveFinance() {
        ofstream out(FINANCE_FILE, ios::binary);
        if (!out.is_open()) return;
        out.write(reinterpret_cast<const char*>(&current_finance_pos), sizeof(int));
        for (const auto& rec : finance_log) {
            out.write(reinterpret_cast<const char*>(&rec), sizeof(FinanceRecord));
        }
        out.close();
    }

    void loadFinance() {
        ifstream in(FINANCE_FILE, ios::binary);
        if (!in.is_open()) {
            current_finance_pos = 0;
            return;
        }
        in.read(reinterpret_cast<char*>(&current_finance_pos), sizeof(int));
        finance_log.clear();
        FinanceRecord rec;
        while (in.read(reinterpret_cast<char*>(&rec), sizeof(FinanceRecord))) {
            finance_log.push_back(rec);
        }
        in.close();
    }

    void addFinanceRecord(double income, double expense) {
        FinanceRecord rec;
        rec.income = income;
        rec.expense = expense;
        finance_log.push_back(rec);
        current_finance_pos++;
        saveFinance();
    }

    vector<string> splitCommand(const string& line) {
        vector<string> parts;
        string current;
        bool in_quote = false;
        for (char c : line) {
            if (c == '"') {
                in_quote = !in_quote;
            } else if (c == ' ' && !in_quote) {
                if (!current.empty()) {
                    parts.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty()) {
            parts.push_back(current);
        }
        return parts;
    }

    string formatPrice(double price) {
        stringstream ss;
        ss << fixed << setprecision(2) << price;
        return ss.str();
    }

public:
    BookstoreSystem() : current_finance_pos(0) {
        loadAccounts();
        loadBooks();
        loadFinance();
    }

    ~BookstoreSystem() {
        saveAccounts();
        saveBooks();
        saveFinance();
    }

    void run() {
        string line;
        while (getline(cin, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
                line.pop_back();
            }
            if (line.empty()) continue;
            executeCommand(line);
        }
    }

    void executeCommand(const string& line) {
        vector<string> parts = splitCommand(line);
        if (parts.empty()) return;

        string cmd = parts[0];

        if (cmd == "quit" || cmd == "exit") {
            exit(0);
        } else if (cmd == "su") {
            handleSu(parts);
        } else if (cmd == "logout") {
            handleLogout(parts);
        } else if (cmd == "register") {
            handleRegister(parts);
        } else if (cmd == "passwd") {
            handlePasswd(parts);
        } else if (cmd == "useradd") {
            handleUseradd(parts);
        } else if (cmd == "delete") {
            handleDelete(parts);
        } else if (cmd == "show") {
            handleShow(parts);
        } else if (cmd == "buy") {
            handleBuy(parts);
        } else if (cmd == "select") {
            handleSelect(parts);
        } else if (cmd == "modify") {
            handleModify(parts);
        } else if (cmd == "import") {
            handleImport(parts);
        } else if (cmd == "log") {
            handleLog(parts);
        } else if (cmd == "report") {
            handleReport(parts);
        } else {
            cout << "Invalid" << endl;
        }
    }

    void handleSu(const vector<string>& parts) {
        if (parts.size() < 2 || parts.size() > 3) {
            cout << "Invalid" << endl;
            return;
        }
        string userid = parts[1];
        if (!isValidUserID(userid)) {
            cout << "Invalid" << endl;
            return;
        }
        auto it = account_map.find(userid);
        if (it == account_map.end()) {
            cout << "Invalid" << endl;
            return;
        }
        Account& target = accounts[it->second];
        int current_priv = getCurrentPrivilege();
        if (parts.size() == 2) {
            if (current_priv > target.privilege) {
                login_stack.push(userid);
            } else {
                cout << "Invalid" << endl;
            }
        } else {
            string password = parts[2];
            if (password == target.password) {
                login_stack.push(userid);
            } else {
                cout << "Invalid" << endl;
            }
        }
    }

    void handleLogout(const vector<string>& parts) {
        if (parts.size() != 1) {
            cout << "Invalid" << endl;
            return;
        }
        if (login_stack.empty()) {
            cout << "Invalid" << endl;
            return;
        }
        login_stack.pop();
    }

    void handleRegister(const vector<string>& parts) {
        if (parts.size() != 4) {
            cout << "Invalid" << endl;
            return;
        }
        string userid = parts[1];
        string password = parts[2];
        string username = parts[3];
        if (!isValidUserID(userid) || !isValidPassword(password) || !isValidUsername(username)) {
            cout << "Invalid" << endl;
            return;
        }
        if (account_map.find(userid) != account_map.end()) {
            cout << "Invalid" << endl;
            return;
        }
        Account acc;
        strcpy(acc.userid, userid.c_str());
        strcpy(acc.password, password.c_str());
        strcpy(acc.username, username.c_str());
        acc.privilege = 1;
        acc.selected_book_pos = -1;
        accounts.push_back(acc);
        account_map[userid] = accounts.size() - 1;
        saveAccounts();
    }

    void handlePasswd(const vector<string>& parts) {
        if (parts.size() < 3 || parts.size() > 4) {
            cout << "Invalid" << endl;
            return;
        }
        string userid = parts[1];
        if (!isValidUserID(userid)) {
            cout << "Invalid" << endl;
            return;
        }
        auto it = account_map.find(userid);
        if (it == account_map.end()) {
            cout << "Invalid" << endl;
            return;
        }
        Account& acc = accounts[it->second];
        int current_priv = getCurrentPrivilege();
        if (parts.size() == 3) {
            if (current_priv == 7) {
                string newpass = parts[2];
                if (!isValidPassword(newpass)) {
                    cout << "Invalid" << endl;
                    return;
                }
                strcpy(acc.password, newpass.c_str());
                saveAccounts();
            } else {
                cout << "Invalid" << endl;
            }
        } else {
            string currentpass = parts[2];
            string newpass = parts[3];
            if (!isValidPassword(newpass)) {
                cout << "Invalid" << endl;
                return;
            }
            if (current_priv == 7 || currentpass == acc.password) {
                strcpy(acc.password, newpass.c_str());
                saveAccounts();
            } else {
                cout << "Invalid" << endl;
            }
        }
    }

    void handleUseradd(const vector<string>& parts) {
        if (parts.size() != 5) {
            cout << "Invalid" << endl;
            return;
        }
        int current_priv = getCurrentPrivilege();
        if (current_priv < 3) {
            cout << "Invalid" << endl;
            return;
        }
        string userid = parts[1];
        string password = parts[2];
        string privilege_str = parts[3];
        string username = parts[4];
        if (!isValidUserID(userid) || !isValidPassword(password) ||
            !isValidPrivilege(privilege_str) || !isValidUsername(username)) {
            cout << "Invalid" << endl;
            return;
        }
        int privilege = stoi(privilege_str);
        if (privilege >= current_priv) {
            cout << "Invalid" << endl;
            return;
        }
        if (account_map.find(userid) != account_map.end()) {
            cout << "Invalid" << endl;
            return;
        }
        Account acc;
        strcpy(acc.userid, userid.c_str());
        strcpy(acc.password, password.c_str());
        strcpy(acc.username, username.c_str());
        acc.privilege = privilege;
        acc.selected_book_pos = -1;
        accounts.push_back(acc);
        account_map[userid] = accounts.size() - 1;
        saveAccounts();
    }

    void handleDelete(const vector<string>& parts) {
        if (parts.size() != 2) {
            cout << "Invalid" << endl;
            return;
        }
        int current_priv = getCurrentPrivilege();
        if (current_priv < 7) {
            cout << "Invalid" << endl;
            return;
        }
        string userid = parts[1];
        if (!isValidUserID(userid)) {
            cout << "Invalid" << endl;
            return;
        }
        auto it = account_map.find(userid);
        if (it == account_map.end()) {
            cout << "Invalid" << endl;
            return;
        }
        int pos = it->second;
        for (const auto& acc : accounts) {
            if (acc.selected_book_pos == pos) {
                cout << "Invalid" << endl;
                return;
            }
        }
        stack<string> temp_stack = login_stack;
        while (!temp_stack.empty()) {
            if (temp_stack.top() == userid) {
                cout << "Invalid" << endl;
                return;
            }
            temp_stack.pop();
        }
        account_map.erase(it);
        accounts.erase(accounts.begin() + pos);
        for (auto& pair : account_map) {
            if (pair.second > pos) pair.second--;
        }
        for (auto& acc : accounts) {
            if (acc.selected_book_pos > pos) acc.selected_book_pos--;
        }
        saveAccounts();
    }

    void handleShow(const vector<string>& parts) {
        int current_priv = getCurrentPrivilege();
        if (current_priv < 1) {
            cout << "Invalid" << endl;
            return;
        }
        if (parts.size() == 1) {
            bool found = false;
            for (const auto& book : books) {
                found = true;
                cout << book.isbn << "\t" << book.bookname << "\t"
                     << book.author << "\t" << book.keyword << "\t"
                     << formatPrice(book.price) << "\t" << book.stock << endl;
            }
            if (!found) cout << endl;
            return;
        }
        string param = parts[1];
        if (param == "finance") {
            handleShowFinance(parts);
            return;
        }
        string value;
        size_t eq_pos = param.find('=');
        if (eq_pos == string::npos) {
            cout << "Invalid" << endl;
            return;
        }
        string key = param.substr(0, eq_pos);
        value = param.substr(eq_pos + 1);
        if (value.empty()) {
            cout << "Invalid" << endl;
            return;
        }
        bool found = false;
        if (key == "-ISBN") {
            if (!isValidISBN(value)) {
                cout << "Invalid" << endl;
                return;
            }
            auto it = book_map.find(value);
            if (it != book_map.end()) {
                found = true;
                const Book& book = books[it->second];
                cout << book.isbn << "\t" << book.bookname << "\t"
                     << book.author << "\t" << book.keyword << "\t"
                     << formatPrice(book.price) << "\t" << book.stock << endl;
            }
        } else if (key == "-name") {
            if (!isValidBookName(value)) {
                cout << "Invalid" << endl;
                return;
            }
            for (const auto& book : books) {
                if (book.bookname == value) {
                    found = true;
                    cout << book.isbn << "\t" << book.bookname << "\t"
                         << book.author << "\t" << book.keyword << "\t"
                         << formatPrice(book.price) << "\t" << book.stock << endl;
                }
            }
        } else if (key == "-author") {
            if (!isValidAuthor(value)) {
                cout << "Invalid" << endl;
                return;
            }
            for (const auto& book : books) {
                if (book.author == value) {
                    found = true;
                    cout << book.isbn << "\t" << book.bookname << "\t"
                         << book.author << "\t" << book.keyword << "\t"
                         << formatPrice(book.price) << "\t" << book.stock << endl;
                }
            }
        } else if (key == "-keyword") {
            if (!isValidKeyword(value)) {
                cout << "Invalid" << endl;
                return;
            }
            if (hasMultipleKeywords(value)) {
                cout << "Invalid" << endl;
                return;
            }
            for (const auto& book : books) {
                string kw = book.keyword;
                vector<string> keywords;
                stringstream ss(kw);
                string token;
                while (getline(ss, token, '|')) {
                    keywords.push_back(token);
                }
                for (const string& k : keywords) {
                    if (k == value) {
                        found = true;
                        cout << book.isbn << "\t" << book.bookname << "\t"
                             << book.author << "\t" << book.keyword << "\t"
                             << formatPrice(book.price) << "\t" << book.stock << endl;
                        break;
                    }
                }
            }
        } else {
            cout << "Invalid" << endl;
            return;
        }
        if (!found) cout << endl;
    }

    void handleShowFinance(const vector<string>& parts) {
        int current_priv = getCurrentPrivilege();
        if (current_priv < 7) {
            cout << "Invalid" << endl;
            return;
        }
        int count = finance_log.size();
        bool has_count_param = (parts.size() == 3);
        if (has_count_param) {
            if (!isValidQuantity(parts[2])) {
                cout << "Invalid" << endl;
                return;
            }
            count = stoi(parts[2]);
            if (count > (int)finance_log.size()) {
                cout << "Invalid" << endl;
                return;
            }
        }
        if (has_count_param && count == 0) {
            cout << endl;
            return;
        }
        double income = 0, expense = 0;
        int start = finance_log.size() - count;
        for (int i = start; i < (int)finance_log.size(); i++) {
            income += finance_log[i].income;
            expense += finance_log[i].expense;
        }
        cout << "+ " << formatPrice(income) << " - " << formatPrice(expense) << endl;
    }

    void handleBuy(const vector<string>& parts) {
        int current_priv = getCurrentPrivilege();
        if (current_priv < 1) {
            cout << "Invalid" << endl;
            return;
        }
        if (parts.size() != 3) {
            cout << "Invalid" << endl;
            return;
        }
        string isbn = parts[1];
        string quantity_str = parts[2];
        if (!isValidISBN(isbn) || !isValidQuantity(quantity_str)) {
            cout << "Invalid" << endl;
            return;
        }
        int quantity = stoi(quantity_str);
        if (quantity <= 0) {
            cout << "Invalid" << endl;
            return;
        }
        auto it = book_map.find(isbn);
        if (it == book_map.end()) {
            cout << "Invalid" << endl;
            return;
        }
        Book& book = books[it->second];
        if (book.stock < quantity) {
            cout << "Invalid" << endl;
            return;
        }
        book.stock -= quantity;
        double total = book.price * quantity;
        addFinanceRecord(total, 0);
        saveBooks();
        cout << formatPrice(total) << endl;
    }

    void handleSelect(const vector<string>& parts) {
        int current_priv = getCurrentPrivilege();
        if (current_priv < 3) {
            cout << "Invalid" << endl;
            return;
        }
        if (parts.size() != 2) {
            cout << "Invalid" << endl;
            return;
        }
        string isbn = parts[1];
        if (!isValidISBN(isbn)) {
            cout << "Invalid" << endl;
            return;
        }
        auto it = book_map.find(isbn);
        if (it == book_map.end()) {
            Book book;
            strcpy(book.isbn, isbn.c_str());
            strcpy(book.bookname, "");
            strcpy(book.author, "");
            strcpy(book.keyword, "");
            book.price = 0;
            book.stock = 0;
            books.push_back(book);
            book_map[isbn] = books.size() - 1;
            setSelectedBookPos(books.size() - 1);
            saveBooks();
        } else {
            setSelectedBookPos(it->second);
        }
    }

    void handleModify(const vector<string>& parts) {
        int current_priv = getCurrentPrivilege();
        if (current_priv < 3) {
            cout << "Invalid" << endl;
            return;
        }
        int book_pos = getSelectedBookPos();
        if (book_pos == -1) {
            cout << "Invalid" << endl;
            return;
        }
        Book& book = books[book_pos];
        string old_isbn = book.isbn;
        string new_isbn = old_isbn;
        bool has_isbn = false, has_name = false, has_author = false, has_keyword = false, has_price = false;
        for (size_t i = 1; i < parts.size(); i++) {
            string param = parts[i];
            size_t eq_pos = param.find('=');
            if (eq_pos == string::npos) {
                cout << "Invalid" << endl;
                return;
            }
            string key = param.substr(0, eq_pos);
            string value = param.substr(eq_pos + 1);
            if (value.empty()) {
                cout << "Invalid" << endl;
                return;
            }
            if (key == "-ISBN") {
                if (has_isbn) {
                    cout << "Invalid" << endl;
                    return;
                }
                has_isbn = true;
                if (!isValidISBN(value)) {
                    cout << "Invalid" << endl;
                    return;
                }
                if (value == old_isbn) {
                    cout << "Invalid" << endl;
                    return;
                }
                if (book_map.find(value) != book_map.end()) {
                    cout << "Invalid" << endl;
                    return;
                }
                new_isbn = value;
            } else if (key == "-name") {
                if (has_name) {
                    cout << "Invalid" << endl;
                    return;
                }
                has_name = true;
                if (!isValidBookName(value)) {
                    cout << "Invalid" << endl;
                    return;
                }
                strcpy(book.bookname, value.c_str());
            } else if (key == "-author") {
                if (has_author) {
                    cout << "Invalid" << endl;
                    return;
                }
                has_author = true;
                if (!isValidAuthor(value)) {
                    cout << "Invalid" << endl;
                    return;
                }
                strcpy(book.author, value.c_str());
            } else if (key == "-keyword") {
                if (has_keyword) {
                    cout << "Invalid" << endl;
                    return;
                }
                has_keyword = true;
                if (!isValidKeyword(value)) {
                    cout << "Invalid" << endl;
                    return;
                }
                if (hasDuplicateKeywords(value)) {
                    cout << "Invalid" << endl;
                    return;
                }
                strcpy(book.keyword, value.c_str());
            } else if (key == "-price") {
                if (has_price) {
                    cout << "Invalid" << endl;
                    return;
                }
                has_price = true;
                if (!isValidPrice(value)) {
                    cout << "Invalid" << endl;
                    return;
                }
                book.price = stod(value);
            } else {
                cout << "Invalid" << endl;
                return;
            }
        }
        if (has_isbn) {
            book_map.erase(old_isbn);
            strcpy(book.isbn, new_isbn.c_str());
            book_map[new_isbn] = book_pos;
        }
        saveBooks();
    }

    void handleImport(const vector<string>& parts) {
        int current_priv = getCurrentPrivilege();
        if (current_priv < 3) {
            cout << "Invalid" << endl;
            return;
        }
        int book_pos = getSelectedBookPos();
        if (book_pos == -1) {
            cout << "Invalid" << endl;
            return;
        }
        if (parts.size() != 3) {
            cout << "Invalid" << endl;
            return;
        }
        string quantity_str = parts[1];
        string cost_str = parts[2];
        if (!isValidQuantity(quantity_str)) {
            cout << "Invalid" << endl;
            return;
        }
        if (!isValidPrice(cost_str)) {
            cout << "Invalid" << endl;
            return;
        }
        int quantity = stoi(quantity_str);
        double cost = stod(cost_str);
        if (quantity <= 0 || cost <= 0) {
            cout << "Invalid" << endl;
            return;
        }
        books[book_pos].stock += quantity;
        addFinanceRecord(0, cost);
        saveBooks();
    }

    void handleLog(const vector<string>& parts) {
        int current_priv = getCurrentPrivilege();
        if (current_priv < 7) {
            cout << "Invalid" << endl;
            return;
        }
        cout << "Log System" << endl;
    }

    void handleReport(const vector<string>& parts) {
        int current_priv = getCurrentPrivilege();
        if (current_priv < 7) {
            cout << "Invalid" << endl;
            return;
        }
        if (parts.size() != 2) {
            cout << "Invalid" << endl;
            return;
        }
        if (parts[1] == "finance") {
            cout << "Financial Report" << endl;
        } else if (parts[1] == "employee") {
            cout << "Employee Report" << endl;
        } else {
            cout << "Invalid" << endl;
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    BookstoreSystem system;
    system.run();
    return 0;
}
