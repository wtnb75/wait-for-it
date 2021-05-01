extern crate clap;

use clap::{App, Arg};
use std::net::{Shutdown, TcpStream, ToSocketAddrs};
use std::process;
use std::{thread, time};

fn check_dns(hostport: &str) -> bool {
    match hostport.to_socket_addrs() {
        Ok(_) => true,
        Err(_) => false,
    }
}

fn check_connect(hostport: &str) -> bool {
    match hostport.to_socket_addrs() {
        Ok(addrs) => {
            for addr in addrs.into_iter() {
                if let Ok(stream) = TcpStream::connect(addr) {
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

fn run_command(command: Vec<&str>, quiet: bool) {
    if command.len() != 0 {
        if !quiet {
            println!("run {:?}", command);
        }
        let cmd = command[0];
        let mut args = command.clone();
        args.remove(0);
        process::Command::new(cmd)
            .args(args)
            .spawn()
            .expect("command");
    } else {
        if !quiet {
            println!("no run");
        }
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
            Arg::with_name("dns")
                .long("dns")
                .required(false)
                .takes_value(false)
                .help("dns resolve(no connect)"),
        )
        .arg(Arg::with_name("command").required(false).multiple(true));
    let matches = app.get_matches();
    let addr = matches.value_of("hostport").unwrap();
    let timeout: u32 = matches.value_of("timeout").unwrap().parse().unwrap();
    let interval: f32 = matches.value_of("interval").unwrap().parse().unwrap();
    let dns: bool = matches.is_present("dns");
    let quiet: bool = matches.is_present("quiet");
    let strict: bool = matches.is_present("strict");
    let command: Vec<&str> = match matches.values_of("command") {
        Some(c) => c.into_iter().collect(),
        None => vec![],
    };
    if !quiet {
        println!(
            "try to connect addr={}, timeout={}, interval={}",
            addr, timeout, interval
        );
    }
    let start = time::SystemTime::now();
    loop {
        if dns {
            if check_dns(addr) {
                if !quiet {
                    println!("resolved after {:?}", start.elapsed().unwrap());
                }
                break;
            }
        } else {
            if check_connect(addr) {
                if !quiet {
                    println!("connected after {:?}", start.elapsed().unwrap());
                }
                break;
            }
        }
        if timeout != 0 && start.elapsed().unwrap().as_secs() > timeout as u64 {
            if !quiet {
                println!("connect failed");
            }
            if !strict {
                run_command(command, quiet);
            }
            process::exit(-1);
        }
        thread::sleep(time::Duration::from_secs_f32(interval));
    }
    run_command(command, quiet);
}
