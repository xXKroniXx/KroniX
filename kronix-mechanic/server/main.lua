ESX = exports['es_extended']:getSharedObject()

function IsMechanic(xPlayer)
    return xPlayer and xPlayer.job.name == Config.Job
end

function AddItem(source, item, count)
    if Config.Inventory == 'ox_inventory' then
        return exports.ox_inventory:AddItem(source, item, count)
    end

    local xPlayer = ESX.GetPlayerFromId(source)
    if not xPlayer then return false end
    xPlayer.addInventoryItem(item, count)
    return true
end

function RemoveItem(source, item, count)
    if Config.Inventory == 'ox_inventory' then
        return exports.ox_inventory:RemoveItem(source, item, count)
    end

    local xPlayer = ESX.GetPlayerFromId(source)
    if not xPlayer then return false end
    xPlayer.removeInventoryItem(item, count)
    return true
end

function GetItemCount(source, item)
    if Config.Inventory == 'ox_inventory' then
        return exports.ox_inventory:GetItemCount(source, item) or 0
    end

    local xPlayer = ESX.GetPlayerFromId(source)
    if not xPlayer then return 0 end
    local invItem = xPlayer.getInventoryItem(item)
    return invItem and invItem.count or 0
end

lib.callback.register('kronix-mechanic:server:hasItem', function(source, item, amount)
    return GetItemCount(source, item) >= amount
end)

lib.callback.register('kronix-mechanic:server:consumeRepairKits', function(source, amount)
    local xPlayer = ESX.GetPlayerFromId(source)
    if not IsMechanic(xPlayer) then return false end
    if GetItemCount(source, Config.Items.repairKit) < amount then return false end

    RemoveItem(source, Config.Items.repairKit, amount)
    return true
end)

lib.callback.register('kronix-mechanic:server:pay', function(source, amount)
    local xPlayer = ESX.GetPlayerFromId(source)
    if not IsMechanic(xPlayer) then return false end
    if xPlayer.getMoney() < amount then return false end

    xPlayer.removeMoney(amount)
    return true
end)
