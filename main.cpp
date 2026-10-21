// ---------------- Libraries ----------------
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

// ---------------- Constants ----------------
const string DATA_FILE = "items.txt";
const vector<string> CATEGORIES = {"Electronics", "Documents/ID", "Books", "Clothing",
                                   "Accessories", "Keys", "Bags", "Other"};

// ---------------- Helper functions ----------------
string toLower(string s) {
    for (size_t i = 0; i < s.size(); i++) s[i] = tolower((unsigned char)s[i]);
    return s;
}

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

string clean(string s) {
    for (size_t i = 0; i < s.size(); i++)
        if (s[i] == '|') s[i] = '/';
    return s;
}

string intToString(int n) {
    ostringstream oss;
    oss << n;
    return oss.str();
}

string today() {
    time_t t = time(nullptr);
    tm* lt = localtime(&t);
    char buf[16];
    strftime(buf, sizeof(buf), "%d-%m-%Y", lt);
    return string(buf);
}

vector<string> split(const string& line, char delim) {
    vector<string> parts;
    string part;
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] == delim) {
            parts.push_back(part);
            part.clear();
        } else {
            part += line[i];
        }
    }
    parts.push_back(part);
    return parts;
}

set<string> words(const string& text) {
    set<string> result;
    string current;
    for (size_t i = 0; i < text.size(); i++) {
        if (isalnum((unsigned char)text[i])) {
            current += (char)tolower((unsigned char)text[i]);
        } else {
            if (current.size() > 2) result.insert(current);
            current.clear();
        }
    }
    if (current.size() > 2) result.insert(current);
    return result;
}

// ---------------- Input functions ----------------
string readLine(const string& prompt) {
    string s;
    while (true) {
        cout << prompt;
        if (!getline(cin, s)) throw runtime_error("Input closed.");
        s = trim(s);
        if (!s.empty()) return clean(s);
        cout << "  This field cannot be empty.\n";
    }
}

int readInt(const string& prompt, int low, int high) {
    while (true) {
        string s = readLine(prompt);
        char* endPtr;
        long value = strtol(s.c_str(), &endPtr, 10);
        if (*endPtr == '\0' && value >= low && value <= high) return (int)value;
        cout << "  Please enter a number between " << low << " and " << high << ".\n";
    }
}

string readPhone() {
    while (true) {
        string s = readLine("Contact phone (10 digits): ");
        bool ok = (s.size() == 10);
        for (size_t i = 0; i < s.size() && ok; i++)
            if (!isdigit((unsigned char)s[i])) ok = false;
        if (ok) return s;
        cout << "  Phone number must be exactly 10 digits.\n";
    }
}

// ---------------- Item class (abstract base) ----------------
class Item {
protected:
    int id;
    string name, category, description, location, date, contactName, contactPhone;
    bool claimed;
    static int totalItems;

public:
    Item(int id, const string& name, const string& category, const string& description,
         const string& location, const string& date, const string& contactName,
         const string& contactPhone, bool claimed)
        : id(id), name(name), category(category), description(description), location(location),
          date(date), contactName(contactName), contactPhone(contactPhone), claimed(claimed) {
        totalItems++;
    }

    virtual ~Item() { totalItems--; }

    virtual string type() const = 0;
    virtual string extraLabel() const = 0;
    virtual string extraValue() const = 0;

    int getId() const { return id; }
    string getName() const { return name; }
    string getCategory() const { return category; }
    string getDescription() const { return description; }
    string getLocation() const { return location; }
    bool isClaimed() const { return claimed; }
    void setClaimed(bool value) { claimed = value; }

    static int getTotal() { return totalItems; }

    string serialize() const {
        return type() + "|" + intToString(id) + "|" + name + "|" + category + "|" + description + "|" +
               location + "|" + date + "|" + contactName + "|" + contactPhone + "|" +
               (claimed ? "1" : "0") + "|" + extraValue();
    }

    friend ostream& operator<<(ostream& os, const Item& it);
};

int Item::totalItems = 0;

// ---------------- Friend function: operator<< ----------------
ostream& operator<<(ostream& os, const Item& it) {
    os << "\n[" << it.type() << " #" << it.id << "] " << it.name
       << (it.claimed ? "  (RETURNED / CLAIMED)" : "") << "\n"
       << "  Category    : " << it.category << "\n"
       << "  Description : " << it.description << "\n"
       << "  Location    : " << it.location << "\n"
       << "  Reported on : " << it.date << "\n"
       << "  " << it.extraLabel() << " : " << it.extraValue() << "\n"
       << "  Contact     : " << it.contactName << " (" << it.contactPhone << ")\n";
    return os;
}

