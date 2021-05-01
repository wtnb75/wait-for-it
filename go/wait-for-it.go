package main

import (
	"flag"
	"fmt"
	"net"
	"os"
	"os/exec"
	"time"
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

func main() {
	var (
		timeout  = flag.Int64("timeout", 30, "")
		interval = flag.Float64("interval", 1.0, "")
		quiet    = flag.Bool("quiet", false, "")
		strict   = flag.Bool("strict", false, "")
		resolve  = flag.Bool("resolve", false, "")
	)
	flag.Parse()
	args := flag.Args()
	if len(args) == 0 {
		flag.Usage()
		return
	}
	hostport := args[0]
	cmd := args[1:]
	if len(cmd) != 0 && cmd[0] == "--" {
		cmd = cmd[1:]
	}
	w := Waitfor{
		hostport: hostport,
		timeout:  *timeout,
		quiet:    *quiet,
		strict:   *strict,
		resolve:  *resolve,
		interval: *interval,
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
