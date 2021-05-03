#include <gflags/gflags.h>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using boost::asio::ip::tcp;

namespace option {
DEFINE_bool(quiet, false, "Don't output any status messages");
DEFINE_bool(strict, false, "Only execute subcommand if the test succeeds");
DEFINE_bool(resolve, false, "resolve check");
DEFINE_int32(timeout, 30, "Timeout in seconds, zero for no timeout");
DEFINE_double(interval, 1.0, "interval second");
}  // namespace option

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
                    break;
                }
            } else {
                if (check_connect()) {
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
    gflags::SetUsageMessage("wait for connection");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (argc != 1) {
        string hostport(argv[1]);
        vector<string> cmd;
        for (int i = 2; i < argc; i++) {
            if (i == 2 && string(argv[i]) == string("--")) {
                continue;
            }
            cmd.push_back(argv[i]);
        }
        waitfor runner(hostport, option::FLAGS_quiet);
        bool result = runner.run(option::FLAGS_resolve, option::FLAGS_timeout,
                                 option::FLAGS_interval);
        if (result || !option::FLAGS_strict) {
            runner.run_command(cmd);
        }
        return result ? 0 : 1;
    }
    return 1;
}
