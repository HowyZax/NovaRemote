local history = {
    disable = false,
}

function history.add(page)
    if not page then return end
    if history.disable then return end
    if history[#history] == page then return end

    print('PageHistory.Add:', page.name)
    local found = false
    table.insert(history, page)
    while #history > 4 do
        table.remove(history,  1)
    end
    print '--- PageHistory ---'
    for k,v in ipairs(history) do
        print(k,v.name)
    end
    print '-------------------'
end

function history.pop()
    table.remove(history)
    if #history == 0 then
        history.add(pages['Home'])
    end
    return history[#history]
end

function history:gotoPrevious()
    local page = history.pop()
    if page then
        history.disable = true
        page:enter()
    end
    history.disable = false
end

return history
