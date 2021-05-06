#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using boost::asio::ip::tcp;

class waitfor {
    string hostport;
    bool quiet;

   public:
    waitfor(string hostport, bool quiet) : hostport(hostport), quiet(quiet) {}

    bool check_resolve() {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        size_t idx = hostport.find_last_of(":");
        string host = hostport.substr(0, idx);
        string port;
        if (idx != string::npos) {
            port = hostport.substr(idx + 1);
        }
        boost::system::error_code error;
        tcp::resolver::results_type endpoints =
            resolver.resolve(host, port, error);
        if (error) {
            return false;
        }
        return true;
    }

    bool check_connect() {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        size_t idx = hostport.find_last_of(":");
        string host = hostport.substr(0, idx);
        string port;
        if (idx != string::npos) {
            port = hostport.substr(idx + 1);
        }
        boost::system::error_code error;
        tcp::resolver::results_type endpoints =
            resolver.resolve(host, port, error);
        if (error) {
            return false;
        }
        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints, error);
        socket.close();
        if (error) {
            return false;
        }
        return true;
    }

    void run_command(vector<string> cmd) {
        char *args[cmd.size() + 1];
        args[cmd.size()] = NULL;
        if (!quiet) {
            cout << "run";
            for (vector<string>::size_type i = 0; i < cmd.size(); i++) {
                cout << " " << cmd.at(i);
                args[i] = strdup(cmd.at(i).c_str());
            }
            cout << endl;
        }
        int res = execvp(args[0], args);
        if (res) {
            perror("exec");
        }
    }

    bool run(bool resolve, int timeout, float interval) {
        if (!quiet) {
            cout << "check " << (resolve ? "resolve" : "connect") << " to "
                 << hostport << ", timeout=" << timeout
                 << ", interval=" << interval << endl;
        }
        std::chrono::milliseconds interval_sec(
            static_cast<int>(interval * 1000));
        chrono::time_point<chrono::system_clock> start =
            chrono::system_clock::now();
        while (true) {
            if (resolve) {
                if (check_resolve()) {
                    if (!quiet) {
                        cout << "resolved "
                             << (chrono::system_clock::now() - start).count() /
                                    1000000.0
                             << " sec." << endl;
                    }
                    break;
                }
            } else {
                if (check_connect()) {
                    if (!quiet) {
                        cout << "connected "
                             << (chrono::system_clock::now() - start).count() /
                                    1000000.0
                             << " sec." << endl;
                        ;
                    }
                    break;
                }
            }
            if ((chrono::system_clock::now() - start).count() >
                timeout * 1000000) {
                if (!quiet) {
                    cout << "timeout "
                         << (chrono::system_clock::now() - start).count() /
                                1000000.0
                         << " sec." << endl;
                }
                return false;
            }
            this_thread::sleep_for(interval_sec);
        }
        return true;
    }
};

int main(int argc, char **argv) {
    using namespace boost::program_options;

    options_description optdesc("wait-for-it");
    optdesc.add_options()("help,h", "show this help");
    optdesc.add_options()("strict,s", "strict option");
    optdesc.add_options()("quiet,q", "be quiet option");
    optdesc.add_options()("resolve", "resolve only");
    optdesc.add_options()("version,v", "version option");
    optdesc.add_options()("timeout,t", value<int>()->default_value(30),
                          "timeout value");
    optdesc.add_options()("interval,i", value<float>()->default_value(1.0),
                          "interval option");
    variables_map vm;
    auto const parsed = parse_command_line(argc, argv, optdesc);
    store(parsed, vm);
    notify(vm);
    if (vm.count("help")) {
        cout << optdesc << endl;
        return 0;
    }
    if (vm.count("version")) {
        cout << "wait-for-it 0.1.0" << endl;
        return 0;
    }
    auto args = collect_unrecognized(parsed.options, include_positional);

    if (args.size() != 0) {
        string hostport = args[0];
        vector<string> cmd(args.begin() + 1, args.end());
        waitfor runner(hostport, vm.count("quiet"));
        bool result = runner.run(vm.count("resolve"), vm["timeout"].as<int>(),
                                 vm["interval"].as<float>());
        if (result || !vm.count("strict")) {
            runner.run_command(cmd);
        }
        return result ? 0 : 1;
    }
    return 1;
}
