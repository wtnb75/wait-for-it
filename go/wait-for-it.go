package main

import (
	"fmt"
	"net"
	"os"
	"os/exec"
	"time"

	flags "github.com/jessevdk/go-flags"
)

type Waitfor struct {
	hostport string
	resolve  bool
	quiet    bool
	strict   bool
	timeout  int64
	interval float64
}

func (w Waitfor) check_resolve() bool {
	_, err := net.ResolveTCPAddr("tcp", w.hostport)
	if err == nil {
		return true
	}
	return false
}

func (w Waitfor) check_connect() bool {
	addr, err := net.ResolveTCPAddr("tcp", w.hostport)
	if err != nil {
		return false
	}
	myaddr := new(net.TCPAddr)
	conn, err := net.DialTCP("tcp", myaddr, addr)
	if err != nil {
		return false
	}
	conn.Close()
	return true
}

func (w Waitfor) run_command(cmd []string) bool {
	err := exec.Command(cmd[0], cmd[1:]...).Run()
	if err != nil {
		if !w.quiet {
			println("error", err)
		}
		return false
	}
	return true
}

func (w Waitfor) run() bool {
	if !w.quiet {
		println("wait for", w.hostport, w.timeout, "sec")
	}
	start := time.Now()
	for {
		if w.resolve {
			if w.check_resolve() {
				if !w.quiet {
					println("resolved", w.hostport)
				}
				return true
			}
		} else {
			if w.check_connect() {
				if !w.quiet {
					println("connected", w.hostport)
				}
				return true
			}
		}
		if w.timeout != 0 && time.Now().Sub(start) > time.Duration(int64(time.Second)*w.timeout) {
			if !w.quiet {
				println("timeout", w.hostport)
			}
			return false
		}
		time.Sleep(time.Duration(float64(time.Second) * w.interval))
	}
}

type Options struct {
	Resolve  bool    `long:"resolve" description:"test resolve(no connect)"`
	Quiet    bool    `short:"q" long:"quiet" description:"Don't output any status messages"`
	Strict   bool    `short:"s" long:"strict" description:"Only execute subcommand if the test succeeds"`
	Timeout  int64   `short:"t" long:"timeout" default:"30" description:"Timeout in seconds, zero for no timeout"`
	Interval float64 `short:"i" long:"interval" default:"1.0" description:"interval second"`
	Version  func()  `short:"V" long:"version" description:"Prints version information"`
}

func main() {
	var opts Options
	opts.Version = func() {
		println("wait-for-it 0.1.0")
		os.Exit(0)
	}
	parser := flags.NewParser(&opts, flags.Default)
	parser.Name = "wait-for-it"
	parser.Usage = "[OPTIONS] HOST:PORT"
	args, err := flags.Parse(&opts)
	if err != nil {
		os.Exit(1)
	}
	if len(args) == 0 {
		parser.WriteHelp(os.Stdout)
		return
	}
	hostport := args[0]
	cmd := args[1:]
	if len(cmd) != 0 && cmd[0] == "--" {
		cmd = cmd[1:]
	}
	w := Waitfor{
		hostport: hostport,
		timeout:  opts.Timeout,
		quiet:    opts.Quiet,
		strict:   opts.Strict,
		resolve:  opts.Resolve,
		interval: opts.Interval,
	}
	res := w.run()
	if res || !w.strict {
		if !w.quiet {
			fmt.Printf("run %s\n", cmd)
		}
		if len(cmd) != 0 {
			w.run_command(cmd)
		}
	}
	if res {
		os.Exit(0)
	}
	os.Exit(1)
}
