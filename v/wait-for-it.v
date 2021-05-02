module main

import os
import cli

fn do_main(cmd cli.Command) {
  print("cmd $cmd")
}

fn main() {
  mut app := cli.Command{
    name: 'wait-for-it'
    description: 'wait for connect'
    execute: do_main
  }
  app.add_flag(cli.Flag{
    flag: .bool
    name: 'quiet'
    abbrev: 'q'
    description: "Don't output any status messages"
    value: 'false'
  })
  app.add_flag(cli.Flag{
    flag: .bool
    name: 'strict'
    abbrev: 's'
    description: "Only execute subcommand if the test succeeds"
    value: 'false'
  })
  app.add_flag(cli.Flag{
    flag: .bool
    name: "version"
    abbrev: "V"
    description: "Prints version information"
    value: 'false'
  })
  app.add_flag(cli.Flag{
    flag: .float
    name: "interval"
    abbrev: "i"
    description: "interval second"
    value: '1.0'
  })
  app.add_flag(cli.Flag{
    flag: .int
    name: "timeout"
    abbrev: "t"
    description: "Timeout in seconds, zero for no timeout"
    value: '30'
  })
  app.setup()
  app.parse(os.args)
}
