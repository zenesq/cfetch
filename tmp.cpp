#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>
using namespace std;

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
  if (++c == 40)
    c = 31;
  return c;
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
    memory = to_string(total - free - buf - cache) + " / " + to_string(total) +
             " MiB";
  }();

  struct winsize window_size;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size);

  int tw = window_size.ws_col, gap1 = 1, gap2 = 5;
  vector<pair<string, string>> vec{
      {"@", username}, {"", hostname}, {"", distro_name}, {"", kernel},
      {"", uptime},   {"", shell},    {"", memory}};

  vector<string> blocks;
  for (pair<string, string> p : vec)
    blocks.push_back(color(p.first, rc()) + pad_left(gap1, color("⋅", 30)) +
                     pad_left(gap1, color(p.second, 38)));

  vector<string> lines{blocks[0]};
  vector<int> padding;
  int cl = tw - 2 - 2 * gap1 - vec[0].second.size(), tmp;
  for (int i = 1; i < blocks.size(); i++) {
    if ((tmp = 2 + 2 * gap1 + gap2 + vec[i].second.size()) <= cl)
      lines[lines.size() - 1] += pad_left(gap2, blocks[i]), cl -= tmp;
    else
      lines.push_back(blocks[i]), padding.push_back(cl), cl = tw - tmp + gap2;
  }
  padding.push_back(cl);

  cout << "\n";
  for (int i = 0; i < lines.size(); i++)
    cout << pad_left(padding[i] / 2, lines[i]) << '\n';
}
