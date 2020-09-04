#include <dirent.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>

#include "linux_parser.h"

using std::stof;
using std::string;
using std::to_string;
using std::vector;

// DONE: An example of how to read data from the filesystem
string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

// DONE: An example of how to read data from the filesystem
string LinuxParser::Kernel() {
  string os, version, kernel;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

float LinuxParser::MemoryUtilization() { 
  float totalMem, availableMem;
  string memFilePath = kProcDirectory + kMeminfoFilename;
  totalMem = std::stof(readKey("MemTotal:", memFilePath));
  availableMem = std::stof(readKey("MemAvailable: ", memFilePath));
  return (totalMem - availableMem) / totalMem; 
}

long LinuxParser::UpTime() { 
  string line, uptime;
  std::ifstream filestream(kProcDirectory + kUptimeFilename);
  if (filestream.is_open()) {
      std::getline(filestream, line);
      std::istringstream linestream(line);
      linestream >> uptime;
      if (uptime.size() == 0) return 0;
      return std::stol(uptime);
  }
  return -1;  // means error
}

// removeing all Jiffi-related functions according to https://knowledge.udacity.com/questions/199377
// this fundtion calculates cpu utilization for a specific process
// child processes not taken into consideration on purpose (because I think of them as separate processes)
float LinuxParser::CpuUtilization(int pid) {

  string line = getLineForPidStat(kProcDirectory + to_string(pid) + kStatFilename);

  long utime, stime;
  string utimeStr, stimeStr;
  utimeStr = getNthToken(14, line);
  stimeStr = getNthToken(15, line);

  utime = (utimeStr.size() > 0) ? std::stol(utimeStr) : 0;
  stime = (stimeStr.size() > 0) ? std::stol(stimeStr) : 0;

  float total_time = ((utime + stime) / sysconf(_SC_CLK_TCK));

  return total_time / UpTime();
}

// following https://rosettacode.org/wiki/Linux_CPU_utilization
float LinuxParser::CpuUtilization() { 
  std::ifstream filestream(kProcDirectory + kStatFilename);
  string line;
  std::getline(filestream, line);
  float idle = std::stof(getNthToken(5, line));
  std::istringstream linestream(line);
  string cur; // current token
  linestream >> cur; // skipping "cpu"
  long total(0);
  for (int i = 1; i < 10; i++) {
    linestream >> cur;
    total += cur.size() >  0 ? std::stol(cur) : 0;
  }

  return 1 - (idle / total); 
}

int LinuxParser::TotalProcesses() { 
  return stoi(readKey("processes", kProcDirectory + kStatFilename)); 
}

int LinuxParser::RunningProcesses() { 
  return stoi(readKey("procs_running", kProcDirectory + kStatFilename)); 
}

string LinuxParser::Command(int pid) { 
  return getLineForPidStat(kProcDirectory + to_string(pid) + kCmdlineFilename); 
}

string LinuxParser::Ram(int pid) { 
  string filepath(kProcDirectory + to_string(pid) + kStatusFilename);
  string mem_in_kb = readKey("VmSize:", filepath); 
  if (mem_in_kb.size() == 0) return "0";
  return to_string(std::stol(mem_in_kb) / 1000);
}

string LinuxParser::Uid(int pid) { return readKey("Uid:", kProcDirectory + to_string(pid) + kStatusFilename); }

string LinuxParser::User(int pid) { 
  string target_uid = Uid(pid);
  string line, user, x, uid;
  std::ifstream filestream(kPasswordPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      linestream >> user >> x >> uid;
      if (uid == target_uid) {
        return user;
      }
    }
  }
  return "No user found"; 
}

long LinuxParser::UpTime(int pid) { 
  string line = getLineForPidStat(kProcDirectory + to_string(pid) + kStatFilename);
  string ticks = getNthToken(22, line);
  if (ticks.size() == 0) ticks = "0";
  return UpTime() - std::stol(ticks) / sysconf(_SC_CLK_TCK); 
}

string getLineForPidStat(string filepath) {
  std::ifstream filestream(filepath);
  string line;
  std::getline(filestream, line);
  return line;
}

// helper function to get N-th token from a line
string getNthToken(int n, string line) {
  std::istringstream linestream(line);
  string token;
  for (int i = 0; i < n; i++) {
    linestream >> token;
  }
  return token;
}

// helper method to read key value from file
string readKey(string searchKey, string filePath) {
  string line, key, value;
  std::ifstream filestream(filePath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      if (line.find(searchKey) == 0) {
        std::istringstream linestream(line);
        linestream >> key >> value;
        return value; 
      }
    }
  }
  return value;
}