extern crate clap;

use clap::{App, Arg};
use std::net::{Shutdown, TcpStream, ToSocketAddrs};
use std::process;
use std::{thread, time};

struct Waitfor<'a> {
    hostport: &'a str,
    resolve: bool,
    strict: bool,
    quiet: bool,
    timeout: u64,
    interval: time::Duration,
}

impl Waitfor<'_> {
    fn check_resolve(&self) -> bool {
        match self.hostport.to_socket_addrs() {
            Ok(_) => true,
            Err(_) => false,
        }
    }
    fn check_connect(&self) -> bool {
        let timeout = self.interval / 2;
        match self.hostport.to_socket_addrs() {
            Ok(addrs) => {
                for addr in addrs.into_iter() {
                    if let Ok(stream) = TcpStream::connect_timeout(&addr, timeout) {
                        stream.shutdown(Shutdown::Both).expect("shutdown failed");
                        return true;
                    }
                }
                return false;
            }
            Err(_) => {
                return false;
            }
        }
    }

    fn run_command(&self, command: Vec<&str>) -> i32 {
        if command.len() != 0 {
            if !self.quiet {
                println!("run {:?}", command);
            }
            let cmd = command[0];
            let mut args = command.clone();
            args.remove(0);
            let res = process::Command::new(cmd)
                .args(args)
                .spawn()
                .expect("command")
                .wait();
            return res.unwrap().code().unwrap();
        } else {
            if !self.quiet {
                println!("no run");
            }
        }
        0
    }

    fn run(&self) -> bool {
        println!(
            "try to connect addr={}, timeout={}, interval={:?}",
            self.hostport, self.timeout, self.interval
        );
        let start = time::SystemTime::now();
        loop {
            if self.resolve {
                if self.check_resolve() {
                    if !self.quiet {
                        println!("resolved after {:?}", start.elapsed().unwrap());
                    }
                    break;
                }
            } else {
                if self.check_connect() {
                    if !self.quiet {
                        println!("connected after {:?}", start.elapsed().unwrap());
                    }
                    break;
                }
            }
            if self.timeout != 0 && start.elapsed().unwrap().as_secs() > self.timeout {
                if !self.quiet {
                    println!("connect failed");
                }
                return false;
            }
            thread::sleep(self.interval);
        }
        true
    }
}

fn main() {
    let app = App::new("wait-for-it")
        .version("0.1.0")
        .author("Watanabe Takashi <wtnb75@gmail.com>")
        .about("wait for connect")
        .arg(Arg::with_name("hostport").help("host:port").required(true))
        .arg(
            Arg::with_name("timeout")
                .short("t")
                .long("timeout")
                .default_value("30")
                .help("Timeout in seconds, zero for no timeout"),
        )
        .arg(
            Arg::with_name("interval")
                .short("i")
                .long("interval")
                .default_value("1.0")
                .help("interval second"),
        )
        .arg(
            Arg::with_name("quiet")
                .short("q")
                .long("quiet")
                .required(false)
                .takes_value(false)
                .help("Don't output any status messages"),
        )
        .arg(
            Arg::with_name("strict")
                .short("s")
                .long("strict")
                .required(false)
                .takes_value(false)
                .help("Only execute subcommand if the test succeeds"),
        )
        .arg(
            Arg::with_name("resolve")
                .long("resolve")
                .required(false)
                .takes_value(false)
                .help("test resolve(no connect)"),
        )
        .arg(Arg::with_name("command").required(false).multiple(true));
    let matches = app.get_matches();
    let hostport = matches.value_of("hostport").unwrap();
    let timeout: u64 = matches.value_of("timeout").unwrap().parse().unwrap();
    let interval: f32 = matches.value_of("interval").unwrap().parse().unwrap();
    let resolve: bool = matches.is_present("resolve");
    let quiet: bool = matches.is_present("quiet");
    let strict: bool = matches.is_present("strict");
    let command: Vec<&str> = match matches.values_of("command") {
        Some(c) => c.into_iter().collect(),
        None => vec![],
    };
    let w = Waitfor {
        hostport: hostport,
        timeout: timeout,
        interval: time::Duration::from_secs_f32(interval),
        quiet: quiet,
        resolve: resolve,
        strict: strict,
    };
    let r = w.run();
    if r || !w.strict {
        process::exit(w.run_command(command));
    }
    if r {
        process::exit(0);
    } else {
        process::exit(1);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::mem::drop;
    use std::net::TcpListener;

    #[test]
    fn test_check_resolve() {
        let w = Waitfor {
            hostport: "localhost:9999",
            timeout: 2,
            interval: time::Duration::from_secs(1),
            quiet: false,
            strict: false,
            resolve: true,
        };
        assert_eq!(true, w.run());
    }

    #[test]
    fn test_check_resolve_fail() {
        let w = Waitfor {
            hostport: "non-existent:9999",
            timeout: 2,
            interval: time::Duration::from_secs(1),
            quiet: false,
            strict: false,
            resolve: true,
        };
        assert_eq!(false, w.run());
    }

    #[test]
    fn test_check_connect_resolvefail() {
        let w = Waitfor {
            hostport: "non-existent:9999",
            timeout: 2,
            interval: time::Duration::from_secs(1),
            quiet: false,
            strict: false,
            resolve: false,
        };
        assert_eq!(false, w.run());
    }

    #[test]
    fn test_check_connect() {
        let listener = TcpListener::bind("127.0.0.1:0").unwrap();
        let hostport = format!("localhost:{}", listener.local_addr().unwrap().port());
        let w = Waitfor {
            hostport: &hostport,
            timeout: 2,
            interval: time::Duration::from_secs(1),
            quiet: false,
            strict: false,
            resolve: false,
        };
        assert_eq!(true, w.run());
        drop(listener);
        thread::sleep(w.interval);
        assert_eq!(false, w.run());
    }

    #[test]
    fn test_run_command() {
        let w = Waitfor {
            hostport: "localhost:9999",
            timeout: 2,
            interval: time::Duration::from_secs(1),
            quiet: false,
            strict: false,
            resolve: false,
        };
        assert_eq!(0, w.run_command(vec!["true",]));
        assert_eq!(1, w.run_command(vec!["false",]));
        let pnc = std::panic::catch_unwind(|| w.run_command(vec!["not-exist", "arg"]));
        assert!(pnc.is_err());
        assert_eq!(0, w.run_command(vec![]));
    }
}
