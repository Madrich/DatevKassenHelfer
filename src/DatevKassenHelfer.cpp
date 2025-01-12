// A small application to handle Datev Cash Register files
// Purpose to compress daily registrations


#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <map>
#include <string>
#include <vector>

#include "ezOptionParser.hpp"

using namespace ez;
using namespace std;


vector<std::filesystem::path> getAllFiles(std::filesystem::path basepath, string ext)
{
    if (basepath.empty()) basepath = std::filesystem::current_path();

    vector<std::filesystem::path> fpaths;

    // Iterate through the directory
    for (const auto& entry : std::filesystem::directory_iterator(basepath))
    {
        if (!entry.is_regular_file()) continue;

        if (entry.path().extension().string() == ext)
        {
            fpaths.push_back(entry.path());
        }
    }

    return fpaths;
}


std::string convertToUTF8(const std::string& input)
{
    typedef std::codecvt_byname<wchar_t, char, std::mbstate_t> codecvt;

    // the following relies on non-standard behavior, codecvt destructors are supposed to be protected and unusable here, but VC++ doesn't complain.
    std::wstring_convert<codecvt> cp1252(new codecvt(".8859"));
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converterToUTF8;
    auto ws = cp1252.from_bytes(input);
    std::string utf8StringConverted = converterToUTF8.to_bytes(ws);
    return utf8StringConverted;
}


vector<string> split(string& s, const string& delimiter)
{
    vector<string> tokens;
    size_t pos = 0;
    string token;

    while ((pos = s.find(delimiter)) != string::npos)
    {
        token = s.substr(0, pos);
        tokens.push_back(token);

        s.erase(0, pos + delimiter.length());
    }

    tokens.push_back(s);

    return tokens;
}

class Entry
{
public:

    Entry() = default;


    Entry(string line)
    {
        parseDatevEntry(line);
    }


    void parseDatevEntry(string line)
    {
        auto sline = split(line, ";");

        for (int i = 0; i < sline.size(); ++i)
        {
            sline[i].erase(std::remove(sline[i].begin(), sline[i].end(), '\"'), sline[i].end());

            if (i == 0) currency_ = sline[i];
            else if (i == 1 && sline[i].length())
            {
                std::replace(sline[i].begin(), sline[i].end(), ',', '.');
                value_ = std::stod(sline[i]);
            }
            else if (i == 2) rnumber_ = sline[i];
            else if (i == 3)
            {
                //auto s = split(sline[i], "-");
                date_ = sline[i];
            }
            else if (i == 4) btext_ = sline[i];
            else if (i == 5 && sline[i].length())
            {
                std::replace(sline[i].begin(), sline[i].end(), ',', '.');
                ust_ = std::stod(sline[i]);
            }
            else if (i == 6) bu_ = sline[i];
            else if (i == 7) konto_ = sline[i];
            else if (i == 8) kost1_ = sline[i];
            else if (i == 9) kost2_ = sline[i];
            else if (i == 10) kostm_ = sline[i];
            else if (i == 11 && sline[i].length())
            {
                std::replace(sline[i].begin(), sline[i].end(), ',', '.');
                skonto_ = std::stod(sline[i]);
            }
            else if (i == 12) msg_ = sline[i];
        }
    }


    string toDatevString(Entry &e)
    {
        stringstream line;
        line << "\"" << e.currency_ << "\";";
        line << "\"" << e.value_ << "\";";
        line << "\"" << e.rnumber_ << "\";";
        line << "\"" << e.date_ << "\";";
        line << "\"" << e.btext_ << "\";";
        line << "\"" << e.ust_ << "\";";
        line << "\"" << e.bu_ << "\";";
        line << "\"" << e.konto_ << "\";";
        line << "\"" << e.kost1_ << "\";";
        line << "\"" << e.kost2_ << "\";";
        line << "\"" << e.kostm_ << "\";";
        line << "\"" << e.skonto_ << "\";";
        line << "\"" << e.msg_ << "\"";
        return line.str();
    }


    void print()
    {
        //cout << " #0:" << currency_ << " #1:" << value_ << " #2:" << rnumber_ << " #3:" << date_ << " #4:" << btext_ << " #5:" << ust_ << " #6:" << bu_ << " #7:" << konto_ << " #8:" << kost1_ << " #9:" << kost2_ << " #10:" << kostm_ << " #11:" << skonto_ << " #12:" << msg_ << endl;
        cout << std::left << std::fixed << std::setprecision (2) <<
            std::setw(5) << currency_ <<
            std::setw(10) << std::right << value_ << std::left << "   " <<
            std::setw(14) << rnumber_ <<
            std::setw(6) << date_ <<
            std::setw(40) << btext_ <<
            std::setw(5) << std::right << ust_ << std::left << "   " <<
            std::setw(2) << bu_ <<
            std::setw(6) << std::right << konto_ << std::left << "   " <<
            std::setw(2) << kost1_ <<
            std::setw(2) << kost2_ <<
            std::setw(4) << kostm_ <<
            std::setw(6) << skonto_ <<
            std::setw(20) << msg_ << endl;
    }

