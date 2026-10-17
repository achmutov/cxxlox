local dap = require("dap")

dap.configurations.cpp = {
  {
    type = "codelldb",
    request = "launch",
    name = "Launch",
    program = "build/debug/lox",
    args = { "examples/exprs.lox" },
  },
}

dap.configurations.cmake = {
  {
    type = "cmake",
    request = "launch",
    name = "Launch Cmake",
  }
}
