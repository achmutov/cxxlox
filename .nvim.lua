vim.api.nvim_create_autocmd("FileType", {
  pattern = "cpp",
  callback = function()
    vim.cmd.compiler("gcc")
    vim.bo.makeprg = "cmake --build --preset debug"
  end,
})
