#include <iostream>  // Para sa input (cin) at output (cout)
#include <vector>    // Para sa dynamic array (list) ng expenses
#include <string>    // Para sa paghawak ng text (date, category, desc)
#include <thread>    // Para mapatakbo ang alarm sa background habang nagta-type ka
#include <atomic>    // Para sa safe na pag-stop ng thread (atomic boolean)
#include <iomanip>   // Para sa pag-format ng decimal places (setprecision)
#include <limits>    // Para sa pag-clear ng input buffer (numeric_limits)
#include <fstream>   // Para sa pag-save at pag-load ng file (txt file)

#ifdef _WIN32
#include <windows.h> // Para sa Windows-specific functions
#include <mmsystem.h> // Para sa PlaySound function
#pragma comment(lib, "winmm.lib") // Sinasabi sa compiler na kailangan ang multimedia library
#endif

using namespace std;

// Blueprint ng bawat isang gastusin
struct Expense { 
    string date; 
    double amount; 
    string category, desc; 
};

class ExpenseManager {
    vector<Expense> list; // Vector: lalagyan ng lahat ng Expense objects
    double limit = 0;      // Budget limit para sa napiling araw
    string sDate = "", filename = "expense_data.txt"; // Pangalan ng file na pagse-save-an
    string cats[6] = {"Food", "Transport", "Bills", "School", "Personal", "Others"};

    // Static function: Ito ang tumatakbo sa loob ng thread para sa tunog
    static void alarm(atomic<bool>& go) {
        while (go) { // Hangga't hindi "false" ang 'go', tutunog ito
            #ifdef _WIN32
                // PlaySound: Pinapatunog ang Windows System Alarm (SND_ASYNC = hindi nagha-hang ang program)
                PlaySound(TEXT("SystemHand"), NULL, SND_ALIAS | SND_ASYNC);
                this_thread::sleep_for(chrono::milliseconds(800)); // Interval ng bawat tunog
            #else
                cout << "\a" << flush; // System beep para sa non-windows
                this_thread::sleep_for(chrono::milliseconds(500));
            #endif
        }
    }

public:
    // Nililinis ang keyboard buffer para iwas sa "input skipping" bug
    void clearBuf() { 
        cin.clear(); 
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
    }

    // Isinusulat ang lahat ng nasa vector papunta sa .txt file
    void saveToFile() {
        ofstream f(filename); // ofstream: Output File Stream
        for (auto& e : list) 
            f << e.date << "|" << e.amount << "|" << e.category << "|" << e.desc << endl;
    }

    // Binabasa ang .txt file at ibinabalik sa loob ng vector 'list'
    void loadFromFile() {
        list.clear(); 
        ifstream f(filename); // ifstream: Input File Stream
        if (!f) return; // Kung wala pang file, huwag muna gawin
        Expense e;
        // getline(f, e.date, '|'): Babasahin ang text hanggang makita ang symbol na '|'
        while (getline(f, e.date, '|') && (f >> e.amount) && f.ignore() && getline(f, e.category, '|') && getline(f, e.desc)) 
            list.push_back(e); // push_back: Idadagdag ang data sa dulo ng vector
    }

    // Pagtatakda ng Petsa at Budget Limit sa simula
    bool init() {
        loadFromFile();
        cout << "\n=== EXPENSE TRACKER (2026) ===\nDate (YYYY-MM-DD) or 'EXIT': "; cin >> sDate;
        if (sDate == "EXIT" || sDate == "exit") return false; // Exit condition

        // Lambda-like loop para hanapin kung may data na sa petsang pinili
        double spent = 0; for(auto& e : list) if(e.date == sDate) spent += e.amount;
        if(spent > 0) {
            cout << "[!] Spent P" << spent << " on " << sDate << ". Continue? (Y/N): ";
            char a; cin >> a; if (toupper(a) == 'N') return init(); // Recursion para bumalik sa simula
        }
        cout << "Limit for " << sDate << ": "; cin >> limit; 
        clearBuf(); return true;
    }