// ---------------- LostItem class ----------------
class LostItem : public Item {
    string identifyingMark;

public:
    LostItem(int id, const string& name, const string& category, const string& description,
             const string& location, const string& date, const string& contactName,
             const string& contactPhone, bool claimed, const string& mark)
        : Item(id, name, category, description, location, date, contactName, contactPhone, claimed),
          identifyingMark(mark) {}

    string type() const override { return "LOST"; }
    string extraLabel() const override { return "Identifying mark"; }
    string extraValue() const override { return identifyingMark; }
};

// ---------------- FoundItem class ----------------
class FoundItem : public Item {
    string keptAt;

public:
    FoundItem(int id, const string& name, const string& category, const string& description,
              const string& location, const string& date, const string& contactName,
              const string& contactPhone, bool claimed, const string& keptAt)
        : Item(id, name, category, description, location, date, contactName, contactPhone, claimed),
          keptAt(keptAt) {}

    string type() const override { return "FOUND"; }
    string extraLabel() const override { return "Currently kept at"; }
    string extraValue() const override { return keptAt; }
};

// ---------------- Main system class ----------------
class LostFoundSystem {
    vector<Item*> items;
    static int nextId;

    Item* findById(int id) {
        for (size_t i = 0; i < items.size(); i++)
            if (items[i]->getId() == id) return items[i];
        return nullptr;
    }

    Item* parseLine(const string& line) {
        vector<string> f = split(line, '|');
        if (f.size() != 11) throw runtime_error("Wrong number of fields");
        int id = atoi(f[1].c_str());
        if (id <= 0) throw runtime_error("Bad ID");
        bool claimed = (f[9] == "1");
        if (f[0] == "LOST") return new LostItem(id, f[2], f[3], f[4], f[5], f[6], f[7], f[8], claimed, f[10]);
        if (f[0] == "FOUND") return new FoundItem(id, f[2], f[3], f[4], f[5], f[6], f[7], f[8], claimed, f[10]);
        throw runtime_error("Unknown item type");
    }

public:
    // ---------------- Destructor (frees memory) ----------------
    ~LostFoundSystem() {
        for (size_t i = 0; i < items.size(); i++) delete items[i];
    }

    // ---------------- Load from file ----------------
    void load() {
        ifstream in(DATA_FILE);
        if (!in.is_open()) return;
        string line;
        int skipped = 0;
        while (getline(in, line)) {
            if (trim(line).empty()) continue;
            try {
                Item* item = parseLine(line);
                if (item->getId() >= nextId) nextId = item->getId() + 1;
                items.push_back(item);
            } catch (const exception&) {
                skipped++;
            }
        }
        if (skipped > 0) cout << "Warning: " << skipped << " corrupted line(s) were skipped.\n";
    }

    // ---------------- Save to file ----------------
    void save() const {
        ofstream out(DATA_FILE);
        if (!out.is_open()) {
            cout << "Error: could not save data to " << DATA_FILE << "\n";
            return;
        }
        for (size_t i = 0; i < items.size(); i++) out << items[i]->serialize() << "\n";
    }

    // ---------------- Report lost / found item ----------------
    void addItem(bool isLost) {
        cout << "\n--- Report " << (isLost ? "Lost" : "Found") << " Item ---\n";
        string name = readLine("Item name: ");

        cout << "Categories:\n";
        for (size_t i = 0; i < CATEGORIES.size(); i++) cout << "  " << i + 1 << ". " << CATEGORIES[i] << "\n";
        int c = readInt("Choose category: ", 1, (int)CATEGORIES.size());

        string desc = readLine("Description (colour, brand, size...): ");
        string loc = readLine(isLost ? "Where did you lose it? " : "Where did you find it? ");
        string cname = readLine("Your name: ");
        string phone = readPhone();

        Item* item;
        if (isLost) {
            string mark = readLine("Any identifying mark (or type 'none'): ");
            item = new LostItem(nextId, name, CATEGORIES[c - 1], desc, loc, today(), cname, phone, false, mark);
        } else {
            string keptAt = readLine("Where is it kept now (e.g. Security office)? ");
            item = new FoundItem(nextId, name, CATEGORIES[c - 1], desc, loc, today(), cname, phone, false, keptAt);
        }
        items.push_back(item);
        cout << "\nSaved! Your report ID is #" << nextId << ".\n";
        nextId++;
        save();
    }

    // ---------------- View items ----------------
    void viewItems() {
        if (items.empty()) {
            cout << "\nNo items reported yet.\n";
            return;
        }
        cout << "\n1. All items\n2. Lost items\n3. Found items\n4. Unclaimed items only\n";
        int filter = readInt("Choose: ", 1, 4);
        int shown = 0;
        for (size_t i = 0; i < items.size(); i++) {
            Item* it = items[i];
            bool show = (filter == 1) || (filter == 2 && it->type() == "LOST") ||
                        (filter == 3 && it->type() == "FOUND") || (filter == 4 && !it->isClaimed());
            if (show) {
                cout << *it;
                shown++;
            }
        }
        cout << "\n" << shown << " item(s) shown (total stored: " << Item::getTotal() << ").\n";
    }

