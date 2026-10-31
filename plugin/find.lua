-- Neovim plugin entry point for Router (:Find)
if vim.g.loaded_router_find == 1 then
    return
end
vim.g.loaded_router_find = 1

local ok, router = pcall(require, "router")
if ok and router then
    router.setup()
end