    // Ang logic para sa Budget Alarm
    void checkBudget() {
        double total = 0; for (auto& e : list) if(e.date == sDate) total += e.amount;
        if (total > limit) {
            atomic<bool> go(true); // Signal para sa thread
            thread t(alarm, ref(go)); // Gagawa ng bagong thread 't' para sa alarm function
            string s;
            // Loop habang naghihintay ng "STOP" mula sa user nang hindi tumitigil ang tunog
            while(cout << "\nBUDGET EXCEEDED FOR " << sDate << "! TYPE 'STOP': " && cin >> s && s != "STOP" && s != "stop");
            go = false; // Ititigil ang alarm condition
            t.join();   // Lilinisin ang thread memory
            cout << "Silenced.\n";
        }
    }

    // Pagdagdag ng bagong gastusin
    void add() {
        Expense e; e.date = sDate;
        cout << "Amount (0 to Cancel): "; if (!(cin >> e.amount) || e.amount == 0) { clearBuf(); return; }
        cout << "Cat (1-Food, 2-Trans, 3-Bills, 4-Sch, 5-Pers, 6-Other): ";
        int c; cin >> c; 
        // Ternary operator: shortcut ng If-Else para sa Category
        e.category = (c>0 && c<7) ? cats[c-1] : "Others";
        cout << "Desc: "; clearBuf(); getline(cin, e.desc);
        list.push_back(e); saveToFile(); checkBudget();
    }

    // Nagpapakita ng status ng budget sa screen
    void status() {
        double dayT = 0; for (auto& e : list) if(e.date == sDate) dayT += e.amount;
        // setprecision(2): Siguradong 2 decimal places ang pera (e.g. 10.50)
        cout << fixed << setprecision(2) << "\n--- STATUS: " << sDate << " ---\nBUDGET: P" << limit 
             << "\nSPENT:  P" << dayT << "\nREM:    P" << (limit-dayT<0?0:limit-dayT) 
             << (dayT > limit ? "\nOVER:   P" + to_string(dayT-limit) : "") << "\n------------\n";
    }

    // CRUD: View, Edit, at Delete records
    void viewRecords() {
        if (list.empty()) return;
        // left << setw(n): Pag-aayos ng columns para maging pantay-pantay ang display
        cout << left << setw(4) << "ID" << setw(12) << "DATE" << setw(10) << "AMT" << "DESC\n";
        for (int i = 0; i < list.size(); i++) 
            cout << "[" << i+1 << "] " << setw(12) << list[i].date << setw(10) << list[i].amount << list[i].desc << endl;
        
        cout << "[1] Edit [2] Delete [0] Back: "; int ch, id; cin >> ch;
        if (ch == 0 || !(cin >> id) || id < 1 || id > list.size()) return;
        
        if (ch == 2) list.erase(list.begin() + id - 1); // erase: Tinatanggal ang record sa vector
        else { 
            cout << "New Amt: "; cin >> list[id-1].amount; 
            cout << "New Desc: "; clearBuf(); getline(cin, list[id-1].desc); 
            checkBudget(); 
        }
        saveToFile();
    }

    // Summary report bago isara ang program
    void showFinalSummary() {
        double dayT = 0, allT = 0;
        for (auto& e : list) { 
            allT += e.amount; // Total ng lahat ng records sa file
            if (e.date == sDate) dayT += e.amount; // Total lang para sa araw na ito
        }
        cout << "\n=== SUMMARY ===\nSPENT TODAY: P" << dayT << "\nALL-TIME:    P" << allT << "\n===============\n";
    }
};

int main() {
    ExpenseManager em; 
    if (!em.init()) return 0; // Tatawagin ang initial setup

    for (int opt; true;) { // Infinite loop para sa Main Menu
        em.status(); 
        cout << "1.Add 2.Records 3.Switch 0.Exit\nAction: ";
        if (!(cin >> opt)) { em.clearBuf(); continue; } // Error handling sa menu selection
        
        if (opt == 1) em.add(); 
        else if (opt == 2) em.viewRecords(); 
        else if (opt == 3) em.init(); 
        else if (opt == 0) { em.showFinalSummary(); break; } // lalabas sa loop at magsasara
    }
    return 0;
}