    // ---------------- Search items ----------------
    void search() {
        string keyword = toLower(readLine("\nEnter keyword to search: "));
        int found = 0;
        for (size_t i = 0; i < items.size(); i++) {
            Item* it = items[i];
            string all = toLower(it->getName() + " " + it->getCategory() + " " + it->getDescription() +
                                 " " + it->getLocation());
            if (all.find(keyword) != string::npos) {
                cout << *it;
                found++;
            }
        }
        if (found == 0) cout << "\nNo items match \"" << keyword << "\".\n";
        else cout << "\n" << found << " result(s) found.\n";
    }

    // ---------------- Smart match ----------------
    void smartMatch() {
        cout << "\n--- Smart Match ---\n";
        int matches = 0;
        for (size_t i = 0; i < items.size(); i++) {
            Item* lost = items[i];
            if (lost->type() != "LOST" || lost->isClaimed()) continue;
            set<string> lostWords = words(lost->getName() + " " + lost->getDescription());

            for (size_t j = 0; j < items.size(); j++) {
                Item* found = items[j];
                if (found->type() != "FOUND" || found->isClaimed()) continue;
                set<string> foundWords = words(found->getName() + " " + found->getDescription());

                int common = 0;
                for (set<string>::iterator w = lostWords.begin(); w != lostWords.end(); ++w)
                    if (foundWords.count(*w)) common++;

                bool sameCategory = (lost->getCategory() == found->getCategory());
                if ((sameCategory && common >= 1) || common >= 2) {
                    int score = common * 2 + (sameCategory ? 3 : 0);
                    cout << "\nPossible match (score " << score << "):\n";
                    cout << "  LOST  #" << lost->getId() << " : " << lost->getName() << "\n";
                    cout << "  FOUND #" << found->getId() << " : " << found->getName() << "\n";
                    matches++;
                }
            }
        }
        if (matches == 0) cout << "No possible matches right now.\n";
        else cout << "\nUse 'View items' to check the details and contact the reporters.\n";
    }

    // ---------------- Mark item as returned ----------------
    void markClaimed() {
        int id = readInt("\nEnter item ID to mark as returned/claimed: ", 1, 1000000);
        Item* it = findById(id);
        if (it == nullptr) {
            cout << "No item with ID #" << id << ".\n";
        } else if (it->isClaimed()) {
            cout << "That item is already marked as returned.\n";
        } else {
            it->setClaimed(true);
            save();
            cout << "Item #" << id << " marked as returned.\n";
        }
    }

    // ---------------- Delete item ----------------
    void deleteItem() {
        int id = readInt("\nEnter item ID to delete: ", 1, 1000000);
        for (size_t i = 0; i < items.size(); i++) {
            if (items[i]->getId() == id) {
                string confirm = toLower(readLine("Delete \"" + items[i]->getName() + "\"? (y/n): "));
                if (confirm == "y") {
                    delete items[i];
                    items.erase(items.begin() + i);
                    save();
                    cout << "Item deleted.\n";
                } else {
                    cout << "Cancelled.\n";
                }
                return;
            }
        }
        cout << "No item with ID #" << id << ".\n";
    }
};

int LostFoundSystem::nextId = 1;

// ---------------- Menu ----------------
void printMenu() {
    cout << "\n==============================\n"
         << "   ReFound - Lost & Found\n"
         << "==============================\n"
         << "1. Report a lost item\n"
         << "2. Report a found item\n"
         << "3. View items\n"
         << "4. Search items\n"
         << "5. Smart match (lost vs found)\n"
         << "6. Mark item as returned\n"
         << "7. Delete an item\n"
         << "8. Exit\n";
}

// ---------------- Main function ----------------
int main() {
    LostFoundSystem system;
    system.load();

    try {
        while (true) {
            printMenu();
            int choice = readInt("Enter your choice: ", 1, 8);
            switch (choice) {
                case 1: system.addItem(true); break;
                case 2: system.addItem(false); break;
                case 3: system.viewItems(); break;
                case 4: system.search(); break;
                case 5: system.smartMatch(); break;
                case 6: system.markClaimed(); break;
                case 7: system.deleteItem(); break;
                case 8:
                    cout << "\nThank you for using ReFound. Goodbye!\n";
                    return 0;
            }
        }
    } catch (const exception& e) {
        cout << "\nProgram ended: " << e.what() << "\n";
    }
    return 0;
}
