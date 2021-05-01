package main

import (
	"net"
	"testing"
)

func TestResolve(t *testing.T) {
	w := Waitfor{
		hostport: "localhost:3000",
	}
	if !w.check_resolve() {
		t.Errorf("resolve %s", w.hostport)
	}
	w.hostport = "not-existent"
	if w.check_resolve() {
		t.Errorf("resolve %s", w.hostport)
	}
}

func TestConnect(t *testing.T) {
	listener, err := net.Listen("tcp", ":0")
	if err != nil {
		t.Error("setup 1")
	}
	t.Logf("%s", listener.Addr().String())

	w := Waitfor{
		hostport: listener.Addr().String(),
	}
	if !w.check_connect() {
		t.Errorf("connect %s", w.hostport)
	}
	listener.Close()
	if w.check_connect() {
		t.Errorf("closed connect %s", w.hostport)
	}
	w.hostport = "not-existent:9999"
	if w.check_connect() {
		t.Errorf("connect %s", w.hostport)
	}
}

func TestRunCommand(t *testing.T) {
	w := Waitfor{quiet: true}
	if !w.run_command([]string{"echo", "1", "2"}) {
		t.Error("run command")
	}
	if !w.run_command([]string{"true"}) {
		t.Error("run true")
	}
	if w.run_command([]string{"false"}) {
		t.Error("run false")
	}
	if w.run_command([]string{"non-existent-command", "arg1", "arg2"}) {
		t.Error("run non-existent")
	}
}
