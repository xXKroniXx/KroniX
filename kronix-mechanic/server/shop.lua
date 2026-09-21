lib.callback.register('kronix-mechanic:server:buyItem', function(source, item, amount, totalPrice)
    local xPlayer = ESX.GetPlayerFromId(source)
    if not IsMechanic(xPlayer) then return false end
    if xPlayer.getMoney() < totalPrice then return false end

    xPlayer.removeMoney(totalPrice)
    AddItem(source, item, amount)
    return true
end)