    string currency_; // währung
    double value_{0.0}; // VorzBetrag
    string rnumber_; // RechNr
    string date_; // BelegDatum
    string btext_; // Belegtext
    double ust_{0.0}; // UStSatz
    string bu_; // BU
    string konto_; // Gegenkonto
    string kost1_;
    string kost2_;
    string kostm_; //Kostmenge
    double skonto_{0.0}; //
    string msg_; //Nachricht
};


vector<Entry> importDatevCSV(string fname, string &header)
{
    std::ifstream fid(fname);

    if (!fid.is_open()) return {};

    string line;
    vector<Entry> lines;

    while (getline(fid, line))
    {
        if (header.empty()) // parse the header line first
        {
            header = line;
            continue;
        }

        lines.push_back(Entry(line));
    }

    return lines;
}


bool exportDatevCSV(string fname, vector<Entry> &entries, string &header)
{
    std::ofstream fid(fname);

    if (!fid.is_open()) return false;

    fid << header << "\n";

    for (auto &e : entries)
    {
        fid << e.toDatevString(e) << "\n";
    }

    fid.close();
    return true;
}


void compressDatevDays(string ifile, string ofile)
{
    string header;
    auto entries = importDatevCSV(ifile, header);
    string currDate;
    vector<Entry> output;
    vector<Entry> dayoutput;
    std::map<string, Entry> kontoAcc;

    auto store = [&]()
    {
        // add all accumulated
        for (auto &it : kontoAcc) output.push_back(it.second);

        // add all daylies
        output.insert(output.end(), dayoutput.begin(), dayoutput.end());
        dayoutput.clear();
        kontoAcc.clear();
    };

    for (int i = 0; i < entries.size(); ++i)
    {
        Entry &entry = entries[i];

        // init a new day
        if (currDate != entry.date_)
        {
            store();
            currDate = entry.date_;

            if (i == (entries.size() - 1)) store(); // in case of last i
        }

        if (entry.konto_ == "0")
        {
            dayoutput.push_back(entry);
            continue;
        }

        // all accumulated
        string key = entry.konto_ + "_" + entry.btext_;

        if (kontoAcc.find(key) == kontoAcc.end())
        {
            kontoAcc[key] = entry;
        }
        else
            kontoAcc[key].value_ += entry.value_;
    }

    // print all
    cout << header << endl;

    for (auto &e : output)
    {
        e.print();
    }

    exportDatevCSV(ofile, output, header);
}


int main(int argc, const char * argv[])
{
    ezOptionParser opt;

    opt.overview = "CashR to handle Datev Cash Register files.";
    opt.syntax = "CashR [OPTIONS]";
    opt.example = "CashR -i datev.csv -o datevcomp.csv\n\n";
    opt.add("", 0, 0, 0, "Print this usage message.", "-h", "-help", "--help", "--usage");
    opt.add("", 0, 0, 0, "option to compress datev files by accumulation.", "-c", "--compress");
    opt.add("", 0, 0, 0, "Convert all csv files in the folder.", "-a", "--all");
    opt.add("datev.csv", 0, 1, 0, "File to import arguments.", "-i", "--import");
    opt.add("datevC.txt", 0, 1, 0, "File to export arguments.", "-o", "--export");
    opt.parse(argc, argv);
    vector<string> badOptions;
    string usage;

    if (!opt.gotExpected(badOptions))
    {
        for (int i = 0; i < badOptions.size(); ++i) cerr << "ERROR: Got unexpected number of arguments for option " << badOptions[i] << ".\n\n";

        opt.getUsage(usage);
        cout << usage;
        return 1;
    }

    if (opt.isSet("-h"))
    {
        opt.getUsage(usage);
        cout << usage;
        return 1;
    }

    if (opt.isSet("-c") && opt.isSet("-a"))
    {
        auto fpaths = getAllFiles(std::filesystem::path(), ".csv");

        if (fpaths.empty()) cerr << "No csv files to convert" << endl;

        for (auto fpath : fpaths)
        {
            cout << "Convert: " << fpath << endl;
            string ifile = fpath.string();
            string ofile = fpath.filename().string() + "_C" + fpath.extension().string();
            compressDatevDays(ifile, ofile);
        }
    }
    else if (opt.isSet("-c") && opt.isSet("-i") && opt.isSet("-o"))
    {
        string ifile, ofile;
        opt.get("-i")->getString(ifile);
        opt.get("-o")->getString(ofile);
        cout << "Input:  " << ifile << "\nOutput: " << ofile << endl;
        compressDatevDays(ifile, ofile);
    }
    else
    {
        string pretty;
        opt.prettyPrint(pretty);
        cout << pretty;
    }
}
