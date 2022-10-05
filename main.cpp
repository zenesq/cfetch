#include <sys/ioctl.h>
#include <unistd.h>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

#define NO_COLOR
// #define ICO

struct file {
    ifstream stream;
    file(string filename) : stream(ifstream(filename)) {}
    ~file() { stream.close(); }

    string next() {
        string temp;
        getline(stream, temp);
        return temp;
    }

    string split_n(int n) {
        string ret;
        while (n--)
            stream >> ret;
        return ret;
    }

    string after(string s) {
        string ret;
        int n = 100;
        while (n--) {
            stream >> ret;
            if (ret == s) {
                stream >> ret;
                return ret;
            }
        }
        return "";
    }
};

string pad_left(int n_spaces, string line) {
    return string(n_spaces, ' ') + line;
}

string repeat(int n, string inp) {
    string ret;
    while (n--)
        ret += inp;
    return ret;
}

string color(string str, int c) {
    return "\033[" + to_string(c) + "m" + str + "\033[0m";
}

int rc() {
    static int c = 39;
#ifndef NO_COLOR
    if (++c == 40)
        c = 31;
#endif
    return c;
}

map<string, string> icons = {
    {"username", "@"},  {"hostname", ""}, {"distro", ""}, {"kernel", "•"},
    {"uptime", "🯊"}, {"shell", ""},    {"memory", ""}};

void print_table(vector<pair<string, string>> lines,
                 int ll,
                 int lr,
                 int rl,
                 int rr) {
    struct winsize window_size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);

    int ml1 = lines[0].first.size(), ml2 = lines[0].second.size(),
        tw = window_size.ws_col;
    for (pair<string, string> line : lines) {
        int l1 = line.first.size(), l2 = line.second.size();
        (ml1 < l1) && (ml1 = l1) || (ml2 < l2) && (ml2 = l2);
    }
    int tl = ml1 + ml2 + (ll + lr + rl + rr) + 1;
    int lp = (tw - tl) / 2;

    auto sll = [&](bool b) {
        static int c = 30;
#ifdef NO_COLOR
        return color(repeat(lp - 3, "╳"), 30);
#else
        return color(repeat(lp - 3, "╳"), c += b);
#endif
    };

    cout << pad_left(lp - 1, "╭") << repeat(tl, "\u2500") << "╮" << endl;
    for (pair<string, string> line : lines)
#ifndef NO_COLOR
        cout << "\u2595" << sll(true) << "\u258f│" << pad_left(ll, line.first)
#else
        cout << "\u2595" << sll(true) << "\u258f│"
             << pad_left(ll, color(line.first, 31))
#endif
             << string(ml1 - line.first.size(), ' ')
             << pad_left(lr,
#ifdef ICO
                         icons[line.first]
#else
                         "-"
#endif
                         )
#ifndef NO_COLOR
             << pad_left(rl, color(line.second, rc()))
#else
             << pad_left(rl, color(line.second, 37))
#endif
             << string(ml2 - line.second.size(), ' ') << pad_left(rr, "│\u2595")
             << sll(false) << "\u258f" << endl;
    cout << pad_left(lp - 1, "╰") << repeat(tl, "\u2500") << "╯" << endl;
}

int main() {
    string username = getenv("USER");
    string hostname = file("/etc/hostname").next();

    string distro_name;
    [&]() {
        file temp("/etc/os-release");
        string prefix = "PRETTY_NAME=", line;
        while (true) {
            if ((line = temp.next()).substr(0, prefix.size()) == prefix) {
                distro_name = line.substr(prefix.size() + 1);
                distro_name.pop_back();
                break;
            }
        }
    }();

    string kernel = file("/proc/version").split_n(3);

    string uptime;
    [&]() {
        int us, um, uh;
        us = stoi(file("/proc/uptime").split_n(1));
        um = us / 60, us = us % 60, uh = um / 60, um = um % 60;
        if (uh > 0)
            uptime += to_string(uh) + "h ";
        if (um > 0)
            uptime += to_string(um) + "m ";
        uptime += to_string(us) + "s";
    }();

    string shell = getenv("SHELL");
    [&]() {
        int i = shell.size() - 1;
        while (shell[i] != '/')
            i--;
        shell = shell.substr(i + 1);
    }();

    string memory;
    [&]() {
        file temp("/proc/meminfo");
        int total = stoi(temp.after("MemTotal:")) / 1024,
            free = stoi(temp.after("MemFree:")) / 1024,
            buf = stoi(temp.after("Buffers:")) / 1024,
            cache = stoi(temp.after("Cached:")) / 1024;
        memory = to_string(total - free - buf - cache) + " / " + to_string(total) + " MiB";
    }();

    vector<pair<string, string>> lines = {
        {"username", username}, {"hostname", hostname}, {"distro", distro_name},
        {"kernel", kernel},     {"uptime", uptime},     {"shell", shell},
        {"memory", memory}};

    print_table(lines, 3, 7, 7, 7);
}
